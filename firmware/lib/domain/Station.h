#pragma once

#include "Bike.h"

namespace firmware {

// @model Station
struct Station {
  // @model Station.stationId
  const char* stationId;
  // @model Station.capacity
  int capacity;

  // @model Station.dockBike()
  void dockBike(Bike* bike) {
    (void)bike;
  }

  // @model Station.releaseBike()
  Bike* releaseBike() {
    return nullptr;
  }
};

}  // namespace firmware
