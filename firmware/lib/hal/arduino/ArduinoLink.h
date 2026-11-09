#pragma once

#include <Arduino.h>

#include "../ILink.h"
#include "../../protocol/Crc8.h"

namespace firmware {

class ArduinoLink : public ILink {
 public:
  explicit ArduinoLink(HardwareSerial& serial)
      : serial_(serial), receiveState_(0), receiveId_(0), receiveLength_(0),
        receiveIndex_(0) {}

  bool send(uint8_t id, const uint8_t* payload, unsigned int len) override {
    if (len > 15U || (len > 0U && payload == nullptr)) {
      return false;
    }

    const uint8_t frameLength = static_cast<uint8_t>(len + 4U);
    if (serial_.availableForWrite() < frameLength) {
      return false;
    }

    uint8_t frame[19];
    frame[0] = 0xA5U;
    frame[1] = id;
    frame[2] = static_cast<uint8_t>(len);
    for (uint8_t i = 0; i < len; ++i) {
      frame[i + 3U] = payload[i];
    }
    frame[frameLength - 1U] = Crc8::compute(frame, frameLength - 1U);
    return serial_.write(frame, frameLength) == frameLength;
  }

  bool receive(uint8_t* id, uint8_t* payload, unsigned int maxLen) override {
    if (id == nullptr || payload == nullptr) {
      return false;
    }

    while (serial_.available() > 0) {
      const uint8_t value = static_cast<uint8_t>(serial_.read());
      if (receiveState_ == 0U) {
        if (value == 0xA5U) {
          checksumBytes_[0] = value;
          receiveState_ = 1U;
        }
      } else if (receiveState_ == 1U) {
        receiveId_ = value;
        checksumBytes_[1] = value;
        receiveState_ = 2U;
      } else if (receiveState_ == 2U) {
        receiveLength_ = value;
        if (receiveLength_ > sizeof(receivePayload_)) {
          resetReceive();
        } else {
          checksumBytes_[2] = value;
          receiveIndex_ = 0U;
          receiveState_ = receiveLength_ == 0U ? 4U : 3U;
        }
      } else if (receiveState_ == 3U) {
        receivePayload_[receiveIndex_] = value;
        checksumBytes_[receiveIndex_ + 3U] = value;
        ++receiveIndex_;
        if (receiveIndex_ == receiveLength_) {
          receiveState_ = 4U;
        }
      } else {
        const uint8_t expected = Crc8::compute(
            checksumBytes_, static_cast<uint8_t>(receiveLength_ + 3U));
        if (expected == value && maxLen >= receiveLength_) {
          *id = receiveId_;
          for (uint8_t i = 0; i < receiveLength_; ++i) {
            payload[i] = receivePayload_[i];
          }
          resetReceive();
          return true;
        }
        resetReceive();
      }
    }
    return false;
  }

 private:
  void resetReceive() {
    receiveState_ = 0U;
    receiveLength_ = 0U;
    receiveIndex_ = 0U;
  }

  HardwareSerial& serial_;
  uint8_t receiveState_;
  uint8_t receiveId_;
  uint8_t receiveLength_;
  uint8_t receiveIndex_;
  uint8_t checksumBytes_[18];
  uint8_t receivePayload_[15];
};

}  // namespace firmware
