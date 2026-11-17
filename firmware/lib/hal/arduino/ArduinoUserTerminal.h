#pragma once

#include <Arduino.h>

#include "../IUserTerminal.h"

namespace firmware {

class ArduinoUserTerminal : public IUserTerminal {
 public:
  explicit ArduinoUserTerminal(HardwareSerial& serial) : serial_(serial) {}

  void showPaymentPrompt(int32_t costCents) override {
    serial_.print(F("Payment due (simulated): EUR "));
    printCost(costCents);
    serial_.println(F(". Short press to confirm."));
  }

  void showReceipt(int32_t costCents) override {
    serial_.print(F("Receipt (simulated payment): EUR "));
    printCost(costCents);
    serial_.println();
  }

  void showError(const char* message) override {
    serial_.print(F("Firmware error: "));
    serial_.println(message);
  }

 private:
  void printCost(int32_t costCents) {
    const uint32_t whole = static_cast<uint32_t>(costCents / 100);
    const uint8_t fraction = static_cast<uint8_t>(costCents % 100);
    serial_.print(whole);
    serial_.print('.');
    if (fraction < 10U) serial_.print('0');
    serial_.print(fraction);
  }

  HardwareSerial& serial_;
};

}  // namespace firmware
