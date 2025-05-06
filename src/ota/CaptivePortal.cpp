#include "ota/CaptivePortal.h"
#include <ArduinoJson.h>
#include <AsyncJson.h>
#include <LittleFS.h>
#include "StoredConfig.h"
#include "CurrentProcessor.h"
#include "RelayControler.h"
#include "InteractiveInterface.h"

CaptivePortal::CaptivePortal()
    : server(80), status_stream_events("/api/status") {}

void CaptivePortal::CaptiveRequestHandler::handleRequest(
    AsyncWebServerRequest *request)
{
  request->send(LittleFS, "/index.html", "text/html");
}

void CaptivePortal::begin()
{
  setupWebServer();
  setupRequestHandlers();
  dnsServer.start(53, "*", WiFi.softAPIP());
  server.begin();
}

void CaptivePortal::updateStatusChange()
{
  unsigned long currentMillis = millis();
  // 每隔 300 毫秒推送一次 status
  // 小于 300 毫秒容易卡住 HTTP 流
  if (currentMillis - lastPushUpdateTime < 300)
    return;
  lastPushUpdateTime = currentMillis;
  // 检查有客户端连接
  if (status_stream_events.count() < 1)
    return;

  JsonDocument doc;
  doc["millis"] = currentMillis;
  doc["sta_conn_status"] = WiFi.status();
  doc["ip"] = WiFi.localIP().toString();
  doc["frequency"] = current_processor.frequency;
  doc["frequency_overall"] = current_processor.frequency_overall;
  doc["btn_pressed"] = interactive_interface.button_pressed_down;
  doc["relay_state"] = relay_controler.relayState;
  doc["lbm_smart_info"] = relay_controler.lbmState;

  String output;
  serializeJson(doc, output);
  status_stream_events.send(output.c_str());
}

void CaptivePortal::update()
{
  dnsServer.processNextRequest();
  updateStatusChange();
}

void CaptivePortal::setupWebServer()
{
  server.serveStatic("/", LittleFS, "/");
  server.addHandler(new CaptiveRequestHandler())
      .setFilter([](AsyncWebServerRequest *request)
                 {
        String url = request->url();
        // 排除API和已知静态资源请求
        if (url.startsWith("/api/"))
          return false;
        // 处理所有其他请求，包括浏览器尝试访问的外部网站
        return true; });
  server.addHandler(&status_stream_events);
}

void CaptivePortal::setupRequestHandlers()
{
  server.on("/api/get_config", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    JsonDocument config = stored_config.load();
    String output;
    serializeJson(config, output);
    request->send(200, "application/json", output.c_str()); });

  AsyncCallbackJsonWebHandler *configHandler = new AsyncCallbackJsonWebHandler(
      "/api/set_config",
      [](AsyncWebServerRequest *request, JsonVariant &json)
      {

        JsonObject jsonObj = json.as<JsonObject>();

        stored_config.wifi_sta_ssid = jsonObj["wifi_sta_ssid"].as<String>();
        stored_config.wifi_sta_password = jsonObj["wifi_sta_password"].as<String>();
        stored_config.relay_schedule_on = jsonObj["relay_schedule_on"].as<unsigned long>();
        stored_config.relay_schedule_off = jsonObj["relay_schedule_off"].as<unsigned long>();
        stored_config.lbm_smart_enabled = jsonObj["lbm_smart_enabled"].as<bool>();
        stored_config.lbm_smart_upper_ferq = jsonObj["lbm_smart_upper_ferq"].as<float>();
        stored_config.lbm_smart_lower_time = jsonObj["lbm_smart_lower_time"].as<unsigned long>();
        stored_config.staConfigRenewed = true;

        Serial.println(stored_config.lbm_smart_enabled);

        stored_config.save();

        request->send(200, "application/json", "{\"message\":\"配置已更新\"}");
      });
  server.addHandler(configHandler);

  server.on("/api/fetch_nearby_wifi_ssids", HTTP_GET,
            [](AsyncWebServerRequest *request)
            {
              Serial.println("Fetching nearby WiFi SSIDs");
              // 保存请求对象以在回调中使用
              static AsyncWebServerRequest *storedRequest = nullptr;
              storedRequest = request;

              // 启动异步扫描
              WiFi.scanNetworksAsync([](int availableSsidCount)
                                     {
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
                WiFi.scanDelete(); });
            });

  server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest *request)
            {
    status_stream_events.onConnect([](AsyncEventSourceClient *client) {
      if (client->lastId()) {
        Serial.printf("Client reconnected! Last message ID: %u\n",
                      client->lastId());
      }
    });
    request->send(200, "text/event-stream", "data: {}"); });

  server.on(
      "/api/set_relay_switch", HTTP_POST, [](AsyncWebServerRequest *request) {},
      NULL,
      [](AsyncWebServerRequest *request, uint8_t *data, size_t len,
         size_t index, size_t total)
      {
        JsonDocument doc;
        deserializeJson(doc, data, len);
        bool relay_switch = doc["relay_switch"];
        Serial.println("Got new relay switch state: " + String(relay_switch));
        relay_controler.relayState = (relay_switch == true); // 避免空值
        request->send(200, "application/json", "{\"message\":\"已更新\"}");
      });
}
