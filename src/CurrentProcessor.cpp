#include "CurrentProcessor.h"

CurrentProcessor* CurrentProcessor::instance = nullptr;

void IRAM_ATTR CurrentProcessor::CF1Interrupt() {
    unsigned long current_time = micros();
    instance->pulse_interval = current_time - instance->last_pulse_time;
    instance->last_pulse_time = current_time;
    instance->pulse_count++;
}

CurrentProcessor::CurrentProcessor() {
    instance = this;
}

void CurrentProcessor::begin() {
    pinMode(14, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(CF1_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(CF1_PIN), CurrentProcessor::CF1Interrupt, RISING);
}

void CurrentProcessor::update() {
    unsigned long current_time = millis();
    if (current_time - last_update_time < 100)
        return;
    last_update_time = current_time;

    unsigned int pulse_count_in_one_second = pulse_count * 10;
    pulse_count = 0;
    double frequency = 0;

    if (pulse_count_in_one_second != 0) {
        frequency = 1000000.0 / pulse_interval;
    }

    if (frequency > FREQUENCY_THRESHOLD && !appliance_working) {
        appliance_working = true;
        screw_count--;
        if (screw_count == 0) {
            screw_count = 4;
        }
    }
    else if (frequency <= FREQUENCY_THRESHOLD && appliance_working) {
        appliance_working = false;
    }

    btn_pressed = digitalRead(BUTTON_PIN) == LOW;
}

String CurrentProcessor::getStatus() {
    return "{\"state\": " + String(pulse_count * 10) +
           ", \"frequency\": " + String(pulse_interval > 0 ? 1000000.0 / pulse_interval : 0) + "}";
}