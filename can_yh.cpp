#include "can_yh.hpp"

namespace hy_agv
{

    bool canYH::init()
    {
        bool success = true;
        if(socket_ < 0)
        {
            for(int i = 0;i < 5;i ++)
            {
                if(connectSocket())
                {
                    std::cout<<"can init success, try to start two new thread"<<std::endl;
                    startRead();
                    startWrite();
                    break;
                }
                else
                {
                    std::cerr<<" can init failed, try reInit ... "<<i<<" times"<<std::endl;
                }
            }
        }
        return success;
    }

    bool canYH::connectSocket()
    {
        socket_ = socket(family_, type_, proto_);
        struct timeval timeout = {0,10000};
        int so_rcvbuf = 4000;
        setsockopt(socket_,SOL_SOCKET,SO_RCVBUF,&so_rcvbuf,sizeof(so_rcvbuf));
        setsockopt(socket_,SOL_SOCKET,SO_SNDBUF,&so_rcvbuf,sizeof(so_rcvbuf));

        setsockopt(socket_,SOL_SOCKET,SO_RCVTIMEO,(char *)&timeout,sizeof(struct timeval));
        if(socket_ < 0)
        {
            std::cerr<<"can init error !!"<<std::endl;
        }
        addr_.can_family = family_;
        strcpy(ifr_.ifr_name, "can0");
        ioctl(socket_, SIOCGIFINDEX, &ifr_);
        addr_.can_ifindex = ifr_.ifr_ifindex;
        if (bind(socket_, (struct sockaddr *)&addr_, sizeof(addr_)) < 0)
        {
            std::cerr<<"can init error !!"<<std::endl;
            return false;
        }
        std::cout<<"can init success !!!"<<std::endl;
        return true;
    }

    void canYH::deInit()
    {
        // 断开连接
        shutdown(socket_,SHUT_RDWR);
        // can
        socket_ = -1;
        family_ = PF_CAN;
        type_ = SOCK_RAW;
        proto_ = CAN_RAW;
        sockaddr_can addr{};
        struct ifreq ifr{};
        addr_ = addr;
        ifr_ = ifr;
    }

    void canYH::reInit()
    {
        stopRead();
        stopWrite();
        deInit();
        init();
        while (!sendIds_.empty())
        {
            sendData_.pop();
            sendIds_.pop();
            sendLengths_.pop();
        }
    }

    void canYH::startRead()
    {
        readThread_ = new std::thread(&canYH::readThread, std::ref(*this));
    }

    void canYH::stopRead()
    {
        delete readThread_;
    }

    void canYH::startWrite()
    {
        writeThread_ = new std::thread(&canYH::writeThread, std::ref(*this));
    }

    void canYH::stopWrite()
    {
        delete writeThread_;
    }


