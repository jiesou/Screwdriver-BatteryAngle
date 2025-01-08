#pragma once
#include "ESPAsyncWebServer.h"
#include "DNSServer.h"

class CaptivePortal {
public:
  CaptivePortal();
  void begin();
  
  // 得到 status 更新
  void updateStatusChange(bool sta_connected, const String& ip, 
                       float frequency, bool btn_pressed);
  
private:
  class CaptiveRequestHandler : public AsyncWebHandler {
  public:
    CaptiveRequestHandler() {}
    virtual ~CaptiveRequestHandler() {}
    // 处理所有请求
    bool canHandle(AsyncWebServerRequest *request) override { return true; }
    void handleRequest(AsyncWebServerRequest *request) override;
  };

  AsyncWebServer server;
  AsyncEventSource status_stream_events;
  
  void initSoftAP();
  void handleRequests();
};
