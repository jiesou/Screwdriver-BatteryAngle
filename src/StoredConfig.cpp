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

  JsonDocument doc;
  // 更新 JsonDocument
  doc["wifi_sta_ssid"] = wifi_sta_ssid;
  doc["wifi_sta_password"] = wifi_sta_password;
  serializeJson(doc, file);
  file.close();

  Serial.println("Config saved");
  return true;
}

bool StoredConfig::load() {
  if (!LittleFS.exists(STORED_PATH)) {
    Serial.println("No WiFi Config found, using default values");
    return false;
  };
  File file = LittleFS.open(STORED_PATH, "r");
  if (!file) {
    Serial.println("Failed to open file for reading");
    return false;
  }

  // 从 JsonDocument 中读取
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    Serial.println("Failed to parse JSON");
    return false;
  }
  wifi_sta_ssid = doc["wifi_sta_ssid"].as<String>();
  wifi_sta_password = doc["wifi_sta_password"].as<String>();
  file.close();

  Serial.println("Config loaded");
  return true;
}

StoredConfig::StoredConfig() {
  if (load() == true) {
    // 如果读出了ssid和password的配置，那就尝试连接
    config_renewed = true;
  }
}

StoredConfig stored_config;
