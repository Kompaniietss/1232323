#pragma once

#include <stdint.h>

#include "../domain/Bike.h"
#include "../domain/Pricing.h"
#include "../domain/Rental.h"
#include "../domain/StateMachine.h"
#include "../domain/Station.h"
#include "../hal/IClock.h"
#include "../hal/ILed.h"
#include "../hal/ILink.h"
#include "../protocol/generated/frame_ids.h"

namespace firmware {

class ActuatorNode {
 public:
  ActuatorNode(Bike* bike, ILed* green, ILed* yellow, ILed* red, ILink* link,
               IClock* clock = nullptr, Rental* rental = nullptr,
               Station* station = nullptr)
      : bike_(bike), green_(green), yellow_(yellow), red_(red), link_(link),
        clock_(clock), rental_(rental), station_(station) {}

  bool tick() {
    if (bike_ == nullptr) return false;
    bool healthy = true;
    if (link_ != nullptr) {
      uint8_t id = 0U;
      uint8_t payload[15] = {};
      unsigned int length = 0U;
      while (link_->receive(&id, payload, sizeof(payload), &length)) {
        if (!handleFrame(id, payload, length)) healthy = false;
      }
    }

    updateLeds();
    return healthy;
  }

 private:
  unsigned long nowMs() const { return clock_ == nullptr ? 0UL : clock_->nowMs(); }

  bool handleFrame(uint8_t id, const uint8_t* payload, unsigned int length) {
    if (id == static_cast<uint8_t>(FrameId::RENT_REQUEST)) {
      if (length != 0U) return false;
      if (bike_->state != BikeState::Available) return true;
      if (!reserveBike()) return false;
      pendingRentUnlock_ = true;
      hasReturnReceipt_ = false;
      pendingReturnPayment_ = false;
      return sendRentReserved();
    }
    if (id == static_cast<uint8_t>(FrameId::RENT_UNLOCK)) {
      if (length != 0U) return false;
      if (!pendingRentUnlock_) return true;
      if (!unlockBike()) return false;
      pendingRentUnlock_ = false;
      rentalStartedAtMs_ = nowMs();
      rentReceiptCostCents_ = Pricing::calculate(0UL);
      hasRentReceipt_ = true;
      if (rental_ != nullptr) {
        rental_->start();
        rental_->startTime = rentalStartedAtMs_;
        rental_->costCents = rentReceiptCostCents_;
      }
      return sendRentStarted() && sendRentActive() && sendRentReceipt();
    }
    if (id == static_cast<uint8_t>(FrameId::RENT_SHOWRECEIPT)) {
      if (length != 0U) return false;
      if (!hasRentReceipt_) return true;
      return sendRentReceipt();
    }
    if (id == static_cast<uint8_t>(FrameId::RETURN_REQUEST)) {
      if (length != 0U) return false;
      if (bike_->state != BikeState::InUse || pendingReturnPayment_) return true;
      if (station_ != nullptr) station_->dockBike(bike_);
      returnCostCents_ = Pricing::calculate(nowMs() - rentalStartedAtMs_);
      pendingReturnPayment_ = true;
      return sendReturnDocked() && sendReturnPaymentRequest();
    }
    if (id == static_cast<uint8_t>(FrameId::RETURN_CONFIRMPAYMENT) &&
        pendingReturnPayment_ && length == 0U) {
      if (!lockReturnedBike()) return false;
      pendingReturnPayment_ = false;
      hasReturnReceipt_ = true;
      if (rental_ != nullptr) {
        rental_->end();
        rental_->endTime = nowMs();
        rental_->costCents = returnCostCents_;
      }
      return sendReturnAvailable() && sendReturnReceipt();
    }
    if (id == static_cast<uint8_t>(FrameId::RETURN_CLOSE)) {
      if (length != 0U) return false;
      if (!hasReturnReceipt_) return true;
      return sendReturnEnded();
    }
    (void)payload;
    return true;
  }

  // @model sequence:rent:m1
  bool reserveBike() {
    if (station_ != nullptr && station_->releaseBike() != bike_) return false;
    return StateMachine::apply(bike_, BikeTrigger::Reserve);
  }

  bool unlockBike() {
    return StateMachine::apply(bike_, BikeTrigger::Unlock);
  }

