#pragma once
#define STORED_PATH "/config.json"
#include <Arduino.h>

class StoredConfig {
private:
  /* data */
public:
  StoredConfig();

  bool save();
  bool load();

  String wifi_sta_ssid = "";
  String wifi_sta_password =  "";
  bool config_renewed = false;
  bool connStatus = false;
};

extern StoredConfig storedConfig;
