#ifdef ESP32
#include <AsyncTCP.h>
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include "ArduinoJson.h"
#include "ArduinoOTA.h"
#include "ESPAsyncWebServer.h"
#include "LittleFS.h" //我们把网页藏在 LittleFS 内（data 目录）

#include "StoredConfig.h"
#include "CurrentProcessor.h"
#include "RelayControler.h"
#include "InteractiveInterface.h"
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
  relay_controler.begin();
  Serial.println("===Relay Controler started===");
  interactive_interface.begin();
  Serial.println("===Interactive Interface started===");

  wifiManager.onConnect([]() {
    Serial.println("Connected to WiFi");
    Serial.println("IP: " + wifiManager.getLocalIP());
    
    stored_config.save();
    Serial.println("Connected to WiFi and saved");
  });
  wifiManager.onDisconnect([]() {
    stored_config.staConfigRenewed = false;
    Serial.println("Disconnected from WiFi");
  });
}

void loop() {
  // if (stored_config.wifi_sta_ssid.length() < 1 && WiFi.isConnected()) {
  //   // 如果没有配置 WiFi 信息，且当前连接了 WiFi，断开连接
  //   Serial.println("===[Loop] No WiFi config, disconnecting from WiFi===");
  //   WiFi.disconnect(true);
  // }
  
  // if (stored_config.staConfigRenewed) {
  //   // 如果配置了新的 WiFi 信息，尝试连接
  //   Serial.println("===[Loop] WiFi config renewed===");
  //   Serial.println("Stored: SSID: " + stored_config.wifi_sta_ssid +
  //                  ", Password: " + stored_config.wifi_sta_password);
  //   stored_config.staConfigRenewed = false;
  //   wifiManager.connectToWiFi(stored_config.wifi_sta_ssid,
  //                             stored_config.wifi_sta_password);
  // }
  otaHandler.update();
  captivePortal.update();
  current_processor.update();
  relay_controler.update();
  interactive_interface.update();
}
