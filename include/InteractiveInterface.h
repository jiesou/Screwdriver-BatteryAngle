#include <Arduino.h>

#define LED_PIN 2
#define BUTTON_PIN 0

#define DEBOUNCE_DELAY 100
#define DOUBLE_CLICK_DELAY 300
#define LONG_PRESS_DELAY 5000
#define SHORT_LONG_PRESS_DELAY 2000
#define BREATHING_HOLDING_INTERVAL 2000

class InteractiveInterface {
private:
  // 按钮状态机
  enum class ButtonState {
    IDLE,              // 空闲状态
    PRESSED,           // 按下状态
    RELEASED_WAIT,     // 释放后等待可能的第二次点击
    DOUBLE_PRESSED     // 双击的第二次按下
  };
  
  ButtonState button_state = ButtonState::IDLE;
  unsigned long last_button_release = 0;
  unsigned long last_button_press = 0;
  
  // LED 运作模式
  enum class LedMode {
    NORMAL,
    BLINKING,
    BREATHING
  };
  LedMode led_mode = LedMode::NORMAL;

  void handleLedBlinking(unsigned long currentMillis);
  unsigned long led_blink_start = 0;
  bool led_blink_start_state = false;
  unsigned int led_blinking_duration = 0;
  
  // 呼吸灯状态机
  void handleLedBreathing(unsigned long currentMillis);
  enum class BreathingState {
    DISABLED,
    FADE_IN,
    FADE_OUT,
    HOLD_MIN,
    HOLD_MAX
  };
  BreathingState breathing_state = BreathingState::DISABLED;
  bool breathing_mode_on = true;
  int breathing_brightness = 0;
  unsigned long last_breathing_update = 0;
  unsigned long last_breathing_reversed = 0;

  void updateLedState();
  void updateButtonState();

  void onButtonClicked();
  void onButtonDoubleClicked();
  void onButtonShortLongPressed();
  void onButtonLongPressed();

public:
  void begin();
  void update();

  void led_blink_async(unsigned int duration);

  bool led_state = true;
  bool button_pressed_down = false;
};

extern InteractiveInterface interactive_interface;