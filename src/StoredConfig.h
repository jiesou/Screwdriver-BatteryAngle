#pragma once
#define STORED_PATH "/config.json"
#include <Arduino.h>
#include <ArduinoJson.h>

class StoredConfig {
private:
  /* data */
public:
  void init();
  bool save();
  JsonDocument load();
  JsonDocument json;

  // 持久状态
  String wifi_sta_ssid = "";
  String wifi_sta_password =  "";
  bool relay_state = false;

  bool staConfigRenewed = false;
  String staConnStatus = "--";
};

extern StoredConfig stored_config;
