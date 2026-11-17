#pragma once

#include <Arduino.h>

#include "../IClock.h"

namespace firmware {

class ArduinoClock : public IClock {
 public:
  unsigned long nowMs() const override { return millis(); }
};

}  // namespace firmware
