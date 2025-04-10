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

  bool relay_state = true;
  unsigned long relay_schedule_on = 0;
  unsigned long relay_schedule_off = 0;
  bool button_pressed = false;

  bool staConfigRenewed = false;
  String staConnStatus = "--";
};

extern StoredConfig stored_config;