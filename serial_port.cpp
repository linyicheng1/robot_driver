#include "serial_port.hpp"
#include <iostream>

using namespace boost::asio;

namespace hy_agv
{
    // 初始化串口，尝试最多失败5次
    bool serialPort::init(const std::string& portName)
    {
        portName_ = portName;
        static int num = 0;
        try
        {
            sp_ = new serial_port(ioSev_,portName_);
            setParam();
            num = 0;
        }
        catch(...)
        {
            num ++;
            std::cerr<< "Exception Error: " << err_.message() << std::endl;
            if(num < 5)
            {
                return init(portName);
            }
            else
            {
                return false;
            }
        }
        return true;
    }
    
    void serialPort::deInit()
    {
        delete sp_;
    }

    // 串口重新初始化
    void serialPort::reInit()
    {
        // 停止读取
        stopRead();
        startWrite();
        // 重新初始化
        deInit();
        init(portName_);
    }

    void serialPort::startRead()
    {
        readThread_ = new std::thread(&serialPort::readThread, std::ref(*this));
    }

    void serialPort::stopRead()
    {
        delete readThread_;
    }

    void serialPort::startWrite()
    {
        writeThread_ = new std::thread(&serialPort::writeThread, std::ref(*this));
    }

    void serialPort::stopWrite()
    {
        delete writeThread_;
    }

    int serialPort::spWrite(unsigned char *msg, int len)
    {
        try
        {
            write(*sp_,buffer(msg,len));
        }
        catch(...)
        {
            std::cerr<< "Exception Error: " <<err_.message() << std::endl;
        }
        return 1;
    }

    int serialPort::spRead(unsigned char *buf, int begin, int max_len)
    {
        try
        {
            read(*sp_, buffer(buf+begin,max_len));
        }
        catch(...)
        {
            std::cerr<< "Exception Error: " <<err_.message() << std::endl;
        }
        updateTime_ = std::chrono::steady_clock::now();
        return 1;
    }
}


