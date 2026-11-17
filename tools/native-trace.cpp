#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../firmware/lib/domain/Bike.h"
#include "../firmware/lib/domain/Pricing.h"
#include "../firmware/lib/domain/Rental.h"
#include "../firmware/lib/domain/StateMachine.h"
#include "../firmware/lib/domain/Station.h"
#include "../firmware/lib/domain/User.h"
#include "../firmware/lib/hal/fake/FakeButton.h"
#include "../firmware/lib/hal/fake/FakeClock.h"
#include "../firmware/lib/hal/fake/FakeLed.h"
#include "../firmware/lib/hal/fake/FakeLink.h"
#include "../firmware/lib/hal/fake/FakeUserTerminal.h"
#include "../firmware/lib/nodes/ActuatorNode.h"
#include "../firmware/lib/nodes/SensorNode.h"
#include "../firmware/lib/protocol/generated/frame_ids.h"

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

struct Runtime {
  firmware::FakeButton button;
  firmware::FakeClock clock;
  firmware::FakeLink sensorLink;
  firmware::FakeLink actuatorLink;
  firmware::FakeLed green{firmware::LedColor::Green};
  firmware::FakeLed yellow{firmware::LedColor::Yellow};
  firmware::FakeLed red{firmware::LedColor::Red};
  firmware::FakeUserTerminal terminal;
  firmware::Bike bike;
  firmware::Station station{"trace-station", 10};
  firmware::User user{"trace-user", "Trace Rider", "simulated"};
  firmware::Rental rental;
  firmware::SensorNode sensor;
  firmware::ActuatorNode actuator;

  Runtime(int batteryLevel, firmware::BikeState initialState)
      : bike{"trace-bike", batteryLevel, initialState},
        rental{"trace-rental", 0UL, 0UL, 0, &user, &bike},
        sensor(&button, &sensorLink, &clock, &terminal),
        actuator(&bike, &green, &yellow, &red, &actuatorLink, &clock, &rental, &station) {
    sensorLink.connect(actuatorLink);
    actuatorLink.connect(sensorLink);
    if (initialState == firmware::BikeState::Available) station.dockBike(&bike);
    actuator.tick();
  }

  void pump() {
    for (unsigned int i = 0; i < 4U; ++i) {
      actuator.tick();
      sensor.tick();
    }
  }

  void press(unsigned long durationMs) {
    button.setPressDurationMs(durationMs);
    button.setPressed(true);
    sensor.tick();
    button.setPressed(false);
    sensor.tick();
    pump();
  }

  const char* ledColor() const {
    if (green.on()) return "Green";
    if (yellow.on()) return "Yellow";
    return "Red";
  }
};

struct TraceStep {
  bool hasFrame;
  uint8_t frameId;
  bool hasCost;
  int32_t costCents;
};

TraceStep replayInput(const char* input, Runtime* runtime, unsigned long durationMs) {
  TraceStep result{false, 0U, false, 0};
  if (strcmp(input, "rentBike") == 0) {
    const unsigned int receiptsBefore = runtime->terminal.receiptCount();
    runtime->press(250UL);
    if (runtime->actuatorLink.sentCount() > 0U) {
      result.hasFrame = true;
      result.frameId = runtime->actuatorLink.lastSentId();
    } else if (runtime->sensorLink.sentCount() > 0U) {
      result.hasFrame = true;
      result.frameId = runtime->sensorLink.lastSentId();
    }
    if (runtime->terminal.receiptCount() > receiptsBefore) {
      result.hasCost = true;
      result.costCents = runtime->terminal.receiptCents();
    }
    return result;
  }
  if (strcmp(input, "returnBike") == 0) {
    runtime->clock.advance(durationMs);
    const unsigned int receiptsBefore = runtime->terminal.receiptCount();
    runtime->press(900UL);
    runtime->press(250UL);
    runtime->press(250UL);
    if (runtime->actuatorLink.sentCount() > 0U) {
      result.hasFrame = true;
      result.frameId = runtime->actuatorLink.lastSentId();
    }
    if (runtime->terminal.receiptCount() > receiptsBefore) {
      result.hasCost = true;
      result.costCents = runtime->terminal.receiptCents();
    }
    return result;
  }
  if (strcmp(input, "reserve") == 0) {
    firmware::StateMachine::apply(&runtime->bike, firmware::BikeTrigger::Reserve);
  } else if (strcmp(input, "timeout") == 0) {
    firmware::StateMachine::apply(&runtime->bike, firmware::BikeTrigger::Timeout);
  } else if (strcmp(input, "batteryLow") == 0) {
    firmware::StateMachine::apply(&runtime->bike, firmware::BikeTrigger::BatteryLow);
  } else if (strcmp(input, "invalidReturn") == 0) {
    firmware::StateMachine::apply(&runtime->bike, firmware::BikeTrigger::ReturnToStation);
  }
  runtime->actuator.tick();
  return result;
}

void printStep(const char* input, firmware::BikeState before, Runtime* runtime,
               const TraceStep& result) {
  printf("{\"input\":\"%s\",\"stateBefore\":\"%s\",\"stateAfter\":\"%s\",\"frameId\":",
         input, stateName(before), stateName(runtime->bike.state));
  if (result.hasFrame) printf("%u", result.frameId);
  else printf("null");
  printf(",\"ledColor\":\"%s\",\"costCents\":", runtime->ledColor());
  if (result.hasCost) printf("%" PRId32, result.costCents);
  else printf("null");
  printf("}");
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 5) return 2;

  firmware::BikeState initialState;
  if (!parseState(argv[2], &initialState)) return 2;
  Runtime runtime(atoi(argv[3]), initialState);
  const unsigned long durationMs = strtoul(argv[4], nullptr, 10);

  printf("{\"scenario\":\"%s\",\"steps\":[", argv[1]);
  for (int index = 5; index < argc; ++index) {
    if (index > 5) printf(",");
    const firmware::BikeState before = runtime.bike.state;
    const TraceStep result = replayInput(argv[index], &runtime, durationMs);
    printStep(argv[index], before, &runtime, result);
  }
  printf("]}\n");
  return 0;
}
