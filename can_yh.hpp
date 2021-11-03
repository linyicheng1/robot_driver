#ifndef __CAN_YH_H
#define __CAN_YH_H

// thread
#include <mutex>
#include <thread>

// can
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <net/if.h>
#include <iostream>
#include <linux/can.h>
#include <cstring>
#include <unistd.h>

// else
#include <queue>
#include <vector>
#include <unordered_map>

namespace hy_agv
{
    class canYH
    {
    public:
        // 构造函数
        canYH() = default;
        // 初始化CAN
        bool init();
        // 取消初始化CAN
        void deInit();
        // 出现异常时的重新初始化
        void reInit();
        // 开启新线程读取数据
        void startRead();
        // 暂停新线程读取数据
        void stopRead();
        // 开启新线程写入数据
        void startWrite();
        // 暂停新线程写入数据
        void stopWrite();
        // 添加需要发送的数据
        void addSendData(int id, int length,const unsigned char* data);
        // 添加定时发送数据
        void addSendDataTimed(int time_ms, int id, int length,const unsigned char* data);
        // 添加需要接受的数据，采用格式
        // 指定id + 指定前面几位数据的方式区分不同的数据
        int addReceiveId(unsigned int receiveId, int maskLength = 0, std::vector<unsigned char> mask={});
        std::pair<std::chrono::steady_clock::time_point,std::vector<unsigned char>> getReceiveData(int index);
        // 获取数据更新时间
        std::chrono::steady_clock::time_point updateTime(){return updateTime_;}
    protected:
        bool connectSocket();
        void readThread();
        void writeThread();
        // 读写操作
        void canSend(int id, int length,const std::vector<unsigned char>&data);
        void canReceive(struct can_frame& receive);
        // 实时发送
        std::queue<int> sendIds_;
        std::queue<int> sendLengths_;
        std::queue<std::vector<unsigned char>> sendData_;
        // 定时发送
        std::vector<int> sendTimes_;
        std::vector<int> sendIdsTimed_;
        std::vector<int> sendLengthsTimed_;
        std::vector<std::vector<unsigned char>> sendDataTimed_;
        //
        int receiveIndex_ = 0;
        std::unordered_map<int, std::pair<std::chrono::steady_clock::time_point,std::vector<unsigned char>>> receiveDataMap_;
        std::vector<int> receiveDataMapId_;
        std::vector<unsigned int> receiveIds_;
        std::vector<int> receiveMaskLength_;
        std::vector<std::vector<unsigned char>> receiveMasks_;
        // can
        int socket_ = -1;
        int family_ = PF_CAN;
        int type_ = SOCK_RAW;
        int proto_ = CAN_RAW;
        struct sockaddr_can addr_{};
        struct ifreq ifr_{};

        // thread
        std::thread *writeThread_{};
        std::thread *readThread_{};
        std::mutex readMutex_{};
        std::mutex writeMutex_{};
        // time
        std::chrono::steady_clock::time_point updateTime_{};
    };
}

#endif // __CAN_YH_H
