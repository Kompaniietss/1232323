#pragma once
#include <stdint.h>

namespace firmware {

enum class BikeState : uint8_t {
  Available,
  Reserved,
  InUse,
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

inline constexpr TransitionEntry kBikeTransitions[] = {
  {"t1", BikeState::Available, BikeState::Reserved, BikeTrigger::Reserve },
  {"t2", BikeState::Reserved, BikeState::Available, BikeTrigger::Timeout },
  {"t2", BikeState::Reserved, BikeState::Available, BikeTrigger::Cancel },
  {"t3", BikeState::Reserved, BikeState::InUse, BikeTrigger::Unlock },
  {"t4", BikeState::Available, BikeState::InUse, BikeTrigger::Unlock },
  {"t5", BikeState::InUse, BikeState::Available, BikeTrigger::ReturnToStation },
  {"t6", BikeState::InUse, BikeState::Maintenance, BikeTrigger::ReportFault },
  {"t7", BikeState::Available, BikeState::Maintenance, BikeTrigger::BatteryLow },
  {"t8", BikeState::Maintenance, BikeState::Available, BikeTrigger::Repaired }
};

}  // namespace firmware
