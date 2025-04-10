#include "RelayControler.h"
#include "StoredConfig.h"
#include <Arduino.h>

void RelayControler::begin() {
  pinMode(BUTTON_PIN, INPUT_PULLUP); // 内部上拉
  pinMode(LED_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(RELAY_PIN, HIGH); // 继电器默认打开
}

void RelayControler::update() {
  unsigned long currentMillis = millis();
  // 按钮去抖和切换逻辑，只在按钮刚按下时触发
  bool button_current = (digitalRead(BUTTON_PIN) == LOW);
  if (button_current && !stored_config.buttonPressed &&
      (currentMillis - last_button_press > 200)) {
    stored_config.relayState = !stored_config.relayState;
    applyHW();
    last_button_press = currentMillis;
  }
  stored_config.buttonPressed = button_current;

  // 如果没有设定定时，则直接根据 relay_state 更新硬件
  if (stored_config.relay_schedule_on == 0 &&
      stored_config.relay_schedule_off == 0) {
    applyHW();
    return;
  }

  // schedule_on 和 schedule_off 是秒数
  unsigned long cycleDuration =
      stored_config.relay_schedule_off + stored_config.relay_schedule_off;
  unsigned long elapsedSeconds = (currentMillis) / 1000;
  // millis 溢出后会从 0 重新计数。模运算在数值回绕后依然是有效的
  unsigned long positionInCycle = elapsedSeconds % cycleDuration;
  if (positionInCycle < stored_config.relay_schedule_on) {
    stored_config.relayState = true;
  } else {
    stored_config.relayState = false;
  }
  applyHW();
}

void RelayControler::applyHW() {
  if (stored_config.relayState) {
    digitalWrite(RELAY_PIN, HIGH);
    digitalWrite(LED_PIN, LOW); // 外部上拉
  } else {
    digitalWrite(RELAY_PIN, LOW);
    digitalWrite(LED_PIN, HIGH);
  }
}

RelayControler relay_controler;