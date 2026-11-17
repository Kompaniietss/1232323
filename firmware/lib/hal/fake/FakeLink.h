#pragma once

#include "../ILink.h"

namespace firmware {

class FakeLink : public ILink {
 public:
  static constexpr unsigned int kQueueCapacity = 16U;
  static constexpr unsigned int kHistoryCapacity = 64U;

  void connect(FakeLink& peer) { peer_ = &peer; }

  bool send(uint8_t id, const uint8_t* payload, unsigned int len) override {
    if (peer_ == nullptr || len > 15U || (len > 0U && payload == nullptr) ||
        peer_->count_ == kQueueCapacity) {
      return false;
    }
    Packet& packet = peer_->packets_[peer_->tail_];
    packet.id = id;
    packet.length = static_cast<uint8_t>(len);
    for (unsigned int i = 0; i < len; ++i) packet.payload[i] = payload[i];
    peer_->tail_ = (peer_->tail_ + 1U) % kQueueCapacity;
    ++peer_->count_;
    if (sentCount_ < kHistoryCapacity) sentIds_[sentCount_] = id;
    ++sentCount_;
    lastSentId_ = id;
    return true;
  }

  bool receive(uint8_t* id, uint8_t* payload, unsigned int maxLen,
               unsigned int* receivedLen) override {
    if (id == nullptr || payload == nullptr || receivedLen == nullptr || count_ == 0U) {
      return false;
    }
    const Packet& packet = packets_[head_];
    if (maxLen < packet.length) return false;
    *id = packet.id;
    *receivedLen = packet.length;
    for (uint8_t i = 0; i < packet.length; ++i) payload[i] = packet.payload[i];
    head_ = (head_ + 1U) % kQueueCapacity;
    --count_;
    return true;
  }

  uint8_t lastSentId() const { return lastSentId_; }
  unsigned int sentCount() const { return sentCount_; }
  uint8_t sentId(unsigned int index) const {
    return index < sentCount_ && index < kHistoryCapacity ? sentIds_[index] : 0U;
  }

 private:
  struct Packet {
    uint8_t id = 0U;
    uint8_t length = 0U;
    uint8_t payload[15] = {};
  };

  FakeLink* peer_ = nullptr;
  Packet packets_[kQueueCapacity] = {};
  unsigned int head_ = 0U;
  unsigned int tail_ = 0U;
  unsigned int count_ = 0U;
  uint8_t sentIds_[kHistoryCapacity] = {};
  unsigned int sentCount_ = 0U;
  uint8_t lastSentId_ = 0U;
};

}  // namespace firmware
