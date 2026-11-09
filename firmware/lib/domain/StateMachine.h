#pragma once

#include "Bike.h"

#include <stddef.h>

namespace firmware {

class StateMachine {
 public:
  static bool apply(Bike* bike, BikeTrigger trigger) {
    if (bike == nullptr) {
      return false;
    }
    for (size_t i = 0; i < sizeof(kBikeTransitions) / sizeof(kBikeTransitions[0]); ++i) {
      const uint8_t* entry = reinterpret_cast<const uint8_t*>(&kBikeTransitions[i]);
      const BikeTrigger entryTrigger = static_cast<BikeTrigger>(
          FIRMWARE_READ_BYTE(entry + offsetof(TransitionEntry, trigger)));
      if (entryTrigger == trigger) {
        const BikeState from = static_cast<BikeState>(
            FIRMWARE_READ_BYTE(entry + offsetof(TransitionEntry, from)));
        if (bike->state == from) {
          bike->state = static_cast<BikeState>(
              FIRMWARE_READ_BYTE(entry + offsetof(TransitionEntry, to)));
          return true;
        }
      }
    }
    return false;
  }
};

}  // namespace firmware
