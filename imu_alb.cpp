#include "imu_alb.hpp"
#include <iostream>
using namespace boost::asio;

namespace hy_agv
{
    IMU_ALB::IMU_ALB(const std::string& serialPortName)
    {
        if(init(serialPortName))
            startRead();
        else
            std::cerr<<"imu init failed, please check it"<<std::endl;
    }
    // 根据数据手册设置参数
    void IMU_ALB::setParam()
    {
        sp_->set_option(serial_port::baud_rate(115200));
        sp_->set_option(serial_port::flow_control(serial_port::flow_control::none));
        sp_->set_option(serial_port::parity(serial_port::parity::none));
        sp_->set_option(serial_port::stop_bits(serial_port::stop_bits::one));
        sp_->set_option(serial_port::character_size(8));
    }

    void IMU_ALB::readThread()
    {
        while (true)
        {
            try
            {
                updateData();
                // 1ms更新频率，这里能太短
                std::this_thread::sleep_for(std::chrono::microseconds(1000));
            }
            catch (...)
            {
                // reInit
                reInit();
            }
        }
    }

    // 没有写线程
    void IMU_ALB::writeThread()
    {
    }

    hy_agv::imuData IMU_ALB::updateData()
    {
        unsigned char rawData[20];
        int num = 0;
        // 读取头部信息 
        while(num < 100)
        {
            spRead(rawData,0,2);
            if(0x3A==rawData[0]&&0x0B==rawData[1])
                break;
            num ++;
        }
        // 读取剩余信息
        {
            spRead(rawData,2,15);
            memcpy(U_transform.c, &rawData[4], 10);
            unsigned char check_sum = 0;
            // 数据校验
            for (int i = 1; i < 14; ++i)
            {
                check_sum +=rawData[i];
            }
            if(check_sum==rawData[14])
            {
                decode();
            }
        }
        return data_;
    }

    // 根据数据手册解码
    void IMU_ALB::decode()
    {
        static bool init = false;
        static float init_angle = 0;
        mutex_.lock();
        data_.stamp = std::chrono::steady_clock::now();
        if(!init)
        {
            data_.gAngle_ = 0;
            init_angle = (float)U_transform.i[0] / 100.0f;// angle / 100.0;
            init = true;
        }
        data_.gAngle_ = (float)U_transform.i[0] / 100.0f - init_angle;// angle / 100.0;
        data_.gRate_ = (float)U_transform.i[1] / 50.0f;// rate / 50.0;
        data_.accX_= (float)U_transform.i[2] / 1000.0f;//x_acc;
        data_.accY_ = (float)U_transform.i[3] / 1000.0f;//y_acc;
        data_.accZ_ = (float)U_transform.i[4] / 1000.0f;//z_acc;
        //std::cout<<"angle "<<m_data.gAngle_<<std::endl;
        updateTime_ = std::chrono::steady_clock::now();
        mutex_.unlock();
    }
    // 外部接口调用，如IMU长时间没有更新数据将会手动更新驱动程序
    imuData IMU_ALB::getImuData()
    {
        auto now = std::chrono::steady_clock::now();
        // 超过2s陀螺仪数据都没有更新
        if(std::chrono::duration_cast<std::chrono::seconds>(now - updateTime_).count() > 2)
        {
            reInit();
        }
        mutex_.lock();
        auto tmp = data_;
        mutex_.unlock();
        return tmp;
    }
}
