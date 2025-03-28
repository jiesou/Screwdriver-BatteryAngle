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
#include "LittleFS.h" //我们把网页藏在 LittleFS 内（data 目录）
#include "StoredConfig.h"
#include "ota/CaptivePortal.h"
#include "ota/OTAHandler.h"
#include "ota/wifi/WiFiManager.h"

AsyncWebServer server(80);

WiFiManager wifiManager;
CaptivePortal captivePortal;
OTAHandler otaHandler;

void setup() {
  Serial.begin(74880);

  Serial.println("===Booting==");

  LittleFS.begin();
  Serial.println("===File system mounted===");
  stored_config.init();
  Serial.println("===Stored Config initialized===");
  wifiManager.startAP();
  Serial.println("===AP started===");
  otaHandler.begin();
  Serial.println("===OTA Service started===");
  captivePortal.begin();
  Serial.println("===Captive Portal started===");
  current_processor.begin();
  Serial.println("===Current Processor started===");

  wifiManager.onConnect([]() {
    Serial.println("Connected to WiFi");
    Serial.println("IP: " + wifiManager.getLocalIP());
    stored_config.staConnStatus = "已连接";
    stored_config.save();
    Serial.println("Connected to WiFi and saved");
  });
  wifiManager.onDisconnect([]() {
    stored_config.staConnStatus = "连接断开，将不会保存";
    stored_config.staConfigRenewed = false;
    Serial.println("Disconnected from WiFi");
  });
  wifiManager.onConnectionError([](const std::string &message) {
    stored_config.staConnStatus = String("连接错误 ") + message.c_str();
    stored_config.staConfigRenewed = false;
    Serial.println("WiFi connect error: " + String(message.c_str()));
  });
  pinMode(2, OUTPUT); // LED
  pinMode(4, OUTPUT);
}

void loop() {
  if (stored_config.staConfigRenewed) {
    Serial.println("===[Loop] WiFi config renewed===");
    Serial.println("Stored: SSID: " + stored_config.wifi_sta_ssid +
                   ", Password: " + stored_config.wifi_sta_password);
    stored_config.staConfigRenewed = false;
    stored_config.staConnStatus = "连接中";
    wifiManager.connectToWiFi(stored_config.wifi_sta_ssid,
                              stored_config.wifi_sta_password);
  }
  otaHandler.update();
  current_processor.update();
  captivePortal.update();

  if (stored_config.relay_state) {
    digitalWrite(4, HIGH);
    digitalWrite(2, LOW);
  } else {
    digitalWrite(4, LOW);
    digitalWrite(2, HIGH);
  }
  // static unsigned long lastToggleTime = 0;
  // static bool io10State = false;
  // unsigned long currentTime = millis();
  // if (currentTime - lastToggleTime >= 1000) {
  //   io10State = !io10State;
  //   digitalWrite(10, io10State ? HIGH : LOW);
  //   lastToggleTime = currentTime;
}
