#ifndef CURRENT_PROCESSOR_H
#define CURRENT_PROCESSOR_H

#include <Arduino.h>
#define CF1 13
#define BUTTON_PIN 2

class CurrentProcessor {
private:
    static CurrentProcessor* instance;
    volatile unsigned long pulse_interval = 0;
    volatile unsigned int pulse_count = 0;
    volatile unsigned long last_pulse_time = 0;
    static void IRAM_ATTR CF1Interrupt();

public:
    volatile bool appliance_working = false;
    volatile bool btn_pressed = false;
    volatile unsigned int screw_count = 4;
    const int CF1_PIN = 13;
    const double FREQUENCY_THRESHOLD = 15.0;
    unsigned long last_update_time = 0;

    CurrentProcessor();
    void begin();
    void update();
    String getStatus();
};

#endif