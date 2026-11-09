#pragma once

#include "../IClock.h"

namespace firmware {

class FakeClock : public IClock {
 public:
  explicit FakeClock(unsigned long startMs = 0UL) : nowMs_(startMs) {}

  unsigned long nowMs() const override { return nowMs_; }
  void advance(unsigned long deltaMs) { nowMs_ += deltaMs; }

 private:
  unsigned long nowMs_;
};

}  // namespace firmware
