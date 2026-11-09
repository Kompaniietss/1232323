#pragma once

#include <Arduino.h>

#include "../ILed.h"

namespace firmware {

class ArduinoLed : public ILed {
 public:
  ArduinoLed(int pin, LedColor color) : pin_(pin), color_(color), active_(false), blinking_(false),
                                        blinkOn_(false), lastBlinkAt_(0) {}

  void begin() {
    pinMode(pin_, OUTPUT);
    digitalWrite(pin_, LOW);
  }

  void setColor(LedColor color) override {
    active_ = color == color_;
    if (!blinking_) {
      digitalWrite(pin_, active_ ? HIGH : LOW);
    }
  }

  void blink(bool enabled) override {
    if (!enabled) {
      blinking_ = false;
      blinkOn_ = false;
      digitalWrite(pin_, active_ ? HIGH : LOW);
      return;
    }

    const unsigned long now = millis();
    if (!blinking_) {
      blinking_ = true;
      blinkOn_ = false;
      lastBlinkAt_ = now;
    } else if (now - lastBlinkAt_ >= 500UL) {
      blinkOn_ = !blinkOn_;
      lastBlinkAt_ = now;
    }
    digitalWrite(pin_, blinkOn_ ? HIGH : LOW);
  }

 private:
  int pin_;
  LedColor color_;
  bool active_;
  bool blinking_;
  bool blinkOn_;
  unsigned long lastBlinkAt_;
};

}  // namespace firmware
