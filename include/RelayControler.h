#pragma once
#define RELAY_PIN 4

#include "StoredConfig.h"
#include "InteractiveInterface.h"

class RelayControler {
private:
  unsigned long last_button_press = 0;

  void applyHW();
  void updateSchedule();
  void updateLbmSmart();
  unsigned long lbmLastFoundUpperFerq;
  unsigned long lbmLastTurnon = 0;
  unsigned long lbmStartTimeOfCheckingFreq = 0;
  friend void InteractiveInterface::onButtonClicked(); // 需要更新 lbmStartTimeOfCheckingFreq

public:
  enum lbmState {
    WAITING_RISING,
    WAITING_DROPPING,
    PREPARING_FOR_CHECKING_FREQ,
    CHECKING_FREQ_IN_DROPPING
  } lbmState = WAITING_RISING;
  void begin();
  void update();

  bool relayState = true; // 上电默认开启
};

extern RelayControler relay_controler;