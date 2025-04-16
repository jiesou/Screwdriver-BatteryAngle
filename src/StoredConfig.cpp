#include "StoredConfig.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

bool StoredConfig::save() {
  File file = LittleFS.open(STORED_PATH, "w");
  if (!file) {
    Serial.println("[StoredConfig] Failed to open file for writing");
    return false;
  }

  // 更新 JsonDocument
  json["wifi_sta_ssid"] = wifi_sta_ssid;
  json["wifi_sta_password"] = wifi_sta_password;
  json["relay_schedule_on"] = relay_schedule_on;
  json["relay_schedule_off"] = relay_schedule_off;
  serializeJson(json, file);
  file.close();

  Serial.println("[StoredConfig] Config saved");
  return true;
}

JsonDocument StoredConfig::load() {
  if (!LittleFS.exists(STORED_PATH)) {
    Serial.println(
        "[StoredConfig] No Config file found, return current(empty) json");
    // return current(empty) json
    return json;
  }
  File file = LittleFS.open(STORED_PATH, "r");
  if (!file) {
    Serial.println("[StoredConfig] Failed to open config file for reading");
    return json;
  }

  DeserializationError error = deserializeJson(json, file);
  if (error) {
    Serial.println("[StoredConfig] Failed to parse JSON");
    return json;
  }
  wifi_sta_ssid = json["wifi_sta_ssid"].as<String>();
  wifi_sta_password = json["wifi_sta_password"].as<String>();
  relay_schedule_on = json["relay_schedule_on"].as<unsigned long>();
  relay_schedule_off = json["relay_schedule_off"].as<unsigned long>();
  file.close();

  Serial.println("[StoredConfig] Config loaded");
  return json;
}

void StoredConfig::init() {
  Serial.println("[StoredConfig] init");
  load();
  if (wifi_sta_ssid.length() > 0 && wifi_sta_password.length() > 0) {
    // 如果读出了 ssid 和 password 的配置，那就可以尝试连接
    staConfigRenewed = true;
  }
}

StoredConfig stored_config;