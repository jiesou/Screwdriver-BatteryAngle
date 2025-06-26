#include "UDPLogger.h"

void UDPLogger::begin()
{
    targetIP = IPAddress(192, 168, 4, 2);
    udp.begin(0);
}

void UDPLogger::println(const String &message)
{
    println(message.c_str());
}

void UDPLogger::println(const char *message)
{
    udp.beginPacket(targetIP, targetPort);
    udp.print(message);
    udp.print("\n");
    udp.endPacket();
}

void UDPLogger::print(const String &message)
{
    print(message.c_str());
}

void UDPLogger::print(const char *message)
{
    udp.beginPacket(targetIP, targetPort);
    udp.print(message);
    udp.endPacket();
}

void UDPLogger::printf(const char *format, ...)
{
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    print(buffer);
}

UDPLogger udpLogger;