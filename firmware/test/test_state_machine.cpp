#include <unity.h>

#include "../lib/domain/Bike.h"
#include "../lib/domain/StateMachine.h"

void test_t1_transition(void) {
  firmware::Bike bike{"bike-1", 100, firmware::BikeState::Available};
  TEST_ASSERT_TRUE(firmware::StateMachine::apply(&bike, firmware::BikeTrigger::Reserve));
  TEST_ASSERT_EQUAL(firmware::BikeState::Reserved, bike.state);
}

void test_t5_transition(void) {
  firmware::Bike bike{"bike-1", 100, firmware::BikeState::InUse};
  TEST_ASSERT_TRUE(firmware::StateMachine::apply(&bike, firmware::BikeTrigger::ReturnToStation));
  TEST_ASSERT_EQUAL(firmware::BikeState::Available, bike.state);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_t1_transition);
  RUN_TEST(test_t5_transition);
  return UNITY_END();
}
