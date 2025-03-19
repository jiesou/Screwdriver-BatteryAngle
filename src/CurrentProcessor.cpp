#include "CurrentProcessor.h"

CurrentProcessor *CurrentProcessor::instance = nullptr;

void IRAM_ATTR CurrentProcessor::CF1Interrupt() {
  instance->current_millis = millis();
  instance->pulse_interval = instance->current_millis -
                             instance->last_pulse_millis; // 两次脉冲的时间差
  instance->last_pulse_millis = instance->current_millis;
}

CurrentProcessor::CurrentProcessor() { instance = this; }

void CurrentProcessor::begin() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(CF1_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(CF1_PIN),
                  CurrentProcessor::CF1Interrupt, RISING);

  tone(13, 1.5);
  tone(14, 5);
  tone(15, 10);
  tone(5, 0.5);
}

// 检测频率
void CurrentProcessor::update() {
  unsigned long current_millis = millis();
  
  // 时间已超过上次脉冲间隔 + PULSE_EXTRA_TIMEOUT 认定无脉冲
  if (current_millis - last_pulse_millis >
      pulse_interval + PULSE_EXTRA_TIMEOUT) {
    frequency = 0; // 无脉冲时频率置零
    return;
  }

  frequency = 1000.0 / pulse_interval;

  // 内部计数
  if (frequency > FREQUENCY_THRESHOLD && !appliance_working) {
    appliance_working = true;
    screw_count--;
    if (screw_count == 0) {
      screw_count = 4;
    }
  } else if (frequency <= FREQUENCY_THRESHOLD && appliance_working) {
    appliance_working = false;
  }
}

CurrentProcessor current_processor;