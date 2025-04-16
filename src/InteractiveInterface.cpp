#include "InteractiveInterface.h"
#include "RelayControler.h"
#include "StoredConfig.h"
#include <Arduino.h>
#include <LittleFS.h>

void InteractiveInterface::begin() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  digitalWrite(LED_PIN, LOW); // LED 默认开启
}

void InteractiveInterface::update() {
  // 更新 LED 状态
  digitalWrite(LED_PIN, LOW ? led_state : HIGH); // 外部上拉
  
  unsigned long currentMillis = millis();
  // 按钮去抖和切换逻辑，只在按钮刚按下/刚抬起时触发
  bool current_button = (digitalRead(BUTTON_PIN) == LOW);

  if (current_button && !button_pressed &&
      (currentMillis - last_button_press > 200)) {

    // 如果正处于周期控制状态，则闪烁提示灯
    if (stored_config.relay_schedule_on != 0 && stored_config.relay_schedule_off != 0) {
      led_blink_async();
    }
    relay_controler.relayState = !relay_controler.relayState; // 切换继电器状态
    button_pressed = true;
  }
  if (!current_button && button_pressed) {
    button_pressed = false;
    last_button_press = currentMillis;
  }



  // 长按按钮 10 秒，重置全部
  if (current_button && (currentMillis - last_button_press > 10000)) {
    Serial.println("Long press detected, resetting BatteryAngle...");
    LittleFS.format(); // 重置 littlefs
    ESP.restart(); // 重新启动 ESP8266
  }
}

InteractiveInterface interactive_interface;