#pragma once

#include "generated/transition_table.h"

namespace firmware {

// @model Bike
class Bike {
 public:
  // @model Bike.bikeId
  const char* bikeId;
  // @model Bike.batteryLevel
  int batteryLevel;
  // @model Bike.state
  // @model transition:t0:initial
  BikeState state = BikeState::Available;

  // @model Bike.lock()
  void lock() {
    state = BikeState::Available;
  }

  // @model Bike.unlock()
  void unlock() {
    if (state == BikeState::Available || state == BikeState::Reserved) {
      state = BikeState::InUse;
    }
  }
};

}  // namespace firmware
