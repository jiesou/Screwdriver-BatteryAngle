#include "CaptivePortal.h"
#include "LittleFS.h"
#include "ArduinoJson.h"
#include "StoredConfig.h"

CaptivePortal::CaptivePortal() 
  : server(80)
  , status_stream_events("/status") {}

void CaptivePortal::CaptiveRequestHandler::handleRequest(AsyncWebServerRequest *request) {
  request->send(LittleFS, "/index.html", "text/html");
}

void CaptivePortal::begin() {
  initSoftAP();
  handleRequests();
}

void CaptivePortal::updateStatusChange(bool sta_connected, const String& ip, 
                                   float frequency, bool btn_pressed) {
  // 检查有客户端连接
  if(status_stream_events.count() > 0) {
    JsonDocument doc;
    doc["sta_connected"] = sta_connected;
    doc["sta_error"] = !sta_connected; 
    doc["ip"] = ip;
    doc["frequency"] = frequency;
    doc["btn_pressed"] = btn_pressed;

    String output;
    serializeJson(doc, output);
    status_stream_events.send(output.c_str());
  }
}

void CaptivePortal::initSoftAP() {
  String apName = "BatteryAngle_" + String((uint32_t)ESP.getChipId(), HEX);
  WiFi.softAP(apName.c_str());
  server.serveStatic("/", LittleFS, "/");

  server.addHandler(new CaptiveRequestHandler())
      .setFilter([](AsyncWebServerRequest *request) {
        String url = request->url();
        return !(url.startsWith("/toggle") || url.startsWith("/set_ap") ||
                 url.startsWith("/status") || url.startsWith("/get_config"));
      });

  server.addHandler(&status_stream_events);
}

void CaptivePortal::handleRequests() {

  server.on("/set_wifi_ssid_and_passwd", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
      [](AsyncWebServerRequest *request, uint8_t *data, size_t len,
         size_t index, size_t total) {
        JsonDocument doc;
        deserializeJson(doc, data, len);
        String ssid = doc["wifi_sta_ssid"];
        String password = doc["wifi_sta_password"];
        Serial.printf("SSID: %s, Password: %s\n", ssid.c_str(), password.c_str());
        stored_config.wifi_sta_ssid = ssid;
        stored_config.wifi_sta_password = password;
        stored_config.config_renewed = true;

        JsonDocument response;
        response["message"] = "公共 WiFi 配置已更新";
        String responseBody;
        serializeJson(response, responseBody);
        request->send(200, "application/json", responseBody);
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
    File file = LittleFS.open("/wifi_config.json", "r");
    if (!file) {
      request->send(500, "application/json",
                    "{\"error\":\"Failed to open file\"}");
      return;
    }
    JsonDocument json;
    DeserializationError error = deserializeJson(json, file);
    file.close();
    if (error) {
      request->send(500, "application/json",
                    "{\"error\":\"Failed to parse JSON\"}");
      return;
    }
    String response;
    serializeJson(json, response);
    request->send(200, "application/json", response);
  });

  server.begin();
}
