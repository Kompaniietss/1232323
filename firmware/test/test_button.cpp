#include <unity.h>

#include "../lib/hal/fake/FakeButton.h"

void test_debounce_passthrough(void) {
  firmware::FakeButton button(true);
  TEST_ASSERT_TRUE(button.pressed());
  TEST_ASSERT_EQUAL_UINT32(250U, button.pressDurationMs());
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_debounce_passthrough);
  return UNITY_END();
}
