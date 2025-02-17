#pragma once
#define STORED_PATH "/config.json"
#include <Arduino.h>
#include <ArduinoJson.h>

class StoredConfig {
private:
  /* data */
public:
  StoredConfig();

  bool save();
  JsonDocument load();
  JsonDocument json;

  String wifi_sta_ssid = "";
  String wifi_sta_password =  "";

  bool staConfigRenewed = false;
  String staConnStatus = "--";
};

extern StoredConfig stored_config;
