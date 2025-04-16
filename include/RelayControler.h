#pragma once
#define RELAY_PIN 4

#include "StoredConfig.h"

class RelayControler {
private:
  unsigned long last_button_press = 0;

public:
  void begin();
  void update();
  void applyHW();

  bool relayState = true; // 上电默认开启

  void setRelayState(bool state) { relayState = state; }
  bool getRelayState() { return relayState; }
};

extern RelayControler relay_controler;