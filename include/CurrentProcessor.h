#pragma once
#include <Arduino.h>
#define CF_PIN 13
#define CF1_PIN 12
#define SEL_PIN 14

#define FREQUENCY_THRESHOLD 15.0

#define FREQ_BUFFER_SIZE 10
#define PULSE_EXTRA_TIMEOUT 20000 // 20 ms 无脉冲视为停止

class CurrentProcessor {
private:
  static CurrentProcessor *instance;

  static void IRAM_ATTR CF1Interrupt();
  void calculateOverallFrequency();

public:
  volatile unsigned long current_micros;
  volatile unsigned long last_pulse_micros;
  volatile unsigned long pulse_interval; // ms

  volatile float frequency;
  float frequency_overall = 0.0;
  float frequency_samples[FREQ_BUFFER_SIZE];
  unsigned int sample_index = 0;
  unsigned long last_sample_time = 0;
  //  内部计数
  volatile bool appliance_working = false;
  volatile unsigned int screw_count = 4;

  CurrentProcessor();
  void begin();
  void update();
};

extern CurrentProcessor current_processor;