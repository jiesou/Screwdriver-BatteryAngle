#ifndef CURRENT_PROCESSOR_H
#define CURRENT_PROCESSOR_H

#include <Arduino.h>
#define CF1_PIN 12
#define BUTTON_PIN 2
#define FREQUENCY_THRESHOLD 15.0

// 简单的频率检测相关常量
#define PULSE_TIMEOUT 1000        // 1000 ms 无脉冲视为停止
#define DEFAULT_T_W_SAMPLING_INTERVAL 300 // 默认 脉冲间隔 法采样时间

class CurrentProcessor {
private:
    static CurrentProcessor* instance;
    volatile unsigned long current_millis = millis();
    volatile unsigned long last_pulse_millis = 0;
    volatile unsigned long pulse_interval = 0; // ms
    volatile unsigned int pulse_count = 0;
    volatile bool triggered_t_w_mode = false;
    volatile unsigned int t_w_sampling_interval = 300;
    volatile unsigned long last_t_w_update_time;
    
    static void IRAM_ATTR CF1Interrupt();

public:
  unsigned long last_interval_update_time = 0;
  volatile float frequency = 0;
  //  内部计数
  volatile bool appliance_working = false;
  volatile unsigned int screw_count = 4;

  CurrentProcessor();
  void begin();
  void update();
};

#endif

extern CurrentProcessor current_processor;