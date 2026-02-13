#include "led_blinky.h"

LedBlinky::LedBlinky(int pin, unsigned long period) {
  _pin = pin;
  _period = period;
  _timer = 0;
  _ledState = LOW;
  _enabled = false;
  pinMode(_pin, OUTPUT);
}

void LedBlinky::start() {
  _enabled = true;
  _timer = millis();
}

void LedBlinky::stop() {
  _enabled = false;
  digitalWrite(_pin, LOW);
}

void LedBlinky::update() {
  if (_enabled) {
    if (millis() - _timer >= _period) {
      _timer = millis();
      _ledState = !_ledState;
      digitalWrite(_pin, _ledState);
    }
  }
}
