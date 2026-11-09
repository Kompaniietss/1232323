#pragma once

#include "../ILed.h"

namespace firmware {

class FakeLed : public ILed {
 public:
  void setColor(LedColor color) override { currentColor_ = color; }
  void blink(bool enabled) override { blinking_ = enabled; }

  LedColor color() const { return currentColor_; }
  bool blinking() const { return blinking_; }

 private:
  LedColor currentColor_ = LedColor::Green;
  bool blinking_ = false;
};

}  // namespace firmware
