#pragma once

#include <stdint.h>

namespace firmware {

// @model Rental
struct Rental {
  // @model Rental.rentalId
  const char* rentalId;
  // @model Rental.startTime
  unsigned long startTime;
  // @model Rental.endTime
  unsigned long endTime;
  // @model Rental.cost
  int32_t costCents;

  // @model Rental.start()
  void start() {
    startTime = 0;
    endTime = 0;
  }

  // @model Rental.end()
  void end() {
    endTime = startTime + 60000UL;
  }
};

}  // namespace firmware
