#pragma once

namespace firmware {

enum class BikeState : unsigned char {
  Available,
  Reserved,
  InUse,
  Maintenance
};

// @model Bike
class Bike {
 public:
  // @model Bike.bikeId
  const char* bikeId;
  // @model Bike.batteryLevel
  int batteryLevel;
  // @model Bike.state
  BikeState state;

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
