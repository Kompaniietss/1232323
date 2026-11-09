#pragma once

namespace firmware {

class IButton {
 public:
  virtual ~IButton() = default;
  virtual bool pressed() const = 0;
  virtual unsigned long pressDurationMs() const = 0;
};

}  // namespace firmware
