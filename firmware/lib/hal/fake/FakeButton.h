#pragma once

#include "../IButton.h"

namespace firmware {

class FakeButton : public IButton {
 public:
  explicit FakeButton(bool pressed = false) : pressed_(pressed) {}

  bool pressed() const override { return pressed_; }
  unsigned long pressDurationMs() const override {
    return pressed_ ? pressDurationMs_ : lastDurationMs_;
  }

  void setPressed(bool value) {
    if (pressed_ && !value) lastDurationMs_ = pressDurationMs_;
    pressed_ = value;
  }
  void setPressDurationMs(unsigned long durationMs) { pressDurationMs_ = durationMs; }

 private:
  bool pressed_;
  unsigned long pressDurationMs_ = 250UL;
  unsigned long lastDurationMs_ = 0UL;
};

}  // namespace firmware
