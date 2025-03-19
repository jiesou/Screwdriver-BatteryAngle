#include "CurrentProcessor.h"

CurrentProcessor *CurrentProcessor::instance = nullptr;

void IRAM_ATTR CurrentProcessor::CF1Interrupt() {
  instance->current_millis = millis();
  instance->pulse_interval = instance->current_millis -
                             instance->last_pulse_millis; // 两次脉冲的时间差
  instance->last_pulse_millis = instance->current_millis;
  instance->pulse_count++;
}

CurrentProcessor::CurrentProcessor() {
  instance = this;
}

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

  // 检测长时间无脉冲情况
  if (current_millis - last_pulse_millis > PULSE_TIMEOUT) {
    frequency = 0;
    pulse_interval = 0;
    last_pulse_millis = current_millis;
    t_w_sampling_interval = DEFAULT_T_W_SAMPLING_INTERVAL;
    triggered_t_w_mode = false;
    return; // 直接返回，防止后续逻辑错误计算频率
  }

  // 计算频率 - 两种方法选一种更适合的，保证实时性、准确性

  // 方法1：基于脉冲间隔（适合 >3.3hz 高频信号）
  if (pulse_interval < 300) {
    // 时间窗口 300ms
    if (current_millis - last_interval_update_time <= 300)
      return;
    triggered_t_w_mode = false;
    t_w_sampling_interval =
        DEFAULT_T_W_SAMPLING_INTERVAL; // 已是高频，归位默认采样时间，这样下一轮会收敛更快
    frequency = 1000.0 / pulse_interval;
  }
  // 方法2：基于时间窗口（t_w）计数（适合低频信号）
  else {
    // 时间窗口 t_w_sampling_interval
    if (current_millis - last_t_w_update_time <= t_w_sampling_interval)
      return;
    last_t_w_update_time = current_millis;

    if (pulse_count == 0) {
      // 脉冲找不到
      // 1 ms 1 ms 地尝试查找
      t_w_sampling_interval = 1;
      frequency = 0; // 确保无脉冲时频率置零
      return;
    }
    // 如果出现了至少一个脉冲
    triggered_t_w_mode = true;
    // 根据两次脉冲时间重设新采样窗口时间
    t_w_sampling_interval = pulse_interval;

    frequency = (1000.0 / t_w_sampling_interval) * pulse_count;
  }

  // 每次流程的最后，无论何种采样方式都要置零
  pulse_count = 0;
  last_interval_update_time = current_millis;

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