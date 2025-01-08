#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include "ArduinoJson.h"
#include "ArduinoOTA.h"
#include "ESPAsyncWebServer.h"
#include "LittleFS.h" //我们把网页藏在LittleFS内
#include "StoredConfig.h"
#include "ota/CaptivePortal.h"
#include "ota/OTAHandler.h"
#include "CurrentProcessor.h"

AsyncWebServer server(80);

CaptivePortal captivePortal;
OTAHandler otaHandler;
CurrentProcessor currentProcessor;

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
    stored_config.save();
    otaHandler.begin();
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
  
  if (stored_config.config_renewed == true) {
    stored_config.config_renewed = false;
    initSTA(stored_config.wifi_sta_ssid, stored_config.wifi_sta_password);
  }
  
  if (stored_config.connStatus == true) {
    otaHandler.update();
  }

  currentProcessor.update();

  static unsigned long last_pushupdate_time = 0;
  if (millis() - last_pushupdate_time > 100) {
    captivePortal.updateStatusChange(
      stored_config.connStatus,
      WiFi.localIP().toString(),
      currentProcessor.frequency,
      currentProcessor.btn_pressed
    );
    last_pushupdate_time = millis();
  }
}
