#pragma once
#include <ESP8266WiFi.h>
#include <functional>

class WiFiManager {
public:
    WiFiManager();
    
    // AP 模式相关
    void startAP();
    
    // STA 模式相关 
    void connectToWiFi(const String& ssid, const String& password);
    bool isConnected() const;
    String getLocalIP() const;
    
    // 事件回调
    using ConnectionCallback = std::function<void()>;
    using DisconnectionCallback = std::function<void()>;
    using ConnectionErrorCallback = std::function<void(const std::string &)>;

    void onConnect(ConnectionCallback cb) { _onConnect = cb; }
    void onDisconnect(DisconnectionCallback cb) { _onDisconnect = cb; }
    void onConnectionError(ConnectionErrorCallback cb) { _onConnectionError = cb; }

private:
    static void onWiFiEvent(WiFiEvent_t event);
    static WiFiManager *instance; // 静态实例指针

    ConnectionCallback _onConnect;
    DisconnectionCallback _onDisconnect;
    ConnectionErrorCallback _onConnectionError;
    bool _isConnected;
};