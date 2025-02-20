#include "CaptivePortal.h"
#include "ArduinoJson.h"
#include "LittleFS.h"
#include "StoredConfig.h"
#include "CurrentProcessor.h"

CaptivePortal::CaptivePortal() : server(80), status_stream_events("/status") {}

void CaptivePortal::CaptiveRequestHandler::handleRequest(
    AsyncWebServerRequest *request) {
  request->send(LittleFS, "/index.html", "text/html");
}

void CaptivePortal::begin() {
  setupWebServer();
  setupRequestHandlers();
}

void CaptivePortal::updateStatusChange(String sta_conn_status, const String &ip,
                                       float frequency, bool btn_pressed) {
  // 检查有客户端连接
  if (status_stream_events.count() > 0) {
    JsonDocument doc;
    doc["sta_conn_status"] = sta_conn_status;
    doc["ip"] = ip;
    doc["frequency"] = frequency;
    doc["btn_pressed"] = btn_pressed;

    String output;
    serializeJson(doc, output);
    status_stream_events.send(output.c_str());
  }
}

//  每隔 300 毫秒推送一次 status
void CaptivePortal::update() {
  unsigned long currentMillis = millis();
  if (currentMillis - lastPushUpdateTime > 300) {
    updateStatusChange(stored_config.staConnStatus, WiFi.localIP().toString(),
                       current_processor.frequency,
                       current_processor.btn_pressed);
    lastPushUpdateTime = currentMillis;
  }
}

void CaptivePortal::setupWebServer() {
  server.serveStatic("/", LittleFS, "/");
  server.addHandler(new CaptiveRequestHandler())
      .setFilter([](AsyncWebServerRequest *request) {
        String url = request->url();
        return url == "/" || url == "/index.html";
      });
  server.addHandler(&status_stream_events);
  server.begin();
}

void CaptivePortal::setupRequestHandlers() {
  server.on(
      "/set_wifi_ssid_and_passwd", HTTP_POST,
      [](AsyncWebServerRequest *request) {}, NULL,
      [](AsyncWebServerRequest *request, uint8_t *data, size_t len,
         size_t index, size_t total) {
        JsonDocument doc;
        deserializeJson(doc, data, len);
        String ssid = doc["wifi_sta_ssid"];
        String password = doc["wifi_sta_password"];
        Serial.printf("Got new SSID: %s, Password: %s\n", ssid.c_str(),
                      password.c_str());
        stored_config.wifi_sta_ssid = ssid;
        stored_config.wifi_sta_password = password;
        stored_config.staConfigRenewed = true;

        request->send(200, "application/json",
                      "{\"message\":\"公共 WiFi 配置已更新\"}");
      });
    
    server.on("/fetch_nearby_wifi_ssids", HTTP_GET, [](AsyncWebServerRequest *request) {
      Serial.println("Fetching nearby WiFi SSIDs");
      // 保存请求对象以在回调中使用
      static AsyncWebServerRequest *storedRequest = nullptr;
      storedRequest = request;

      // 启动异步扫描
      WiFi.scanNetworksAsync([](int availableSsidCount) {
        if (!storedRequest)
          return;

        // 限制返回数量为 8 条
        if (availableSsidCount > 8)
          availableSsidCount = 8;

        JsonDocument doc;
        JsonArray array = doc.to<JsonArray>();

        for (int i = 0; i < availableSsidCount; i++) {
          array.add(WiFi.SSID(i));
        }

        String output;
        serializeJson(doc, output);
        storedRequest->send(200, "application/json", output.c_str());
        storedRequest = nullptr;

        // 清理扫描结果
        WiFi.scanDelete();
      });
    });

  server.on("/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
    status_stream_events.onConnect([](AsyncEventSourceClient *client) {
      if (client->lastId()) {
        Serial.printf("Client reconnected! Last message ID: %u\n",
                      client->lastId());
      }
    });
    request->send(200, "text/event-stream", "data: {}");
  });

  server.on("/get_config", HTTP_GET, [](AsyncWebServerRequest *request) {
    JsonDocument config = stored_config.load();
    String output;
    serializeJson(config, output);
    request->send(200, "application/json", output.c_str());
  });

  server.begin();
}
