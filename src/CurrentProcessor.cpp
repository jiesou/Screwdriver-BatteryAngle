#include "CurrentProcessor.h"

CurrentProcessor *CurrentProcessor::instance = nullptr;

void IRAM_ATTR CurrentProcessor::CF1Interrupt() {
  volatile unsigned long current_millis = millis();
  instance->pulse_interval = current_millis - instance->last_pulse_time;
  instance->last_pulse_time = current_millis;
  ++instance->pulse_count;
}

CurrentProcessor::CurrentProcessor() { instance = this; }

void CurrentProcessor::begin() {
  pinMode(14, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(CF1_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(CF1_PIN),
                  CurrentProcessor::CF1Interrupt, RISING);
}

void CurrentProcessor::update() {
  // 20ms 更新一次
  unsigned long current_millis = millis();
  if (current_millis - last_update_time < 20)
    return;
  last_update_time = current_millis;

  if (pulse_count != 0) {
    frequency = 1000.0 / pulse_interval;
  } else {
    frequency = 0;
  }
  pulse_count = 0;

  //  内部计数
  if (frequency > FREQUENCY_THRESHOLD && !appliance_working) {
    appliance_working = true;
    screw_count--;
    if (screw_count == 0) {
      screw_count = 4;
    }
  } else if (frequency <= FREQUENCY_THRESHOLD && appliance_working) {
    appliance_working = false;
  }

  // 插座按钮状态
  btn_pressed = digitalRead(BUTTON_PIN) == LOW;
}
