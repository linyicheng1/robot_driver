#ifndef __SERIAL_PORT_H
#define __SERIAL_PORT_H

#include <vector>
#include <unistd.h>
#include <termios.h>
#include <cstring>
#include <fcntl.h>
#include <string>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <mutex>
#include <thread>

namespace hy_agv
{
// 串口数据读写
class serialPort
{
public:
    // 构造函数
    serialPort() = default;
    // 初始化串口
    bool init(const std::string& portName);
    // 取消初始化串口
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
    // 获取数据更新时间
    std::chrono::steady_clock::time_point update_time(){return updateTime_;}
protected:
    // 重载此函数以设定具体的串口参数
    virtual void setParam() = 0;
    virtual void readThread() = 0;
    virtual void writeThread() = 0;
    // 串口操作
    int spWrite(unsigned char *msg, int len);
    int spRead(unsigned char *buf, int begin, int max_len);

    // serial port
    boost::asio::serial_port *sp_{};
    boost::asio::io_service ioSev_;
    std::string portName_;
    boost::system::error_code err_;
    // thread 
    std::thread *writeThread_{};
    std::thread *readThread_{};
    std::mutex mutex_;
    // time 
    std::chrono::steady_clock::time_point updateTime_;
};
}

#endif // __SERIAL_PORT_H