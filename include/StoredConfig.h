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

  // WiFi相关信息
  String wifi_sta_ssid = "";
  String wifi_sta_password = "";
  unsigned long relay_schedule_on = 0;
  unsigned long relay_schedule_off = 0;
  
  bool relayState = true;
  bool buttonPressed = false;
  bool staConfigRenewed = false;
};

extern StoredConfig stored_config;