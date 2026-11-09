#pragma once

namespace firmware {

// @model Pricing
class Pricing {
 public:
  static constexpr int kUnlockFeeCents = 100;
  static constexpr int kPricePerStartedMinuteCents = 15;
  static constexpr unsigned long kMillisecondsPerMinute = 60000UL;

  // @model Pricing.calculate()
  static int calculate(unsigned long durationMs) {
    const unsigned long startedMinutes = durationMs == 0 ? 0 : (durationMs + kMillisecondsPerMinute - 1UL) / kMillisecondsPerMinute;
    return kUnlockFeeCents + static_cast<int>(startedMinutes * kPricePerStartedMinuteCents);
  }
};

}  // namespace firmware
