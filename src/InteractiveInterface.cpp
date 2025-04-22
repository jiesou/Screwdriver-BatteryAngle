#include "InteractiveInterface.h"
#include "CurrentProcessor.h"
#include "RelayControler.h"
#include "StoredConfig.h"
#include <Arduino.h>
#include <LittleFS.h>

void InteractiveInterface::begin() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
}

void InteractiveInterface::update() {
  updateLedState();
  updateButtonState();
}

void InteractiveInterface::updateLedState() {
  unsigned long currentMillis = millis();

  // 确定 LED 的基本状态为继电器开关 (仅用于普通模式)
  led_state = relay_controler.relayState;

  if (stored_config.lbm_smart_enabled) {
    switch (relay_controler.lbmState) {
    case RelayControler::lbmState::WAITING_RISING:
      // LBM 充电中
      if (breathing_state == BreathingState::DISABLED)
        breathing_state = BreathingState::FADE_IN;
      breathing_mode_on = true; // 常亮呼吸
      break;
    case RelayControler::lbmState::WAITING_DROPPING:
      // LBM 耗电中
      if (breathing_state == BreathingState::DISABLED)
        breathing_state = BreathingState::FADE_IN;
      breathing_mode_on = false; // 常灭呼吸
      break;
    default:
      // 其他状态下 LED 快闪（检查频率中）
      breathing_state = BreathingState::DISABLED;
      led_state = millis() % 200 < 100;
      break;
    }
  } else {
    breathing_state = BreathingState::DISABLED;
  }

  // 每次 update 都要确定 LED 运作模式
  if (led_blinking_duration > 0) {
    // 闪灯模式优先级最高
    led_mode = LedMode::BLINKING;
  } else if (breathing_state != BreathingState::DISABLED) {
    // 其次是呼吸灯模式
    led_mode = LedMode::BREATHING;
  } else {
    led_mode = LedMode::NORMAL;
  }

  switch (led_mode) {
  case LedMode::BLINKING:
    handleLedBlinking(currentMillis);
    break;
  case LedMode::BREATHING:
    handleLedBreathing(currentMillis);
    break;
  case LedMode::NORMAL:
  default:
    digitalWrite(LED_PIN, led_state ? LOW : HIGH); // 注意外部上拉
    break;
  }
}

void InteractiveInterface::handleLedBlinking(unsigned long currentMillis) {
  unsigned long elapsedMillis = currentMillis - led_blink_start;

  if (elapsedMillis >= led_blinking_duration) {
    // 闪烁结束
    led_blinking_duration = 0;
    led_state = led_blink_start_state;
    digitalWrite(LED_PIN, led_state ? LOW : HIGH);
    return;
  }

  // 闪烁逻辑
  bool blinkState;
  if (elapsedMillis % 200 < 100) {
    blinkState = !led_blink_start_state;
  } else {
    blinkState = led_blink_start_state;
  }

  digitalWrite(LED_PIN, blinkState ? LOW : HIGH);
}

void InteractiveInterface::handleLedBreathing(unsigned long currentMillis) {
  if (currentMillis - last_breathing_update < 20)
    return; // 20ms 更新一次呼吸灯
  last_breathing_update = currentMillis;

  switch (breathing_state) {
  case BreathingState::FADE_IN:
    // 亮度增加阶段
    breathing_brightness += 3;
    if (breathing_brightness >= 100) {
      breathing_brightness = 100;
      last_breathing_reversed = currentMillis;
      if (breathing_mode_on) {
        // 常灭模式：直接进入亮度减少阶段
        breathing_state = BreathingState::FADE_OUT;
      } else {
        // 常亮模式：保持最大亮度 breathing_interval
        breathing_state = BreathingState::HOLD_MAX;
      }
    }
    break;

  case BreathingState::FADE_OUT:
    // 亮度减少阶段
    breathing_brightness -= 3;
    if (breathing_brightness <= 0) {
      breathing_brightness = 0;
      last_breathing_reversed = currentMillis;

      if (breathing_mode_on) {
        breathing_state = BreathingState::HOLD_MIN;
      } else {
        breathing_state = BreathingState::FADE_IN;
      }
    }
    break;

  case BreathingState::HOLD_MAX:
    // 保持最大亮度状态
    if (currentMillis - last_breathing_reversed >= BREATHING_HOLDING_INTERVAL) {
      breathing_state = BreathingState::FADE_OUT;
    }
    break;

  case BreathingState::HOLD_MIN:
    // 保持最小亮度状态
    if (currentMillis - last_breathing_reversed >= BREATHING_HOLDING_INTERVAL) {
      breathing_state = BreathingState::FADE_IN;
    }
    break;

  case BreathingState::DISABLED:
    break;
  }

  analogWrite(LED_PIN, map(breathing_brightness, 0, 100, 0, 511));
}

