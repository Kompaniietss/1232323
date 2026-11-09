#pragma once

#include "../ILink.h"

namespace firmware {

class ArduinoLink : public ILink {
 public:
  bool send(uint8_t id, const uint8_t* payload, unsigned int len) override {
    (void)id;
    (void)payload;
    (void)len;
    return true;
  }

  bool receive(uint8_t* id, uint8_t* payload, unsigned int maxLen) override {
    (void)id;
    (void)payload;
    (void)maxLen;
    return false;
  }
};

}  // namespace firmware
