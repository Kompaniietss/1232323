#include <unity.h>

#include "../../lib/domain/Rental.h"

void test_cost_is_stored_in_signed_32_bit_minor_units(void) {
  firmware::Rental rental = {};
  rental.costCents = 33400;
  TEST_ASSERT_EQUAL_INT32(33400L, rental.costCents);
  TEST_ASSERT_TRUE(sizeof(rental.costCents) >= 4U);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_cost_is_stored_in_signed_32_bit_minor_units);
  return UNITY_END();
}
