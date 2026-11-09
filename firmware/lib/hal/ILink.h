#pragma once

namespace firmware {

class ILink {
 public:
  virtual ~ILink() = default;
  virtual bool send(uint8_t id, const uint8_t* payload, unsigned int len) = 0;
  virtual bool receive(uint8_t* id, uint8_t* payload, unsigned int maxLen) = 0;
};

}  // namespace firmware
