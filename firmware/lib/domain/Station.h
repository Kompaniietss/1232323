#pragma once

#include "Bike.h"

namespace firmware {

// @model Station
struct Station {
  // @model Station.stationId
  const char* stationId;
  // @model Station.capacity
  int capacity;

  void dockBike(Bike* bike) {
    (void)bike;
  }

  Bike* releaseBike() {
    return nullptr;
  }
};

}  // namespace firmware
