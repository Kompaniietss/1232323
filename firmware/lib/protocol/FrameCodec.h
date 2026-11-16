#pragma once

#include <stdint.h>

#include "Crc8.h"

namespace firmware {

struct Frame {
  uint8_t id;
  uint8_t length;
  uint8_t payload[16];
  uint8_t crc;
};

class FrameCodec {
 public:
  static constexpr uint8_t kStartByte = 0xA5U;
  static constexpr uint8_t kMaxPayload = 15U;
  static constexpr uint8_t kMaxWireBytes = 4U + kMaxPayload;

  // CRC-8 (poly 0x07, init 0x00) over id, length and payload.
  static uint8_t computeCrc(uint8_t id, uint8_t length, const uint8_t* payload) {
    uint8_t buffer[2U + kMaxPayload];
    buffer[0] = id;
    buffer[1] = length;
    for (uint8_t i = 0; i < length; ++i) {
      buffer[2U + i] = payload[i];
    }
    return Crc8::compute(buffer, static_cast<uint8_t>(2U + length));
  }

  static bool encode(uint8_t id, const uint8_t* payload, uint8_t length, Frame* frame) {
    if (frame == nullptr || length > kMaxPayload || (length > 0U && payload == nullptr)) {
      return false;
    }
    frame->id = id;
    frame->length = length;
    for (uint8_t i = 0; i < length; ++i) {
      frame->payload[i] = payload[i];
    }
    frame->crc = computeCrc(id, length, frame->payload);
    return true;
  }

  static bool decode(const Frame* frame, uint8_t* payload, uint8_t maxLength) {
    if (frame == nullptr || payload == nullptr || frame->length > kMaxPayload ||
        maxLength < frame->length) {
      return false;
    }
    if (computeCrc(frame->id, frame->length, frame->payload) != frame->crc) {
      return false;
    }
    for (uint8_t i = 0; i < frame->length; ++i) {
      payload[i] = frame->payload[i];
    }
    return true;
  }

  // Wire format: [0xA5][id][length][payload...][crc]. Returns bytes written, 0 on failure.
  static uint8_t serialize(const Frame& frame, uint8_t* out, uint8_t capacity) {
    const uint8_t size = static_cast<uint8_t>(4U + frame.length);
    if (out == nullptr || frame.length > kMaxPayload || capacity < size) {
      return 0U;
    }
    out[0] = kStartByte;
    out[1] = frame.id;
    out[2] = frame.length;
    for (uint8_t i = 0; i < frame.length; ++i) {
      out[3U + i] = frame.payload[i];
    }
    out[3U + frame.length] = frame.crc;
    return size;
  }

  // Parses exactly one complete frame; rejects wrong start byte, wrong size and bad CRC.
  static bool parse(const uint8_t* in, uint8_t size, Frame* frame) {
    if (in == nullptr || frame == nullptr || size < 4U || in[0] != kStartByte) {
      return false;
    }
    const uint8_t length = in[2];
    if (length > kMaxPayload || size != static_cast<uint8_t>(4U + length)) {
      return false;
    }
    frame->id = in[1];
    frame->length = length;
    for (uint8_t i = 0; i < length; ++i) {
      frame->payload[i] = in[3U + i];
    }
    frame->crc = in[3U + length];
    return computeCrc(frame->id, frame->length, frame->payload) == frame->crc;
  }
};

}  // namespace firmware