  // @model sequence:return:m4
  bool lockReturnedBike() {
    if (!StateMachine::apply(bike_, BikeTrigger::ReturnToStation)) return false;
    bike_->lock();
    return true;
  }

  static void encodeCost(int32_t costCents, uint8_t payload[4]) {
    const uint32_t value = static_cast<uint32_t>(costCents);
    payload[0] = static_cast<uint8_t>(value & 0xFFU);
    payload[1] = static_cast<uint8_t>((value >> 8U) & 0xFFU);
    payload[2] = static_cast<uint8_t>((value >> 16U) & 0xFFU);
    payload[3] = static_cast<uint8_t>((value >> 24U) & 0xFFU);
  }

  bool sendCost(uint8_t id, int32_t costCents) {
    if (link_ == nullptr) return false;
    uint8_t payload[4] = {};
    encodeCost(costCents, payload);
    return link_->send(id, payload, sizeof(payload));
  }

  bool sendRentReserved() {
    // @model sequence:rent:m2
    return link_ != nullptr &&
        link_->send(static_cast<uint8_t>(FrameId::RENT_RESERVED), nullptr, 0U);
  }

  bool sendRentStarted() {
    // @model sequence:rent:m4
    return link_ != nullptr &&
        link_->send(static_cast<uint8_t>(FrameId::RENT_STARTED), nullptr, 0U);
  }

  bool sendRentActive() {
    // @model sequence:rent:m5
    return link_ != nullptr &&
        link_->send(static_cast<uint8_t>(FrameId::RENT_ACTIVE), nullptr, 0U);
  }

  bool sendRentReceipt() {
    // @model sequence:rent:m6
    return sendCost(static_cast<uint8_t>(FrameId::RENT_RECEIPT), rentReceiptCostCents_);
  }

  bool sendReturnDocked() {
    // @model sequence:return:m1
    return link_ != nullptr &&
        link_->send(static_cast<uint8_t>(FrameId::RETURN_DOCKED), nullptr, 0U);
  }

  bool sendReturnPaymentRequest() {
    // @model sequence:return:m2
    return sendCost(static_cast<uint8_t>(FrameId::RETURN_PAYMENTREQUEST), returnCostCents_);
  }

  bool sendReturnAvailable() {
    // @model sequence:return:m5
    return link_ != nullptr &&
        link_->send(static_cast<uint8_t>(FrameId::RETURN_AVAILABLE), nullptr, 0U);
  }

  bool sendReturnReceipt() {
    // @model sequence:return:m6
    return sendCost(static_cast<uint8_t>(FrameId::RETURN_RECEIPT), returnCostCents_);
  }

  bool sendReturnEnded() {
    // @model sequence:return:m8
    return link_ != nullptr &&
        link_->send(static_cast<uint8_t>(FrameId::RETURN_ENDED), nullptr, 0U);
  }

  void updateLeds() {
    if (bike_->state != BikeState::Maintenance) {
      if (green_ != nullptr) green_->blink(false);
      if (yellow_ != nullptr) yellow_->blink(false);
      if (red_ != nullptr) red_->blink(false);
    }

    LedColor color = LedColor::Green;
    switch (bike_->state) {
      case BikeState::Available:
        color = LedColor::Green;
        break;
      case BikeState::Reserved:
        color = LedColor::Yellow;
        break;
      case BikeState::InUse:
        color = LedColor::Red;
        break;
      case BikeState::Maintenance:
        color = LedColor::Red;
        break;
    }

    if (green_ != nullptr) green_->setColor(color);
    if (yellow_ != nullptr) yellow_->setColor(color);
    if (red_ != nullptr) {
      red_->setColor(color);
      red_->blink(bike_->state == BikeState::Maintenance);
    }
  }

  Bike* bike_;
  ILed* green_;
  ILed* yellow_;
  ILed* red_;
  ILink* link_;
  IClock* clock_;
  Rental* rental_;
  Station* station_;
  unsigned long rentalStartedAtMs_ = 0UL;
  int32_t rentReceiptCostCents_ = 0;
  int32_t returnCostCents_ = 0;
  bool pendingRentUnlock_ = false;
  bool pendingReturnPayment_ = false;
  bool hasRentReceipt_ = false;
  bool hasReturnReceipt_ = false;
};

}  // namespace firmware
