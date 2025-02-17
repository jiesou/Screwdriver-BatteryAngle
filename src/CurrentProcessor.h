#ifndef CURRENT_PROCESSOR_H
#define CURRENT_PROCESSOR_H

#include <Arduino.h>
#define CF1_PIN 12
#define BUTTON_PIN 2
#define FREQUENCY_THRESHOLD 15.0

class CurrentProcessor {
private:
    static CurrentProcessor* instance;
    volatile unsigned long last_pulse_time = 0;
    volatile unsigned long pulse_interval = 0;
    volatile unsigned int pulse_count = 0;
    static void IRAM_ATTR CF1Interrupt();

public:
    unsigned long last_update_time = 0;
    volatile float frequency = 0;
    //  内部计数
    volatile bool appliance_working = false;
    volatile unsigned int screw_count = 4;
    volatile bool btn_pressed = false;

    CurrentProcessor();
    void begin();
    void update();
};

#endif

extern CurrentProcessor current_processor;