void InteractiveInterface::updateButtonState() {
  unsigned long currentMillis = millis();
  bool current_button = (digitalRead(BUTTON_PIN) == LOW);

  switch (button_state) {
  case ButtonState::IDLE:
    if (current_button) { // 从空闲状态检测到按钮按下
      last_button_press = currentMillis;
      button_state = ButtonState::PRESSED;
      button_pressed_down = true;
    }
    break;
  case ButtonState::PRESSED:
    if (current_button &&
        (currentMillis - last_button_press > LONG_PRESS_DELAY)) {
      // 检测到长按
      onButtonLongPressed();
      button_state = ButtonState::IDLE;
      button_pressed_down = false;
      break;
    } else if (current_button &&
               (currentMillis - last_button_press > SHORT_LONG_PRESS_DELAY) &&
               (currentMillis - last_button_press < LONG_PRESS_DELAY)) {
      // 检测到短长按 (处于2秒和5秒之间)
      onButtonShortLongPressed();
      button_pressed_down = false;
      // 这个时候依然要继续检测按住的时间，不能直接进入 IDLE
      break;
    } else if (!current_button &&
               (currentMillis - last_button_press > DEBOUNCE_DELAY)) {
      // 按钮马上释放，进入等待状态以检测可能的双击
      last_button_release = currentMillis;
      button_state = ButtonState::RELEASED_WAIT;
      button_pressed_down = false;
    }
    break;

  case ButtonState::RELEASED_WAIT:
    if (current_button &&
        (currentMillis - last_button_release < DOUBLE_CLICK_DELAY)) {
      // 在双击时间窗口内检测到第二次按下
      button_state = ButtonState::DOUBLE_PRESSED;
      button_pressed_down = true;
    } else if (currentMillis - last_button_release >= DOUBLE_CLICK_DELAY) {
      // 超过双击时间窗口，判定为单击
      onButtonClicked();
      button_state = ButtonState::IDLE;
    }
    break;

  case ButtonState::DOUBLE_PRESSED:
    if (!current_button &&
        (currentMillis - last_button_press > DEBOUNCE_DELAY)) {
      // 第二次按下后释放，确认为双击
      onButtonDoubleClicked();
      button_state = ButtonState::IDLE;
      button_pressed_down = false;
    }
    break;
  }
}

void InteractiveInterface::onButtonClicked() {
  if (stored_config.relay_schedule_on != 0 &&
      stored_config.relay_schedule_off != 0) {
    // 如果正处于周期控制状态，则闪烁提示灯
    led_blink_async(1500);
  } else if (stored_config.lbm_smart_enabled) {
    // 如果正处于 LBM 智能控制状态，则无论如何切换到充电状态
    relay_controler.relayState = true; // 切换到充电状态
    relay_controler.lbmState = RelayControler::lbmState::WAITING_RISING;
  } else {
    // 默认状态切换继电器状态
    relay_controler.relayState = !relay_controler.relayState;
  }
}

void InteractiveInterface::onButtonDoubleClicked() {
  led_blink_async(400); // 快速闪烁提示灯

  // 双击功能: 切换智能 LBM 模式
  stored_config.lbm_smart_enabled = !stored_config.lbm_smart_enabled;
  // 保存配置
  stored_config.save();
}

void InteractiveInterface::onButtonShortLongPressed() {
  // 短按长按功能: 设置继电器整定电流
  if (relay_controler.relayState && current_processor.frequency != 0) {
    stored_config.lbm_smart_upper_ferq = current_processor.frequency;
    led_blink_async(400); // 快速闪烁提示灯
    Serial.println("Short long press detected, setting relay current...");
  }
  stored_config.save();
}

void InteractiveInterface::onButtonLongPressed() {
  // 长按功能: 恢复出厂设置
  digitalWrite(LED_PIN, led_state ? HIGH : LOW); // 反向亮灭，提示已重置
  delay(1000);

  Serial.println("Long press detected, resetting BatteryAngle...");
  stored_config.clear(); // 清除配置并重启
}

void InteractiveInterface::led_blink_async(unsigned int duration) {
  if (led_blinking_duration != 0) // 如果正在闪烁，则不处理
    return;
  led_blinking_duration = duration;
  led_blink_start = millis();
  led_blink_start_state = led_state;
}

InteractiveInterface interactive_interface;