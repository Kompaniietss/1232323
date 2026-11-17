#include <stdint.h>

#include <Arduino.h>

#include "../lib/domain/Bike.h"
#include "../lib/domain/Pricing.h"
#include "../lib/domain/Rental.h"
#include "../lib/domain/StateMachine.h"
#include "../lib/domain/Station.h"
#include "../lib/domain/User.h"
#include "../lib/hal/arduino/ArduinoButton.h"
#include "../lib/hal/arduino/ArduinoClock.h"
#include "../lib/hal/arduino/ArduinoLed.h"
#include "../lib/hal/arduino/ArduinoLink.h"
#include "../lib/hal/arduino/ArduinoUserTerminal.h"
#include "../lib/nodes/ActuatorNode.h"
#include "../lib/nodes/SensorNode.h"

namespace {

constexpr unsigned long kBaudRate = 115200UL;

firmware::Bike bike{"bike-1", 100, firmware::BikeState::Available};
firmware::Station station{"station-1", 10};
firmware::User user{"user-001", "Demo Rider", "mock-payment"};
firmware::Rental rental{"rental-001", 0UL, 0UL, 0, nullptr, nullptr};
firmware::ArduinoButton button(2);
firmware::ArduinoLed green(5, firmware::LedColor::Green);
firmware::ArduinoLed yellow(6, firmware::LedColor::Yellow);
firmware::ArduinoLed red(7, firmware::LedColor::Red);
firmware::ArduinoClock clock;
firmware::ArduinoUserTerminal userTerminal(Serial);
firmware::ArduinoLink sensorLink(Serial1);
firmware::ArduinoLink actuatorLink(Serial2);
firmware::SensorNode sensor(&button, &sensorLink, &clock, &userTerminal);
firmware::ActuatorNode actuator(
    &bike, &green, &yellow, &red, &actuatorLink, &clock, &rental, &station);

void initializeDomainAssociations() {
  station.dockBike(&bike);
  rental.user = &user;
  rental.bike = &bike;
}

}  // namespace

void setup() {
  initializeDomainAssociations();
  Serial.begin(kBaudRate);
  Serial1.begin(kBaudRate);
  Serial2.begin(kBaudRate);
  button.begin();
  green.begin();
  yellow.begin();
  red.begin();
}

void loop() {
  if (!sensor.tick()) userTerminal.showError("Sensor flow failed.");
  if (!actuator.tick()) userTerminal.showError("Actuator flow failed.");
}
