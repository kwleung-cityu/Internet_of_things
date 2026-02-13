#ifndef _LED_BLINKY_H
#define _LED_BLINKY_h

#include <Arduino.h>

class LedBlinky {
  public:
    // Constructor
    // pin: GPIO pin number for the LED
    // period: Blinking period in milliseconds (default is 500ms)
    LedBlinky(int pin, unsigned long period = 500);

    void start();
    void stop();
    void update();

  private:
    int _pin;
    unsigned long _period;
    unsigned long _timer;
    bool _ledState;
    bool _enabled;
};

#endif
