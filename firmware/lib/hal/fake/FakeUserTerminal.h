#pragma once

#include "../IUserTerminal.h"

namespace firmware {

class FakeUserTerminal : public IUserTerminal {
 public:
  void showPaymentPrompt(int32_t costCents) override {
    paymentPromptCents_ = costCents;
    ++paymentPromptCount_;
  }

  void showReceipt(int32_t costCents) override {
    receiptCents_ = costCents;
    ++receiptCount_;
  }

  void showError(const char* message) override {
    lastError_ = message;
    ++errorCount_;
  }

  int32_t paymentPromptCents() const { return paymentPromptCents_; }
  int32_t receiptCents() const { return receiptCents_; }
  unsigned int paymentPromptCount() const { return paymentPromptCount_; }
  unsigned int receiptCount() const { return receiptCount_; }
  const char* lastError() const { return lastError_; }
  unsigned int errorCount() const { return errorCount_; }

 private:
  int32_t paymentPromptCents_ = 0;
  int32_t receiptCents_ = 0;
  unsigned int paymentPromptCount_ = 0;
  unsigned int receiptCount_ = 0;
  const char* lastError_ = "";
  unsigned int errorCount_ = 0;
};

}  // namespace firmware
