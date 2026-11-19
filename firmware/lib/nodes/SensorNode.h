#pragma once

#include <stdint.h>

#include "../hal/IButton.h"
#include "../hal/IClock.h"
#include "../hal/ILink.h"
#include "../hal/IUserTerminal.h"
#include "../protocol/generated/frame_ids.h"

namespace firmware {

class SensorNode {
 public:
  static constexpr unsigned long kLongPressThresholdMs = 800UL;
  static constexpr unsigned long kDoublePressWindowMs = 350UL;
  static constexpr unsigned long kResponseTimeoutMs = 2000UL;

  SensorNode(IButton* button, ILink* link, IClock* clock,
             IUserTerminal* terminal = nullptr)
      : button_(button), link_(link), clock_(clock), terminal_(terminal) {}

  bool tick() {
    if (button_ == nullptr || link_ == nullptr || clock_ == nullptr) return false;

    bool healthy = checkResponseTimeout();
    const bool pressed = button_->pressed();
    if (pressed) {
      wasPressed_ = true;
    } else if (wasPressed_) {
      wasPressed_ = false;
      if (!handlePress(button_->pressDurationMs())) healthy = false;
    }

    if (!pressed && pendingDoublePress_ &&
        nowMs() - pendingPressAtMs_ >= kDoublePressWindowMs) {
      pendingDoublePress_ = false;
    }

    uint8_t id = 0U;
    uint8_t payload[15] = {};
    unsigned int length = 0U;
    while (link_->receive(&id, payload, sizeof(payload), &length)) {
      if (!handleFrame(id, payload, length)) healthy = false;
    }
    return healthy;
  }

 private:
  enum class Flow : uint8_t {
    Idle,
    AwaitingRentReserved,
    AwaitingRentReceipt,
    AwaitingRentReceiptDisplay,
    RentReceiptAvailable,
    AwaitingPaymentRequest,
    AwaitingPaymentConfirmation,
    AwaitingReturnReceipt,
    AwaitingReturnClose,
    AwaitingReturnEnded
  };

  unsigned long nowMs() const { return clock_ == nullptr ? 0UL : clock_->nowMs(); }

  void waitForReply(Flow flow) {
    flow_ = flow;
    responseStartedAtMs_ = nowMs();
    responseTimeoutActive_ = clock_ != nullptr;
  }

  bool checkResponseTimeout() {
    if (!responseTimeoutActive_ || clock_ == nullptr ||
        nowMs() - responseStartedAtMs_ < kResponseTimeoutMs) {
      return true;
    }

    responseTimeoutActive_ = false;
    pendingDoublePress_ = false;
    flow_ = rentReceiptShown_ ? Flow::RentReceiptAvailable : Flow::Idle;
    if (terminal_ == nullptr) return false;
    terminal_->showError("Actuator response timed out");
    return true;
  }

  bool handlePress(unsigned long durationMs) {
    if (durationMs >= kLongPressThresholdMs) {
      pendingDoublePress_ = false;
      if (flow_ != Flow::Idle && flow_ != Flow::RentReceiptAvailable) return true;
      // @model sequence:return:m0
      if (!link_->send(static_cast<uint8_t>(FrameId::RETURN_REQUEST), nullptr, 0U)) {
        return false;
      }
      waitForReply(Flow::AwaitingPaymentRequest);
      return true;
    }

    if (flow_ == Flow::AwaitingPaymentConfirmation) {
      // @model sequence:return:m3
      if (!link_->send(static_cast<uint8_t>(FrameId::RETURN_CONFIRMPAYMENT), nullptr, 0U)) {
        return false;
      }
      waitForReply(Flow::AwaitingReturnReceipt);
      return true;
    }

    if (flow_ == Flow::AwaitingReturnClose) {
      // @model sequence:return:m7
      if (!link_->send(static_cast<uint8_t>(FrameId::RETURN_CLOSE), nullptr, 0U)) {
        return false;
      }
      waitForReply(Flow::AwaitingReturnEnded);
      return true;
    }

    if (flow_ == Flow::RentReceiptAvailable) {
      if (pendingDoublePress_) {
        if (nowMs() - pendingPressAtMs_ < kDoublePressWindowMs) {
          // @model sequence:rent:m7
          if (!link_->send(static_cast<uint8_t>(FrameId::RENT_SHOWRECEIPT), nullptr, 0U)) {
            return false;
          }
          pendingDoublePress_ = false;
          waitForReply(Flow::AwaitingRentReceiptDisplay);
          return true;
        }
        pendingDoublePress_ = false;
      }
      pendingDoublePress_ = true;
      pendingPressAtMs_ = nowMs();
      return true;
    }

    if (flow_ == Flow::Idle) {
      return sendRentRequest();
    }
    return true;
  }

