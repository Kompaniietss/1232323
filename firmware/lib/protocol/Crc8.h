#pragma once

#include <stdint.h>

namespace firmware {

class Crc8 {
 public:
  static uint8_t compute(const uint8_t* data, uint8_t length) {
    uint8_t crc = 0x00U;
    for (uint8_t i = 0; i < length; ++i) {
      crc ^= data[i];
      for (uint8_t bit = 0; bit < 8; ++bit) {
        if ((crc & 0x80U) != 0U) {
          crc = static_cast<uint8_t>((crc << 1U) ^ 0x07U);
        } else {
          crc = static_cast<uint8_t>(crc << 1U);
        }
      }
    }
    return crc;
  }
};

}  // namespace firmware
