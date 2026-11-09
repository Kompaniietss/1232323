#pragma once

#include <Arduino.h>

#include "../IButton.h"

namespace firmware {

class ArduinoButton : public IButton {
 public:
  explicit ArduinoButton(int pin)
      : pin_(pin), rawPressed_(false), stablePressed_(false), rawChangedAt_(0),
        pressedAt_(0), lastDuration_(0) {}

  void begin() {
    pinMode(pin_, INPUT_PULLUP);
    rawPressed_ = digitalRead(pin_) == LOW;
    rawChangedAt_ = millis();
  }

  bool pressed() const override {
    const unsigned long now = millis();
    const bool rawPressed = digitalRead(pin_) == LOW;
    if (rawPressed != rawPressed_) {
      rawPressed_ = rawPressed;
      rawChangedAt_ = now;
    }
    if (rawPressed_ != stablePressed_ && now - rawChangedAt_ >= 20UL) {
      stablePressed_ = rawPressed_;
      if (stablePressed_) {
        pressedAt_ = now;
      } else {
        lastDuration_ = now - pressedAt_;
      }
    }
    return stablePressed_;
  }

  unsigned long pressDurationMs() const override {
    return stablePressed_ ? millis() - pressedAt_ : lastDuration_;
  }

 private:
  int pin_;
  mutable bool rawPressed_;
  mutable bool stablePressed_;
  mutable unsigned long rawChangedAt_;
  mutable unsigned long pressedAt_;
  mutable unsigned long lastDuration_;
};

}  // namespace firmware
