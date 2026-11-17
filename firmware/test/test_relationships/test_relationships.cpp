#include <unity.h>

#include "../../lib/domain/Bike.h"
#include "../../lib/domain/Rental.h"
#include "../../lib/domain/User.h"

void test_rental_references_its_user_and_bike(void) {
  firmware::User user{"user-1", "Rider", "opaque"};
  firmware::Bike bike{"bike-1", 80, firmware::BikeState::Available};
  firmware::Rental rental = {};

  rental.user = &user;
  rental.bike = &bike;

  TEST_ASSERT_EQUAL_PTR(&user, rental.user);
  TEST_ASSERT_EQUAL_PTR(&bike, rental.bike);
  TEST_ASSERT_EQUAL_STRING("user-1", rental.user->userId);
  TEST_ASSERT_EQUAL_STRING("bike-1", rental.bike->bikeId);
}

void test_unassociated_rental_has_no_user_or_bike(void) {
  firmware::Rental rental = {};

  TEST_ASSERT_NULL(rental.user);
  TEST_ASSERT_NULL(rental.bike);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_rental_references_its_user_and_bike);
  RUN_TEST(test_unassociated_rental_has_no_user_or_bike);
  return UNITY_END();
}
