#pragma once

#include <stdint.h>

namespace firmware {

static_assert(sizeof(int32_t) >= 4, "Rental pricing requires at least 32-bit cents");

// @model Pricing
class Pricing {
 public:
  static constexpr int kUnlockFeeCents = 100;
  static constexpr int kPricePerStartedMinuteCents = 15;
  static constexpr unsigned long kMillisecondsPerMinute = 60000UL;

  // @model Pricing.calculate()
  static int32_t calculate(unsigned long durationMs) {
    const unsigned long startedMinutes = durationMs == 0 ? 0 : (durationMs + kMillisecondsPerMinute - 1UL) / kMillisecondsPerMinute;
    return static_cast<int32_t>(kUnlockFeeCents + startedMinutes * kPricePerStartedMinuteCents);
  }
};

}  // namespace firmware
