#include "CurrentProcessor.h"

CurrentProcessor *CurrentProcessor::instance = nullptr;

void IRAM_ATTR CurrentProcessor::CF1Interrupt() {
  volatile unsigned long current_millis = millis();
  instance->pulse_interval =
      current_millis - instance->last_pulse_millis; // 两次脉冲的时间差
  instance->last_pulse_millis = current_millis;
  instance->pulse_count++;
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

void CurrentProcessor::update() {

  // 检测频率
  unsigned long current_millis = millis();

  // 每 sampling_interval ms 触发一次流程
  if (current_millis - last_update_time <= sampling_interval) {
    return;
  }
  last_update_time = current_millis;

  // 检查是否有超时（长时间无脉冲）
  // if ((current_millis - last_pulse_time) > PULSE_TIMEOUT && frequency >
  // 0) {
  //   frequency = 0;
  //   Serial.println("No pulse detected, frequency reset to 0");
  // }

  // 计算频率 - 两种方法选一种更适合的，保证实时性、准确性

  // 方法1：基于脉冲间隔 (适合 >5hz 高频信号)
  if (pulse_interval < 200) {
    triggered_sample_mode = false;
    frequency = 1000.0 / pulse_interval;
    sampling_interval = 300;
  }
  // 方法2：基于时间窗口计数 (适合低频信号)
  else {
    if (pulse_count == 0) {
      // 没找到脉冲先 1 ms 1 ms 地尝试
      sampling_interval = 1;
      return;
    }

    // 如果出现了至少一个脉冲
    triggered_sample_mode = true;
    // 根据两次脉冲时间重订新采样窗口时间
    sampling_interval = pulse_interval;
    frequency = (1000.0 / sampling_interval) * pulse_count;
  }

  Serial.println("Mills: " + String(current_millis) +
                 " freq: " + String(frequency) +
                 " pulse_interval: " + String(pulse_interval) +
                 " pulse_count: " + String(pulse_count) +
                 " sampling_interval: " + String(sampling_interval) +
                 " triggered_sample_mode: " + String(triggered_sample_mode) +
                 " screw_count: " + String(screw_count));

  // 每次流程的最后，无论何种采样方式都要置零
  pulse_count = 0;

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