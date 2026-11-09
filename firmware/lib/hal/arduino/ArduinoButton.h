#pragma once

#include "../IButton.h"

namespace firmware {

class ArduinoButton : public IButton {
 public:
  explicit ArduinoButton(int pin) : pin_(pin) {}

  bool pressed() const override { return false; }
  unsigned long pressDurationMs() const override { return 0UL; }

 private:
  int pin_;
};

}  // namespace firmware
