#include "CurrentProcessor.h"

CurrentProcessor *CurrentProcessor::instance = nullptr;

void IRAM_ATTR CurrentProcessor::CF1Interrupt() {
  instance->current_micros = micros();
  instance->pulse_interval =
      instance->current_micros -
      instance->last_pulse_micros; // 两次脉冲的时间差（微秒）
  instance->last_pulse_micros = instance->current_micros;
}

CurrentProcessor::CurrentProcessor() { instance = this; }

void CurrentProcessor::begin() {
  pinMode(SEL_PIN, OUTPUT);
  pinMode(CF_PIN, INPUT);
  pinMode(CF1_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(CF1_PIN),
                  CurrentProcessor::CF1Interrupt, RISING);

  digitalWrite(SEL_PIN, LOW);
}

// 检测频率
void CurrentProcessor::update() {
  unsigned long current_micros = micros();

  if (pulse_interval > 1000000)
    pulse_interval = 0; // 频率过低，置零

  // 时间已超过上次脉冲间隔 + PULSE_EXTRA_TIMEOUT 认定无脉冲
  if (current_micros - last_pulse_micros >
      pulse_interval + PULSE_EXTRA_TIMEOUT) {
    frequency = 0; // 无脉冲时频率置零
    return;
  }

  frequency = 1000000.0 / pulse_interval; //  (1秒=1 000 000微秒)

  if (millis() - last_sample_time >= 100)
  {
    frequency_samples[sample_index] = frequency;
    sample_index = (sample_index + 1) % FREQ_BUFFER_SIZE;
    last_sample_time = millis();
  }

  calculateOverallFrequency();

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

void CurrentProcessor::calculateOverallFrequency()
{
  float samples[FREQ_BUFFER_SIZE];
  memcpy(samples, frequency_samples, sizeof(samples));

  // 排序
  for (int i = 0; i < FREQ_BUFFER_SIZE; ++i)
  {
    for (int j = i + 1; j < FREQ_BUFFER_SIZE; ++j)
    {
      if (samples[i] > samples[j])
      {
        float temp = samples[i];
        samples[i] = samples[j];
        samples[j] = temp;
      }
    }
  }

  // 计算中位数
  if (FREQ_BUFFER_SIZE % 2 == 1)
  {
    frequency_overall = samples[FREQ_BUFFER_SIZE / 2];
  }
  else
  {
    frequency_overall = (samples[FREQ_BUFFER_SIZE / 2 - 1] + samples[FREQ_BUFFER_SIZE / 2]) / 2.0f;
  }
}

CurrentProcessor current_processor;