#pragma once
#include "DNSServer.h"
#include "ESPAsyncWebServer.h"

class CaptivePortal {
public:
  CaptivePortal();
  void begin();

  // 得到 status 更新
  void updateStatusChange(bool sta_connected, const String &ip, float frequency,
                          bool btn_pressed);

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
