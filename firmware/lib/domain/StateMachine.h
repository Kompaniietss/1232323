#pragma once

#include "Bike.h"
#include "generated/transition_table.h"

namespace firmware {

// @model state:Available
// @model state:Reserved
// @model state:InUse
// @model state:Maintenance
// @model t1
// @model t2
// @model t3
// @model t4
// @model t5
// @model t6
// @model t7
// @model t8
class StateMachine {
 public:
  static bool apply(Bike* bike, BikeTrigger trigger) {
    if (bike == nullptr) {
      return false;
    }
    for (const TransitionEntry& entry : kBikeTransitions) {
      if (entry.trigger == trigger) {
        if (bike->state == entry.from) {
          bike->state = entry.to;
          return true;
        }
      }
    }
    return false;
  }
};

}  // namespace firmware
