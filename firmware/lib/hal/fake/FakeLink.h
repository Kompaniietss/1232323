#pragma once

#include "../ILink.h"

namespace firmware {

class FakeLink : public ILink {
 public:
  bool send(uint8_t id, const uint8_t* payload, unsigned int len) override {
    lastId_ = id;
    payloadLen_ = len;
    if (payload != nullptr && len > 0) {
      payload_[0] = payload[0];
    }
    return true;
  }

  bool receive(uint8_t* id, uint8_t* payload, unsigned int maxLen) override {
    if (id != nullptr) {
      *id = lastId_;
    }
    if (payload != nullptr && maxLen > 0) {
      payload[0] = payload_[0];
    }
    return payloadLen_ > 0;
  }

 private:
  uint8_t lastId_ = 0;
  uint8_t payload_[8] = {0};
  unsigned int payloadLen_ = 0;
};

}  // namespace firmware
