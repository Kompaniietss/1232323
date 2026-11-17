#include <stdint.h>
#include <stdio.h>

#include "../lib/domain/Pricing.h"

int main() {
  const unsigned long durations[] = {0UL, 60000UL, 61000UL, 600000UL};
  for (unsigned int i = 0; i < sizeof(durations) / sizeof(durations[0]); ++i) {
    const int32_t cents = firmware::Pricing::calculate(durations[i]);
    printf("duration_ms=%lu cost_cents=%ld\n", durations[i], static_cast<long>(cents));
  }
  return 0;
}
