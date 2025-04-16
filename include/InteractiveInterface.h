#include <Arduino.h>

#define LED_PIN 2
#define BUTTON_PIN 0

class InteractiveInterface {
private:
  unsigned long last_button_press = 0;
  unsigned long led_blink_start = 0;
  bool led_blink_start_state = false;
  bool led_blinking = false;

public:
  void begin();
  void update();

  void led_blink_async() {
    if (!led_blinking) {
      led_blinking = true;
      led_blink_start_state = led_state;
      led_blink_start = millis();
    }
    if ((millis() - led_blink_start) % 100 < 50) {
      led_state = !led_state;
    }

    if (millis() - led_blink_start > 1000) {
      led_blinking = false;
      led_state = led_blink_start_state;
    }
  }

  bool led_state = true;
  bool button_pressed = false;
};

extern InteractiveInterface interactive_interface;