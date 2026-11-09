#pragma once

#include <stdint.h>

namespace firmware {

struct Frame {
  uint8_t id;
  uint8_t length;
  uint8_t payload[16];
  uint8_t crc;
};

class FrameCodec {
 public:
  static bool encode(uint8_t id, const uint8_t* payload, uint8_t length, Frame* frame) {
    if (frame == nullptr || length > 15U) {
      return false;
    }
    frame->id = id;
    frame->length = length;
    for (uint8_t i = 0; i < length; ++i) {
      frame->payload[i] = payload[i];
    }
    frame->crc = 0x00U;
    return true;
  }

  static bool decode(const Frame* frame, uint8_t* payload, uint8_t maxLength) {
    if (frame == nullptr || payload == nullptr || maxLength < frame->length) {
      return false;
    }
    for (uint8_t i = 0; i < frame->length; ++i) {
      payload[i] = frame->payload[i];
    }
    return true;
  }
};

}  // namespace firmware
