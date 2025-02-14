#ifdef ESP32
#include <AsyncTCP.h>
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include "ArduinoJson.h"
#include "ArduinoOTA.h"
#include "CurrentProcessor.h"
#include "ESPAsyncWebServer.h"
#include "LittleFS.h" //我们把网页藏在LittleFS内
#include "StoredConfig.h"
#include "ota/CaptivePortal.h"
#include "ota/OTAHandler.h"
#include "ota/wifi/WiFiManager.h"

AsyncWebServer server(80);

WiFiManager wifiManager;
CaptivePortal captivePortal;
OTAHandler otaHandler;
CurrentProcessor currentProcessor;

void setup() {
  Serial.begin(74880);

  Serial.println("===Booting==");

  LittleFS.begin();
  Serial.println("===File system mounted===");
  wifiManager.startAP();
  Serial.println("===AP started===");
  otaHandler.begin();
  Serial.println("===OTA Service started===");
  captivePortal.begin();
  Serial.println("===Captive Portal started===");
  currentProcessor.begin();
  Serial.println("===Current Processor started===");

  wifiManager.onConnect([]() {
    Serial.println("Connected to WiFi");
    Serial.println("IP: " + wifiManager.getLocalIP());
    stored_config.connStatus = true;
    stored_config.save();
  });
  wifiManager.onDisconnect([]() {
    stored_config.connStatus = false;
    stored_config.config_renewed = false;
    Serial.println("Disconnected from WiFi");
  });
  tone(13, 1000);
}

void loop() {
  if (stored_config.config_renewed) {
    stored_config.config_renewed = false;
    wifiManager.connectToWiFi(stored_config.wifi_sta_ssid,
                              stored_config.wifi_sta_password);
  }
  Serial.println("===[Loop] WiFi config checked===");
  otaHandler.update();
  Serial.println("===[Loop] OTA Service updated===");
  currentProcessor.update();
  Serial.println("===[Loop] Current Processor updated===");
  static unsigned long last_pushupdate_time = 0;
  if (millis() - last_pushupdate_time > 100) {
    captivePortal.updateStatusChange(
        stored_config.connStatus, WiFi.localIP().toString(),
        currentProcessor.frequency, currentProcessor.btn_pressed);
    last_pushupdate_time = millis();
    Serial.println("===[Loop] Captive Portal status api updated===");
  }
}
