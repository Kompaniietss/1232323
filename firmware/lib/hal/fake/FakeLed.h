#pragma once

#include "../ILed.h"

namespace firmware {

class FakeLed : public ILed {
 public:
  explicit FakeLed(LedColor ledColor = LedColor::Green) : ledColor_(ledColor) {}

  void setColor(LedColor color) override { currentColor_ = color; }
  void blink(bool enabled) override { blinking_ = enabled; }

  LedColor color() const { return currentColor_; }
  bool on() const { return currentColor_ == ledColor_; }
  bool blinking() const { return blinking_; }

 private:
  LedColor ledColor_;
  LedColor currentColor_ = LedColor::Green;
  bool blinking_ = false;
};

}  // namespace firmware
