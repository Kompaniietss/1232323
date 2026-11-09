#pragma once

#include "../IButton.h"

namespace firmware {

class FakeButton : public IButton {
 public:
  explicit FakeButton(bool pressed = false) : pressed_(pressed) {}

  bool pressed() const override { return pressed_; }
  unsigned long pressDurationMs() const override { return pressed_ ? 250UL : 0UL; }

  void setPressed(bool value) { pressed_ = value; }

 private:
  bool pressed_;
};

}  // namespace firmware
