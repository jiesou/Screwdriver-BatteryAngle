#pragma once
#include "DNSServer.h"
#include "ESPAsyncWebServer.h"

class CaptivePortal {
public:
  CaptivePortal();
  void begin();

  // 推送 status 更新
  void updateStatusChange();

  // 定时得到 status 的接口
  void update();

  // 记录上次执行 push 更新的时间
  unsigned long lastPushUpdateTime = 0;

private:
  class CaptiveRequestHandler : public AsyncWebHandler {
  public:
    CaptiveRequestHandler() {}
    virtual ~CaptiveRequestHandler() {}
    bool canHandle(AsyncWebServerRequest *request) {
      // request->addInterestingHeader("ANY");
      return true;
    }
    void handleRequest(AsyncWebServerRequest *request);
  };

  AsyncWebServer server;
  AsyncEventSource status_stream_events;

  void setupWebServer();
  void setupRequestHandlers();
};
