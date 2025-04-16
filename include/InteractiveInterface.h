#include <Arduino.h>

#define LED_PIN 2
#define BUTTON_PIN 0

class InteractiveInterface {
private:
  unsigned long last_button_release = 0;
  unsigned long last_button_press = 0;
  unsigned long led_blink_start = 0;
  bool led_blink_start_state = false;
  bool led_blinking = false;

  void onButtonClicked();
  void whenLedBlinking();

public:
  void begin();
  void update();

  void led_blink_async();

  bool led_state = true;
  bool button_pressed_down = false;
};

extern InteractiveInterface interactive_interface;