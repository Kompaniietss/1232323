#pragma once

#include <stdint.h>

namespace firmware {

class IUserTerminal {
 public:
  virtual ~IUserTerminal() = default;
  virtual void showPaymentPrompt(int32_t costCents) = 0;
  virtual void showReceipt(int32_t costCents) = 0;
  virtual void showError(const char* message) = 0;
};

}  // namespace firmware
