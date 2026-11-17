#pragma once

#include "Bike.h"

namespace firmware {

constexpr int kMaxStationBikes = 10;

// @model Station
struct Station {
  // @model Station.stationId
  const char* stationId;
  // @model Station.capacity
  int capacity;

  // @model relationship:e3
  Bike* bikes[kMaxStationBikes] = {};

  // @model Station.dockBike()
  void dockBike(Bike* bike) {
    if (bike == nullptr || capacity <= 0) {
      return;
    }

    const int limit = capacity < kMaxStationBikes ? capacity : kMaxStationBikes;
    for (int i = 0; i < limit; ++i) {
      if (bikes[i] == bike) {
        return;
      }
    }
    for (int i = 0; i < limit; ++i) {
      if (bikes[i] == nullptr) {
        bikes[i] = bike;
        return;
      }
    }
  }

  // @model Station.releaseBike()
  Bike* releaseBike() {
    for (int i = 0; i < kMaxStationBikes; ++i) {
      if (bikes[i] != nullptr) {
        Bike* bike = bikes[i];
        for (int j = i; j < kMaxStationBikes - 1; ++j) {
          bikes[j] = bikes[j + 1];
        }
        bikes[kMaxStationBikes - 1] = nullptr;
        return bike;
      }
    }
    return nullptr;
  }
};

}  // namespace firmware
