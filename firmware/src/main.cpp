#include <stdint.h>

#include "../lib/domain/Bike.h"
#include "../lib/domain/StateMachine.h"
#include "../lib/hal/arduino/ArduinoButton.h"
#include "../lib/hal/arduino/ArduinoLed.h"
#include "../lib/hal/arduino/ArduinoLink.h"
#include "../lib/nodes/ActuatorNode.h"
#include "../lib/nodes/SensorNode.h"

int main() {
  firmware::Bike bike {"bike-1", 100, firmware::BikeState::Available};
  firmware::ArduinoButton button(2);
  firmware::ArduinoLed green(5);
  firmware::ArduinoLed yellow(6);
  firmware::ArduinoLed red(7);
  firmware::ArduinoLink link;

  firmware::SensorNode sensor(&button, &link);
  firmware::ActuatorNode actuator(&bike, &green, &yellow, &red, &link);

  (void)sensor;
  (void)actuator;
  return 0;
}
