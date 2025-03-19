#ifndef CURRENT_PROCESSOR_H
#define CURRENT_PROCESSOR_H

#include <Arduino.h>
#define CF1_PIN 12
#define BUTTON_PIN 2
#define FREQUENCY_THRESHOLD 15.0

// 简单的频率检测相关常量
#define PULSE_TIMEOUT 3000        // 3秒无脉冲视为停止
#define OVERALL_SAMPLING_INTERVAL 300 // 默认 脉冲间隔 法采样时间

class CurrentProcessor {
private:
    static CurrentProcessor* instance;
    volatile unsigned long last_pulse_millis = 0;
    volatile unsigned long pulse_interval = 0; // ms
    volatile unsigned int pulse_count = 0;
    volatile unsigned int sampling_interval = 300;
    volatile bool triggered_sample_mode = false;
    
    static void IRAM_ATTR CF1Interrupt();

public:
    unsigned long last_update_time = 0;
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