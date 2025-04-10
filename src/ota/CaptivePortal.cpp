#include "ota/CaptivePortal.h"
#include "ArduinoJson.h"
#include "CurrentProcessor.h"
#include "LittleFS.h"
#include "StoredConfig.h"

CaptivePortal::CaptivePortal() : server(80), status_stream_events("/status") {}

void CaptivePortal::CaptiveRequestHandler::handleRequest(
    AsyncWebServerRequest *request) {
  request->send(LittleFS, "/index.html", "text/html");
}

void CaptivePortal::begin() {
  setupWebServer();
  setupRequestHandlers();
}

void CaptivePortal::updateStatusChange() {
  unsigned long currentMillis = millis();
  // 每隔 300 毫秒推送一次 status
  // 小于 300 毫秒容易卡住 HTTP 流
  if (currentMillis - lastPushUpdateTime < 300) {
    return;
  }
  // 检查有客户端连接
  if (status_stream_events.count() < 1) {
    return;
  }

  JsonDocument doc;
  doc["sta_conn_status"] = stored_config.staConnStatus;
  doc["ip"] = WiFi.localIP().toString();
  doc["frequency"] = current_processor.frequency;
  doc["btn_pressed"] = stored_config.buttonPressed;
  doc["relay_state"] = stored_config.relayState;

  String output;
  serializeJson(doc, output);
  status_stream_events.send(output.c_str());
  lastPushUpdateTime = currentMillis;
}

void CaptivePortal::update() { updateStatusChange(); }

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
  server.on("/get_config", HTTP_GET, [](AsyncWebServerRequest *request) {
    JsonDocument config = stored_config.load();
    String output;
    serializeJson(config, output);
    request->send(200, "application/json", output.c_str());
  });
  
  server.on(
      "/set_config", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
      [](AsyncWebServerRequest *request, uint8_t *data, size_t len,
         size_t index, size_t total) {
        JsonDocument doc;
        deserializeJson(doc, data, len);
        stored_config.wifi_sta_ssid = doc["wifi_sta_ssid"].as<String>();
        stored_config.wifi_sta_password = doc["wifi_sta_password"].as<String>();
        stored_config.relay_schedule_on =
            doc["relay_schedule_on"].as<unsigned long>();
        stored_config.relay_schedule_off =
            doc["relay_schedule_off"].as<unsigned long>();
        stored_config.staConfigRenewed = true;

        request->send(200, "application/json",
                      "{\"message\":\"公共 WiFi 配置已更新\"}");
      });

  server.on("/fetch_nearby_wifi_ssids", HTTP_GET,
            [](AsyncWebServerRequest *request) {
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

  server.on(
      "/set_relay_switch", HTTP_POST, [](AsyncWebServerRequest *request) {},
      NULL,
      [](AsyncWebServerRequest *request, uint8_t *data, size_t len,
         size_t index, size_t total) {
        JsonDocument doc;
        deserializeJson(doc, data, len);
        bool relay_switch = doc["relay_switch"];
        Serial.println("Got new relay switch state: " + String(relay_switch));
        stored_config.relayState = (relay_switch == true); // 避免空值
        request->send(200, "application/json", "{\"message\":\"已更新\"}");
      });

  server.begin();
}
