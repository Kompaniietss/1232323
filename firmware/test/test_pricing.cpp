#include <unity.h>

#include "../lib/domain/Pricing.h"

void test_unlock_fee_only(void) {
  TEST_ASSERT_EQUAL_INT(100, firmware::Pricing::calculate(0UL));
}

void test_started_minute_rounding(void) {
  TEST_ASSERT_EQUAL_INT(115, firmware::Pricing::calculate(61000UL));
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_unlock_fee_only);
  RUN_TEST(test_started_minute_rounding);
  return UNITY_END();
}
