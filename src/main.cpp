#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include "ArduinoJson.h"
#include "ArduinoOTA.h"
#include "CurrentProcessor.h"
#include "ESPAsyncWebServer.h"
#include "LittleFS.h" //我们把网页藏在LittleFS内
#include "index_html.h"
#include "ota/CaptivePortal.h"
#include "StoredConfig.h"
#include "ota/OTAHandler.h"

AsyncWebServer server(80);

StoredConfig stored_config;
CaptivePortal captivePortal;
OTAHandler otaHandler;
CurrentProcessor currentProcessor;


class CaptiveRequestHandler : public AsyncWebHandler {
public:
  CaptiveRequestHandler() {}
  virtual ~CaptiveRequestHandler() {}

  bool canHandle(AsyncWebServerRequest *request) {
    // request->addInterestingHeader("ANY");
    return true;
  }

  void handleRequest(AsyncWebServerRequest *request) {
    request->send(200, "text/html", index_html);
  }
};

// Initialize LittleFS
void initLittleFS() {
  if (!LittleFS.begin()) {
    Serial.println("An error has occurred while mounting LittleFS");
  }
  Serial.println("LittleFS mounted successfully");
}


// 删除原来的 initOTA() 函数

void onWiFiEvent(WiFiEvent_t event) {
  switch (event) {
  case WIFI_EVENT_STAMODE_GOT_IP:
    Serial.println("Ready");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    stored_config.connStatus = true;
    storedConfig.save();
    OTAHandler::begin(); // 改用 OTAHandler
    break;
  case WIFI_EVENT_STAMODE_DISCONNECTED:
    stored_config.connStatus = false;
    Serial.println("Disconnected from Wi-Fi.");
    break;
  default:
    break;
  }
}

void initSTA(String &ssid, String &password) {
  // WiFi.disconnect(true);  // 清除配置并断开连接
  // delay(100);             // 等待一些时间确保清除完成
  WiFi.onEvent(onWiFiEvent);
  WiFi.begin(ssid, password);
}

void setup() {
  Serial.begin(74880);
  initLittleFS();
  WiFi.mode(WIFI_AP_STA);
  captivePortal.begin();

  currentProcessor.begin();
  tone(13, 1000);
}

void loop() {
  captivePortal.update();
  
  if (stored_config.config_renewed == true) {
    stored_config.config_renewed = false;
    initSTA(stored_config.wifi_sta_ssid, stored_config.wifi_sta_password);
  }
  
  if (stored_config.connStatus == true) {
    otaHandler.update(); // 改用 OTAHandler
  }

  currentProcessor.update();

  static unsigned long last_pushupdate_time = 0;
  if (millis() - last_pushupdate_time > 100) {
    captivePortal.notifyStatusChange(
      stored_config.connStatus,
      WiFi.localIP().toString(),
      currentProcessor.frequency,
      currentProcessor.btn_pressed
    );
    last_pushupdate_time = millis();
  }
}
