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
  unsigned long lbmLastFoundUpperFreq;
  unsigned long lbmLastTurnon = 0;
  unsigned long lbmStartTimeOfCheckingFreq = 0;
  friend void InteractiveInterface::onButtonClicked(); // 需要更新 lbmStartTimeOfCheckingFreq

public:
  enum lbmState
  {
    DISABLED,
    TO_LEARN_MODELDATA, // 需要学习 ModelData
    LEARNING_MODELDATA,
    WAITING_RISING,
    WAITING_DROPPING,
    PREPARING_FOR_CHECKING_FREQ,
    CHECKING_FREQ_IN_DROPPING
  } lbmState = PREPARING_FOR_CHECKING_FREQ; // 默认上电，就因为 upper 是无效的 -1.0f，就会自动触发 ModelData 学习。这里不用考虑
  void begin();
  void update();

  bool relayState = true; // 上电默认开启
};

extern RelayControler relay_controler;