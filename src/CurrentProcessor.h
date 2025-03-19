#ifndef CURRENT_PROCESSOR_H
#define CURRENT_PROCESSOR_H

#include <Arduino.h>
#define CF1_PIN 12
#define BUTTON_PIN 2
#define FREQUENCY_THRESHOLD 15.0

#define PULSE_EXTRA_TIMEOUT 200        // 1000 ms 无脉冲视为停止

class CurrentProcessor {
private:
    static CurrentProcessor* instance;
    volatile unsigned long current_millis;
    volatile unsigned long last_pulse_millis;
    volatile unsigned long pulse_interval; // ms
    
    static void IRAM_ATTR CF1Interrupt();

public:
  volatile float frequency;
  //  内部计数
  volatile bool appliance_working = false;
  volatile unsigned int screw_count = 4;

  CurrentProcessor();
  void begin();
  void update();
};

#endif

extern CurrentProcessor current_processor;