#pragma once

#include "../ILed.h"

namespace firmware {

class ArduinoLed : public ILed {
 public:
  explicit ArduinoLed(int pin) : pin_(pin) {}

  void setColor(LedColor color) override { (void)color; }
  void blink(bool enabled) override { (void)enabled; }

 private:
  int pin_;
};

}  // namespace firmware
