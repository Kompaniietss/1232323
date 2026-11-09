#pragma once
#include <stdint.h>
#include "../Progmem.h"

namespace firmware {

enum class BikeState : uint8_t {
  // @model state:Available
  Available,
  // @model state:Reserved
  Reserved,
  // @model state:InUse
  InUse,
  // @model state:Maintenance
  Maintenance
};

enum class BikeTrigger : uint8_t {
  Reserve,
  Timeout,
  Cancel,
  Unlock,
  ReturnToStation,
  ReportFault,
  BatteryLow,
  Repaired
};

struct TransitionEntry {
  const char* id;
  BikeState from;
  BikeState to;
  BikeTrigger trigger;
};

inline constexpr TransitionEntry kBikeTransitions[] FIRMWARE_PROGMEM = {
  // @model transition:t1:reserve()
  {"t1", BikeState::Available, BikeState::Reserved, BikeTrigger::Reserve },
  // @model transition:t2:timeout
  {"t2", BikeState::Reserved, BikeState::Available, BikeTrigger::Timeout },
  // @model transition:t2:cancel()
  {"t2", BikeState::Reserved, BikeState::Available, BikeTrigger::Cancel },
  // @model transition:t3:unlock()
  {"t3", BikeState::Reserved, BikeState::InUse, BikeTrigger::Unlock },
  // @model transition:t4:unlock()
  {"t4", BikeState::Available, BikeState::InUse, BikeTrigger::Unlock },
  // @model transition:t5:returnToStation()
  {"t5", BikeState::InUse, BikeState::Available, BikeTrigger::ReturnToStation },
  // @model transition:t6:reportFault()
  {"t6", BikeState::InUse, BikeState::Maintenance, BikeTrigger::ReportFault },
  // @model transition:t7:batteryLow()
  {"t7", BikeState::Available, BikeState::Maintenance, BikeTrigger::BatteryLow },
  // @model transition:t8:repaired()
  {"t8", BikeState::Maintenance, BikeState::Available, BikeTrigger::Repaired }
};

}  // namespace firmware
