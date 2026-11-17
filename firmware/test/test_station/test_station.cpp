#include <unity.h>

#include "../../lib/domain/Station.h"

void test_dock_and_release_bike(void) {
  firmware::Station station = {};
  station.stationId = "station-1";
  station.capacity = 2;
  firmware::Bike bike{"bike-1", 100, firmware::BikeState::Available};

  station.dockBike(&bike);
  TEST_ASSERT_EQUAL_PTR(&bike, station.releaseBike());
  TEST_ASSERT_NULL(station.releaseBike());
}

void test_dock_respects_capacity_and_ignores_duplicate_and_null(void) {
  firmware::Station station = {};
  station.stationId = "station-1";
  station.capacity = 1;
  firmware::Bike first{"bike-1", 100, firmware::BikeState::Available};
  firmware::Bike second{"bike-2", 100, firmware::BikeState::Available};

  station.dockBike(nullptr);
  station.dockBike(&first);
  station.dockBike(&first);
  station.dockBike(&second);

  TEST_ASSERT_EQUAL_PTR(&first, station.releaseBike());
  TEST_ASSERT_NULL(station.releaseBike());
}

void test_release_returns_bikes_in_dock_order(void) {
  firmware::Station station = {};
  station.stationId = "station-1";
  station.capacity = 2;
  firmware::Bike first{"bike-1", 100, firmware::BikeState::Available};
  firmware::Bike second{"bike-2", 100, firmware::BikeState::Available};
  firmware::Bike third{"bike-3", 100, firmware::BikeState::Available};

  station.dockBike(&first);
  station.dockBike(&second);

  TEST_ASSERT_EQUAL_PTR(&first, station.releaseBike());
  station.dockBike(&third);
  TEST_ASSERT_EQUAL_PTR(&second, station.releaseBike());
  TEST_ASSERT_EQUAL_PTR(&third, station.releaseBike());
  TEST_ASSERT_NULL(station.releaseBike());
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_dock_and_release_bike);
  RUN_TEST(test_dock_respects_capacity_and_ignores_duplicate_and_null);
  RUN_TEST(test_release_returns_bikes_in_dock_order);
  return UNITY_END();
}
