#pragma once

namespace firmware {

class IClock {
 public:
  virtual ~IClock() = default;
  virtual unsigned long nowMs() const = 0;
};

}  // namespace firmware