  bool sendRentRequest() {
    // @model sequence:rent:m0
    if (!link_->send(static_cast<uint8_t>(FrameId::RENT_REQUEST), nullptr, 0U)) {
      return false;
    }
    waitForReply(Flow::AwaitingRentReserved);
    return true;
  }

  bool handleFrame(uint8_t id, const uint8_t* payload, unsigned int length) {
    if (id == static_cast<uint8_t>(FrameId::RENT_RESERVED)) {
      if (length != 0U) return false;
      if (flow_ != Flow::AwaitingRentReserved) return true;
      // @model sequence:rent:m3
      if (!link_->send(static_cast<uint8_t>(FrameId::RENT_UNLOCK), nullptr, 0U)) {
        return false;
      }
      waitForReply(Flow::AwaitingRentReceipt);
      return true;
    }
    if (id == static_cast<uint8_t>(FrameId::RENT_RECEIPT) &&
        (flow_ == Flow::AwaitingRentReceipt ||
         flow_ == Flow::AwaitingRentReceiptDisplay)) {
      int32_t costCents = 0;
      if (!decodeCost(payload, length, &costCents)) return false;
      if (terminal_ == nullptr) return false;
      terminal_->showReceipt(costCents);
      responseTimeoutActive_ = false;
      pendingDoublePress_ = false;
      rentReceiptShown_ = true;
      flow_ = Flow::RentReceiptAvailable;
      return true;
    }
    if (id == static_cast<uint8_t>(FrameId::RETURN_PAYMENTREQUEST) &&
        flow_ == Flow::AwaitingPaymentRequest) {
      int32_t costCents = 0;
      if (!decodeCost(payload, length, &costCents)) return false;
      if (terminal_ == nullptr) return false;
      terminal_->showPaymentPrompt(costCents);
      responseTimeoutActive_ = false;
      flow_ = Flow::AwaitingPaymentConfirmation;
      return true;
    }
    if (id == static_cast<uint8_t>(FrameId::RETURN_RECEIPT) &&
        flow_ == Flow::AwaitingReturnReceipt) {
      int32_t costCents = 0;
      if (!decodeCost(payload, length, &costCents)) return false;
      if (terminal_ == nullptr) return false;
      terminal_->showReceipt(costCents);
      responseTimeoutActive_ = false;
      flow_ = Flow::AwaitingReturnClose;
      return true;
    }
    if (id == static_cast<uint8_t>(FrameId::RETURN_ENDED)) {
      if (length != 0U) return false;
      if (flow_ != Flow::AwaitingReturnEnded) return true;
      responseTimeoutActive_ = false;
      rentReceiptShown_ = false;
      flow_ = Flow::Idle;
      return true;
    }
    return true;
  }

  static bool decodeCost(const uint8_t* payload, unsigned int length, int32_t* costCents) {
    if (payload == nullptr || costCents == nullptr || length != 4U) return false;
    const uint32_t value = static_cast<uint32_t>(payload[0]) |
        (static_cast<uint32_t>(payload[1]) << 8U) |
        (static_cast<uint32_t>(payload[2]) << 16U) |
        (static_cast<uint32_t>(payload[3]) << 24U);
    *costCents = static_cast<int32_t>(value);
    return true;
  }

  IButton* button_;
  ILink* link_;
  IClock* clock_;
  IUserTerminal* terminal_;
  Flow flow_ = Flow::Idle;
  bool wasPressed_ = false;
  bool pendingDoublePress_ = false;
  unsigned long pendingPressAtMs_ = 0UL;
  unsigned long responseStartedAtMs_ = 0UL;
  bool responseTimeoutActive_ = false;
  bool rentReceiptShown_ = false;
};

}  // namespace firmware
