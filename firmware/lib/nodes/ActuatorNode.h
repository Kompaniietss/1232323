#pragma once

#include "../domain/Bike.h"
#include "../domain/StateMachine.h"
#include "../hal/ILed.h"
#include "../hal/ILink.h"
#include "../protocol/generated/frame_ids.h"

namespace firmware {

class ActuatorNode {
 public:
  ActuatorNode(Bike* bike, ILed* green, ILed* yellow, ILed* red, ILink* link)
      : bike_(bike), green_(green), yellow_(yellow), red_(red), link_(link) {}

  bool tick() {
    if (bike_ == nullptr) {
      return false;
    }
    if (link_ != nullptr) {
      uint8_t id = 0;
      uint8_t payload[15] = {};
      while (link_->receive(&id, payload, sizeof(payload))) {
        if (id == static_cast<uint8_t>(FrameId::RENT_REQUEST)) {
          if (bike_->state == BikeState::Available) {
            StateMachine::apply(bike_, BikeTrigger::Reserve);
          }
          StateMachine::apply(bike_, BikeTrigger::Unlock);
        } else if (id == static_cast<uint8_t>(FrameId::RETURN_REQUEST)) {
          StateMachine::apply(bike_, BikeTrigger::ReturnToStation);
        }
      }
    }

    if (bike_->state != BikeState::Maintenance) {
      if (green_ != nullptr) green_->blink(false);
      if (yellow_ != nullptr) yellow_->blink(false);
      if (red_ != nullptr) red_->blink(false);
    }

    switch (bike_->state) {
      case BikeState::Available:
        if (green_ != nullptr) green_->setColor(LedColor::Green);
        if (yellow_ != nullptr) yellow_->setColor(LedColor::Green);
        if (red_ != nullptr) red_->setColor(LedColor::Green);
        break;
      case BikeState::Reserved:
        if (yellow_ != nullptr) yellow_->setColor(LedColor::Yellow);
        if (green_ != nullptr) green_->setColor(LedColor::Yellow);
        if (red_ != nullptr) red_->setColor(LedColor::Yellow);
        break;
      case BikeState::InUse:
        if (red_ != nullptr) red_->setColor(LedColor::Red);
        if (yellow_ != nullptr) yellow_->setColor(LedColor::Red);
        if (green_ != nullptr) green_->setColor(LedColor::Red);
        break;
      case BikeState::Maintenance:
        if (green_ != nullptr) green_->setColor(LedColor::Red);
        if (yellow_ != nullptr) yellow_->setColor(LedColor::Red);
        if (red_ != nullptr) red_->blink(true);
        break;
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
