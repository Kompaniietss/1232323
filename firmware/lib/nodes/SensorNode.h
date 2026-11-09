#pragma once

#include "../hal/IButton.h"
#include "../hal/ILink.h"
#include "../protocol/generated/frame_ids.h"

namespace firmware {

class SensorNode {
 public:
  SensorNode(IButton* button, ILink* link) : button_(button), link_(link) {}

  bool tick() {
    if (button_ == nullptr || link_ == nullptr) {
      return false;
    }
    const bool pressed = button_->pressed();
    if (pressed) {
      wasPressed_ = true;
      return false;
    }
    if (!wasPressed_) {
      return false;
    }
    wasPressed_ = false;
    if (button_->pressDurationMs() >= 800UL) {
      // @model sequence:return:m0
      return link_->send(static_cast<uint8_t>(FrameId::RETURN_REQUEST), nullptr, 0U);
    }
    // @model sequence:rent:m0
    return link_->send(static_cast<uint8_t>(FrameId::RENT_REQUEST), nullptr, 0U);
  }

 private:
  IButton* button_;
  ILink* link_;
  bool wasPressed_ = false;
};

}  // namespace firmware
