#include <unity.h>

#include "../../lib/domain/Pricing.h"

// Cases mirror src/domain/pricing.test.ts (UT-PRICE-01), EUR converted to cents.
void test_zero_duration_is_unlock_fee_only(void) {
  TEST_ASSERT_EQUAL_INT(100, firmware::Pricing::calculate(0UL));
}

void test_one_millisecond_is_one_started_minute(void) {
  TEST_ASSERT_EQUAL_INT(115, firmware::Pricing::calculate(1UL));
}

void test_thirty_seconds_is_one_started_minute(void) {
  TEST_ASSERT_EQUAL_INT(115, firmware::Pricing::calculate(30000UL));
}

void test_exactly_one_minute_is_one_started_minute(void) {
  TEST_ASSERT_EQUAL_INT(115, firmware::Pricing::calculate(60000UL));
}

void test_one_ms_over_a_minute_is_two_started_minutes(void) {
  TEST_ASSERT_EQUAL_INT(130, firmware::Pricing::calculate(60001UL));
}

void test_61_seconds_is_two_started_minutes(void) {
  TEST_ASSERT_EQUAL_INT(130, firmware::Pricing::calculate(61000UL));
}

void test_exactly_two_minutes_is_two_started_minutes(void) {
  TEST_ASSERT_EQUAL_INT(130, firmware::Pricing::calculate(120000UL));
}

void test_thirty_seven_hours_cost_fits_signed_32_bit_cents(void) {
  const unsigned long durationMs = 37UL * 60UL * 60UL * 1000UL;
  TEST_ASSERT_EQUAL_INT32(33400L, firmware::Pricing::calculate(durationMs));
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_zero_duration_is_unlock_fee_only);
  RUN_TEST(test_one_millisecond_is_one_started_minute);
  RUN_TEST(test_thirty_seconds_is_one_started_minute);
  RUN_TEST(test_exactly_one_minute_is_one_started_minute);
  RUN_TEST(test_one_ms_over_a_minute_is_two_started_minutes);
  RUN_TEST(test_61_seconds_is_two_started_minutes);
  RUN_TEST(test_exactly_two_minutes_is_two_started_minutes);
  RUN_TEST(test_thirty_seven_hours_cost_fits_signed_32_bit_cents);
  return UNITY_END();
}