    void canYH::readThread()
    {
        while (true)
        {
            try
            {
                can_frame receive{};
                canReceive(receive);
                for(int i = 0;i < receiveIds_.size();i ++)
                {
                    if(receive.can_id == receiveIds_.at(i))
                    {// 和指定的id保持一致
                        bool matchable = true;
                        for(int j = 0;j < receiveMaskLength_.at(i);j ++)
                        {// 判断前面几位是否和指定的一致
                            if(receive.data[j] != receiveMasks_.at(i).at(j))
                            {
                                matchable = false;
                            }
                        }
                        // success update data
                        if(matchable)
                        {// 数据正确匹配，采用一个地图映射保存收到数据的时间 + 收到的数据 
                            std::vector<unsigned char> d;
                            for(int i = 0;i < receive.can_dlc;i ++)
                            {
                                d.emplace_back(receive.data[i]);
                            }
                            readMutex_.lock();
                            receiveDataMap_.find(receiveDataMapId_.at(i))->second = std::make_pair(std::chrono::steady_clock::now(),d);
                            readMutex_.unlock();
                            updateTime_ = std::chrono::steady_clock::now();
                        }
                    }
                }
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
            catch (...)
            {
                reInit();
            }
        }
    }

    void canYH::writeThread()
    {
        std::vector<unsigned char> data;
        int id;
        int length;
        int cnt = 0;
        while (true)
        {
            try
            {
                // 实时发送，有数据就发送
                if(!sendIds_.empty())
                {
                    data = sendData_.front();
                    id = sendIds_.front();
                    length = sendLengths_.front();
                    writeMutex_.lock();
                    sendData_.pop();
                    sendIds_.pop();
                    sendLengths_.pop();
                    writeMutex_.unlock();
                    canSend(id, length, data);
                    // 1 ms
                }
                // 定时发送，根据设定的定时时间，循环发送数据
                for(int i = 0;i < sendIdsTimed_.size();i ++)
                {
                    if(cnt % sendTimes_.at(i) == 0)
                    {// 时间到了
                        std::this_thread::sleep_for(std::chrono::microseconds(1000));
                        canSend(sendIdsTimed_.at(i), sendLengthsTimed_.at(i), sendDataTimed_.at(i));
                    }
                }
                cnt ++;
                if(cnt > 100000)
                    cnt = 0;
                std::this_thread::sleep_for(std::chrono::microseconds(1000));
            }
            catch (...)
            {
                reInit();
            }
        }
    }

    void canYH::canSend(int id, int length, const std::vector<unsigned char>&data)
    {
        struct can_frame frame{};
        frame.can_id = id;
        frame.can_dlc = length;
        for(int i = 0;i < length;i ++)
        {
            frame.data[i] = data[i];
        }
        if(write(socket_, &frame, sizeof(frame)) < 0)
        {
            reInit();
        }
    }

    void canYH::canReceive(can_frame &receive)
    {
        if (read(socket_, &receive, sizeof(receive)) < 0)
        {
            reInit();
        }
        //printf("id%x data %x %x %x %x %x %x %x %x \n",receive.can_id,receive.data[0],receive.data[1],receive.data[2],receive.data[3],receive.data[4],receive.data[5],receive.data[6],receive.data[7]);
    }

    void canYH::addSendData(int id, int length, const unsigned char *data)
    {
        std::vector<unsigned char> t;
        t.reserve(length);
        for(int i = 0;i < length;i ++)
        {
            t.emplace_back(data[i]);
        }
        writeMutex_.lock();
        sendIds_.push(id);
        sendLengths_.push(length);
        sendData_.push(t);
        writeMutex_.unlock();
    }

    void canYH::addSendDataTimed(int time_ms, int id, int length, const unsigned char *data)
    {
        std::vector<unsigned char> t;
        t.reserve(length);
        for(int i = 0;i < length;i ++)
        {
            t.emplace_back(data[i]);
        }
        writeMutex_.lock();
        sendTimes_.emplace_back(time_ms);
        sendIdsTimed_.emplace_back(id);
        sendLengthsTimed_.emplace_back(length);
        sendDataTimed_.emplace_back(t);
        writeMutex_.unlock();
    }

    int canYH::addReceiveId(unsigned int receiveId, int maskLength, std::vector<unsigned char> mask)
    {
        readMutex_.lock();
        receiveIds_.emplace_back(receiveId);
        receiveMaskLength_.emplace_back(maskLength);
        receiveMasks_.emplace_back(mask);
        receiveDataMapId_.emplace_back(receiveIndex_);
        std::vector<unsigned char> data;
        receiveDataMap_.insert(std::make_pair(receiveId, std::make_pair(std::chrono::steady_clock::now(),data)));
        readMutex_.unlock();
        receiveIndex_ ++;
        return receiveIndex_ - 1;
    }

    std::pair<std::chrono::steady_clock::time_point, std::vector<unsigned char>> canYH::getReceiveData(int index)
    {
        auto now = std::chrono::steady_clock::now();
        // 超过2s数据都没有更新
        if(std::chrono::duration_cast<std::chrono::seconds>(now - updateTime_).count() > 2)
        {
            reInit();
        }
        // return
        readMutex_.lock();
        auto re = receiveDataMap_.find(index)->second;
        readMutex_.unlock();
        return re;
    }
}