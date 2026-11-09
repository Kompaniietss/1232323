#pragma once

#include "../hal/IButton.h"
#include "../hal/ILink.h"

namespace firmware {

class SensorNode {
 public:
  SensorNode(IButton* button, ILink* link) : button_(button), link_(link) {}

  bool tick() {
    if (button_ == nullptr || link_ == nullptr) {
      return false;
    }
    const bool pressed = button_->pressed();
    const unsigned long duration = button_->pressDurationMs();
    if (!pressed) {
      return false;
    }
    const uint8_t action = duration >= 800UL ? 0x20U : 0x10U;
    return link_->send(action, nullptr, 0U);
  }

 private:
  IButton* button_;
  ILink* link_;
};

}  // namespace firmware
