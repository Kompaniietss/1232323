#pragma once

namespace firmware {

enum class LedColor : unsigned char { Green, Yellow, Red };

class ILed {
 public:
  virtual ~ILed() = default;
  virtual void setColor(LedColor color) = 0;
  virtual void blink(bool enabled) = 0;
};

}  // namespace firmware
