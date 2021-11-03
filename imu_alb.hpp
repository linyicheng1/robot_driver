#ifndef __IMU_ALB_H
#define __IMU_ALB_H

#include "serial_port.hpp"

namespace hy_agv
{
    // 串口数据结构体
    typedef struct imuData
    {
        std::chrono::steady_clock::time_point stamp;
        float time;
        int index_;
        float gAngle_;
        float gRate_;
        float accX_;
        float accY_;
        float accZ_;
    }imuData;

    class IMU_ALB:public serialPort
    {
    public:
        IMU_ALB(const std::string& serialPortName);
        // 重载函数 
        void setParam() override;
        void readThread() override;
        void writeThread() override;
        // 获取IMU数据接口
        imuData getImuData();
    private:
        // IMU 数据解码
        union u8_2_int16
        {
            int16_t i[5];
            uint8_t c[10];
        }U_transform{};
        imuData data_{};
        void decode();
        imuData updateData();
    };
}

#endif // __IMU_ALB_H
