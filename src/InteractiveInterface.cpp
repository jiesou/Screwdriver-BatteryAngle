#include "InteractiveInterface.h"
#include "CurrentProcessor.h"
#include "RelayControler.h"
#include "StoredConfig.h"
#include <Arduino.h>

void InteractiveInterface::begin()
{
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
}

void InteractiveInterface::update()
{
  updateLedState();
  updateButtonState();
}

void InteractiveInterface::updateLedState()
{
  unsigned long currentMillis = millis();

  // 确定 LED 的基本状态为继电器开关 (仅用于普通模式)
  led_state = relay_controler.relayState;

  if (stored_config.lbm_smart_enabled)
  {
    switch (relay_controler.lbmState)
    {
    case RelayControler::lbmState::WAITING_RISING:
      // LBM 充电中
      breathing_state = BreathingState::FADE_IN; // 呼吸灯渐亮
      break;
    case RelayControler::lbmState::WAITING_DROPPING:
      // LBM 耗电中
      breathing_state = BreathingState::FADE_OUT; // 呼吸灯渐暗
      break;
    default:
      // 其他状态下 LED 快闪（检查频率中）
      breathing_state = BreathingState::DISABLED;
      led_state = millis() % 200 < 100;
      break;
    }
  }
  else
  {
    // 如果不处于 LBM 智能控制状态，则 LED 呼吸灯模式状态禁用
    breathing_state = BreathingState::DISABLED;
  }

  // 每次 update 都要确定 LED 运作模式
  if (led_blinking_duration > 0)
  {
    // 闪灯模式优先级最高
    led_mode = LedMode::BLINKING;
  }
  else if (breathing_state != BreathingState::DISABLED)
  {
    // 其次是呼吸灯模式
    led_mode = LedMode::BREATHING;
  }
  else
  {
    led_mode = LedMode::NORMAL;
  }

  switch (led_mode)
  {
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

void InteractiveInterface::handleLedBlinking(unsigned long currentMillis)
{
  unsigned long elapsedMillis = currentMillis - led_blink_start;

  if (elapsedMillis >= led_blinking_duration)
  {
    // 闪烁结束
    led_blinking_duration = 0;
    led_state = led_blink_start_state;
    digitalWrite(LED_PIN, led_state ? LOW : HIGH);
    return;
  }

  // 闪烁逻辑
  bool blinkState;
  if (elapsedMillis % 200 < 100)
  {
    blinkState = !led_blink_start_state;
  }
  else
  {
    blinkState = led_blink_start_state;
  }

  digitalWrite(LED_PIN, blinkState ? LOW : HIGH);
}

void InteractiveInterface::handleLedBreathing(unsigned long currentMillis)
{
  if (currentMillis - last_breathing_update < 30)
    return; // 30ms 更新一次呼吸灯
  last_breathing_update = currentMillis;

  switch (breathing_state)
  {
  case BreathingState::FADE_IN:
    // 亮度增加阶段
    breathing_brightness += 2;
    if (breathing_brightness >= 150)
    {
      breathing_brightness = 30; // 从 30 开始，补偿低亮度
    }
    break;

  case BreathingState::FADE_OUT:
    // 亮度减少阶段
    breathing_brightness -= 2;
    if (breathing_brightness <= -50)
    {
      breathing_brightness = 80; // 从 70 开始，补偿高亮度
    }
    break;

  case BreathingState::DISABLED:
    break;
  }

  analogWrite(LED_PIN, map(breathing_brightness, 0, 100, 255, 0)); // 注意外部上拉
}

void InteractiveInterface::updateButtonState()
{
  unsigned long currentMillis = millis();
  bool current_button = (digitalRead(BUTTON_PIN) == LOW);

  switch (button_state)
  {
  case ButtonState::IDLE:
    if (current_button)
    { // 从空闲状态检测到按钮按下
      last_button_press = currentMillis;
      button_state = ButtonState::PRESSED;
      button_pressed_down = true;
    }
    break;
  case ButtonState::PRESSED:
    if (current_button &&
        (currentMillis - last_button_press > LONG_PRESS_DELAY))
    {
      // 检测到长按
      onButtonLongPressed();
      button_state = ButtonState::IDLE;
      button_pressed_down = false;
      break;
    }
    else if (current_button &&
             (currentMillis - last_button_press > SHORT_LONG_PRESS_DELAY) &&
             (currentMillis - last_button_press < LONG_PRESS_DELAY))
    {
      // 检测到短长按 (处于2秒和10秒之间)
      onButtonShortLongPressed();
      // 保持在 PRESSED 状态以便后续可能的释放和双击检测
      break;
    }
    else if (!current_button &&
             (currentMillis - last_button_press > DEBOUNCE_DELAY))
    {
      // 按钮马上释放，进入等待状态以检测可能的双击
      last_button_release = currentMillis;
      button_state = ButtonState::RELEASED_WAIT;
      button_pressed_down = false;
    }
    break;

  case ButtonState::RELEASED_WAIT:
    if (current_button &&
        (currentMillis - last_button_release < DOUBLE_CLICK_DELAY))
    {
      // 在双击时间窗口内检测到第二次按下
      button_state = ButtonState::DOUBLE_PRESSED;
      button_pressed_down = true;
    }
    else if (currentMillis - last_button_release >= DOUBLE_CLICK_DELAY)
    {
      // 超过双击时间窗口，判定为单击
      onButtonClicked();
      button_state = ButtonState::IDLE;
    }
    break;

  case ButtonState::DOUBLE_PRESSED:
    if (!current_button &&
        (currentMillis - last_button_press > DEBOUNCE_DELAY))
    {
      // 第二次按下后释放，确认为双击
      onButtonDoubleClicked();
      button_state = ButtonState::IDLE;
      button_pressed_down = false;
    }
    break;
  }
}

void InteractiveInterface::onButtonClicked()
{
  if (stored_config.relay_schedule_on != 0 &&
      stored_config.relay_schedule_off != 0)
  {
    // 如果正处于周期控制状态，则闪烁提示灯
    led_blink_async(1500);
  }
  else if (stored_config.lbm_smart_enabled)
  {
    led_blink_async(1500);
    // 如果正处于 LBM 智能控制状态，则无论如何切换到检测频率（充电状态）
    relay_controler.relayState = true; // 开启继电器
    relay_controler.lbmStartTimeOfCheckingFreq = millis();
    relay_controler.lbmState = RelayControler::lbmState::PREPARING_FOR_CHECKING_FREQ;
  }
  else
  {
    // 默认状态切换继电器状态
    relay_controler.relayState = !relay_controler.relayState;
  }
}

void InteractiveInterface::onButtonDoubleClicked()
{
  led_blink_async(400); // 快速闪烁提示灯

  // 双击功能: 切换智能 LBM 模式
  stored_config.lbm_smart_enabled = !stored_config.lbm_smart_enabled;
  // 保存配置
  stored_config.save();
}

void InteractiveInterface::onButtonShortLongPressed()
{
  // 短长按功能: 设置继电器整定电流
  if (relay_controler.relayState && current_processor.frequency != 0)
  {
    stored_config.lbm_smart_upper_ferq = current_processor.frequency;
    led_blink_async(400); // 快速闪烁提示灯
    Serial.println("Short long press detected, setting relay current...");
  }
  stored_config.save();
}

void InteractiveInterface::onButtonLongPressed()
{
  // 长按功能: 恢复出厂设置
  digitalWrite(LED_PIN, led_state ? HIGH : LOW); // 反向亮灭，提示已重置
  delay(1000);

  Serial.println("Long press detected, resetting BatteryAngle...");
  stored_config.clear(); // 清除配置并重启
}

void InteractiveInterface::led_blink_async(unsigned int duration)
{
  if (led_blinking_duration != 0) // 如果正在闪烁，则不处理
    return;
  led_blinking_duration = duration;
  led_blink_start = millis();
  led_blink_start_state = led_state;
}

InteractiveInterface interactive_interface;