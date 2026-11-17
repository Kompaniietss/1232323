#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../firmware/lib/domain/Bike.h"
#include "../firmware/lib/domain/Pricing.h"
#include "../firmware/lib/domain/Rental.h"
#include "../firmware/lib/domain/StateMachine.h"
#include "../firmware/lib/domain/User.h"

namespace {

const char* stateName(firmware::BikeState state) {
  switch (state) {
    case firmware::BikeState::Available:
      return "Available";
    case firmware::BikeState::Reserved:
      return "Reserved";
    case firmware::BikeState::InUse:
      return "InUse";
    case firmware::BikeState::Maintenance:
      return "Maintenance";
  }
  return "Unknown";
}

bool parseState(const char* value, firmware::BikeState* state) {
  if (strcmp(value, "Available") == 0) *state = firmware::BikeState::Available;
  else if (strcmp(value, "Reserved") == 0) *state = firmware::BikeState::Reserved;
  else if (strcmp(value, "InUse") == 0) *state = firmware::BikeState::InUse;
  else if (strcmp(value, "Maintenance") == 0) *state = firmware::BikeState::Maintenance;
  else return false;
  return true;
}

void printStep(const char* input, firmware::BikeState before,
               firmware::BikeState after, bool hasCost, int32_t costCents) {
  printf("{\"input\":\"%s\",\"stateBefore\":\"%s\",\"stateAfter\":\"%s\","
         "\"frameId\":null,\"ledColor\":null,\"costCents\":",
         input, stateName(before), stateName(after));
  if (hasCost) printf("%" PRId32, costCents);
  else printf("null");
  printf("}");
}

bool replayInput(const char* input, firmware::Bike* bike, firmware::Rental* rental,
                 unsigned long durationMs, bool* hasCost, int32_t* costCents) {
  *hasCost = false;
  if (strcmp(input, "rentBike") == 0) {
    if (firmware::StateMachine::apply(bike, firmware::BikeTrigger::Unlock)) {
      rental->start();
      return true;
    }
    return false;
  }
  if (strcmp(input, "returnBike") == 0) {
    if (!firmware::StateMachine::apply(bike, firmware::BikeTrigger::ReturnToStation)) {
      return false;
    }
    rental->end();
    *costCents = firmware::Pricing::calculate(durationMs);
    rental->costCents = *costCents;
    *hasCost = true;
    return true;
  }
  if (strcmp(input, "reserve") == 0) {
    return firmware::StateMachine::apply(bike, firmware::BikeTrigger::Reserve);
  }
  if (strcmp(input, "timeout") == 0) {
    return firmware::StateMachine::apply(bike, firmware::BikeTrigger::Timeout);
  }
  if (strcmp(input, "batteryLow") == 0) {
    return firmware::StateMachine::apply(bike, firmware::BikeTrigger::BatteryLow);
  }
  if (strcmp(input, "invalidReturn") == 0) {
    return firmware::StateMachine::apply(bike, firmware::BikeTrigger::ReturnToStation);
  }
  return false;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 5) return 2;

  firmware::BikeState initialState;
  if (!parseState(argv[2], &initialState)) return 2;
  firmware::Bike bike{"trace-bike", atoi(argv[3]), initialState};
  firmware::User user{"trace-user", "Trace Rider", "opaque"};
  firmware::Rental rental{"trace-rental", 0UL, 0UL, 0, &user, &bike};
  const unsigned long durationMs = strtoul(argv[4], nullptr, 10);

  printf("{\"scenario\":\"%s\",\"steps\":[", argv[1]);
  for (int index = 5; index < argc; ++index) {
    if (index > 5) printf(",");
    const firmware::BikeState before = bike.state;
    bool hasCost = false;
    int32_t costCents = 0;
    replayInput(argv[index], &bike, &rental, durationMs, &hasCost, &costCents);
    printStep(argv[index], before, bike.state, hasCost, costCents);
  }
  printf("]}\n");
  return 0;
}
