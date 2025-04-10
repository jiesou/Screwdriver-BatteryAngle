#pragma once
#define RELAY_PIN 4
#define LED_PIN 2
#define BUTTON_PIN 0

#include "StoredConfig.h"

class RelayControler {
private:
  unsigned long last_button_press = 0;

public:
  void begin();
  void update();
  void applyHW();

  void setRelayState(bool state) { stored_config.relayState = state; }
  bool getRelayState() { return stored_config.relayState; }

  void setSchedule(unsigned int on, unsigned int off) {
    stored_config.relay_schedule_on = on;
    stored_config.relay_schedule_off = off;
  }
  int getScheduleOn() { return stored_config.relay_schedule_on; }
};

extern RelayControler relay_controler;