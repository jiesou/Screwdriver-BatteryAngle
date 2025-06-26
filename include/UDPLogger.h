#pragma once
#include <WiFiUdp.h>

class UDPLogger
{
private:
    WiFiUDP udp;
    IPAddress targetIP;
    uint16_t targetPort = 12345; // 服务端端口

public:
    void begin();
    // 日志输出方法
    void println(const String &message);
    void println(const char *message);
    void print(const String &message);
    void print(const char *message);
    // 格式化输出
    void printf(const char *format, ...);
};

extern UDPLogger udpLogger;