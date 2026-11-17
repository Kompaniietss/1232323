#include <stdint.h>

#include <Arduino.h>

#include "../lib/domain/Bike.h"
#include "../lib/domain/Pricing.h"
#include "../lib/domain/StateMachine.h"
#include "../lib/hal/arduino/ArduinoButton.h"
#include "../lib/hal/arduino/ArduinoLed.h"
#include "../lib/hal/arduino/ArduinoLink.h"
#include "../lib/nodes/ActuatorNode.h"
#include "../lib/nodes/SensorNode.h"

namespace {

constexpr unsigned long kBaudRate = 115200UL;

firmware::Bike bike{"bike-1", 100, firmware::BikeState::Available};
firmware::ArduinoButton button(2);
firmware::ArduinoLed green(5, firmware::LedColor::Green);
firmware::ArduinoLed yellow(6, firmware::LedColor::Yellow);
firmware::ArduinoLed red(7, firmware::LedColor::Red);
firmware::ArduinoLink sensorLink(Serial1);
firmware::ArduinoLink actuatorLink(Serial2);
firmware::SensorNode sensor(&button, &sensorLink);
firmware::ActuatorNode actuator(&bike, &green, &yellow, &red, &actuatorLink);

}  // namespace

void setup() {
  Serial.begin(kBaudRate);
  Serial1.begin(kBaudRate);
  Serial2.begin(kBaudRate);
  button.begin();
  green.begin();
  yellow.begin();
  red.begin();
}

void loop() {
  sensor.tick();
  actuator.tick();
}
