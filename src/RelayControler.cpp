#include <Arduino.h>
#include "RelayControler.h"
#include "StoredConfig.h"
#include "InteractiveInterface.h"

void RelayControler::begin() {
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // 继电器默认打开
}

void RelayControler::update() {
  // 如果没有设定定时，则直接根据 relay_state 更新硬件
  if (stored_config.relay_schedule_on == 0 ||
      stored_config.relay_schedule_off == 0) {
    applyHW();
    return;
  }

  // relay_schedule_on 和 relay_schedule_off 是秒数
  unsigned long cycleDuration =
      stored_config.relay_schedule_off + stored_config.relay_schedule_off;
  unsigned long elapsedSeconds = (millis()) / 1000;
  // millis 溢出后会从 0 重新计数。模运算在数值回绕后依然是有效的
  unsigned long positionInCycle = elapsedSeconds % cycleDuration;
  if (positionInCycle < stored_config.relay_schedule_on) {
    relayState = true;
  } else {
    relayState = false;
  }
  applyHW();
}

void RelayControler::applyHW() {
  if (relayState) {
    digitalWrite(RELAY_PIN, HIGH);
    interactive_interface.led_state = true;
  } else {
    digitalWrite(RELAY_PIN, LOW);
    interactive_interface.led_state = false;
  }
}

RelayControler relay_controler;