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
  if (led_blinking) whenLedBlinking();
  digitalWrite(LED_PIN, led_state ? LOW : HIGH); // 外部上拉

  unsigned long currentMillis = millis();
  // 按钮去抖和切换逻辑，只在按钮刚按下/刚抬起时触发
  bool current_button = (digitalRead(BUTTON_PIN) == LOW);

  if (current_button && !button_pressed_down) {
    button_pressed_down = true;
    last_button_press = currentMillis;
  }
  if (!current_button && button_pressed_down &&
      (currentMillis - last_button_press > 200)) {

    onButtonClicked(); // 按钮松开触发 click

    button_pressed_down = false;
    last_button_release = currentMillis;
  }

  // 长按按钮 5 秒，重置全部
  if (current_button && (currentMillis - last_button_press > 5000)) {
    digitalWrite(LED_PIN, led_state ? HIGH : LOW); // 反向亮灭，提示已重置
    delay(1000);

    Serial.println("Long press detected, resetting BatteryAngle...");
    LittleFS.format(); // 重置 littlefs
    ESP.restart();     // 重新启动 ESP8266
  }
}

void InteractiveInterface::onButtonClicked() {
  // 如果正处于周期控制状态，则闪烁提示灯
  if (stored_config.relay_schedule_on != 0 &&
      stored_config.relay_schedule_off != 0) {
    led_blink_async();
  }
  relay_controler.relayState = !relay_controler.relayState; // 切换继电器状态
}

void InteractiveInterface::led_blink_async() {
  if (led_blinking) return;
  led_blinking = true;
  led_blink_start = millis();
  led_blink_start_state = led_state;
}

void InteractiveInterface::whenLedBlinking() {
  unsigned long currentMillis = millis();
  unsigned long elapsedMillis = currentMillis - led_blink_start;
  if (elapsedMillis >= 1000) {
    led_blinking = false;
    led_state = led_blink_start_state; // 恢复原状态
    return;
  }
  // LED 闪烁逻辑
  if (elapsedMillis % 200 < 100) {
    led_state = !led_blink_start_state;
  } else {
    led_state = led_blink_start_state;
  }
}

InteractiveInterface interactive_interface;