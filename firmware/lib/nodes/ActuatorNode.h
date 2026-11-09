#pragma once

#include "../domain/Bike.h"
#include "../domain/StateMachine.h"
#include "../hal/ILed.h"
#include "../hal/ILink.h"

namespace firmware {

class ActuatorNode {
 public:
  ActuatorNode(Bike* bike, ILed* green, ILed* yellow, ILed* red, ILink* link)
      : bike_(bike), green_(green), yellow_(yellow), red_(red), link_(link) {}

  bool tick() {
    if (bike_ == nullptr) {
      return false;
    }
    switch (bike_->state) {
      case BikeState::Available:
        if (green_ != nullptr) green_->setColor(LedColor::Green);
        if (yellow_ != nullptr) yellow_->blink(false);
        if (red_ != nullptr) red_->blink(false);
        break;
      case BikeState::Reserved:
        if (yellow_ != nullptr) yellow_->setColor(LedColor::Yellow);
        if (green_ != nullptr) green_->blink(false);
        if (red_ != nullptr) red_->blink(false);
        break;
      case BikeState::InUse:
        if (red_ != nullptr) red_->setColor(LedColor::Red);
        if (yellow_ != nullptr) yellow_->blink(false);
        if (green_ != nullptr) green_->blink(false);
        break;
      case BikeState::Maintenance:
        if (red_ != nullptr) red_->blink(true);
        break;
    }
    if (link_ != nullptr) {
      uint8_t id = 0x00U;
      uint8_t payload[1] = {static_cast<uint8_t>(bike_->state)};
      return link_->send(id, payload, 1U);
    }
    return true;
  }

 private:
  Bike* bike_;
  ILed* green_;
  ILed* yellow_;
  ILed* red_;
  ILink* link_;
};

}  // namespace firmware
