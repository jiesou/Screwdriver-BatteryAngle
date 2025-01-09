#include "StoredConfig.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

bool StoredConfig::save() {
  File file = LittleFS.open(STORED_PATH, "w");
  if (!file) {
    Serial.println("Failed to open file for writing");
    return false;
  }

  // 更新 JsonDocument
  json["wifi_sta_ssid"] = wifi_sta_ssid;
  json["wifi_sta_password"] = wifi_sta_password;
  serializeJson(json, file);
  file.close();

  Serial.println("Config saved");
  return true;
}

JsonDocument StoredConfig::load() {
  if (!LittleFS.exists(STORED_PATH)) {
    Serial.println("No WiFi Config found, using default values");
    // return emtpy json
    return json;
  };
  File file = LittleFS.open(STORED_PATH, "r");
  if (!file) {
    Serial.println("Failed to open file for reading");
    return json;
  }

  DeserializationError error = deserializeJson(json, file);
  if (error) {
    Serial.println("Failed to parse JSON");
    return json;
  }
  wifi_sta_ssid = json["wifi_sta_ssid"].as<String>();
  wifi_sta_password = json["wifi_sta_password"].as<String>();
  file.close();

  Serial.println("Config loaded");
  return json;
}

StoredConfig::StoredConfig() {
  if (load() == true) {
    // 如果读出了ssid和password的配置，那就尝试连接
    config_renewed = true;
  }
}

StoredConfig stored_config;
