#include <Arduino.h>
#include <stdint.h>

#include "../lib/domain/Bike.h"
#include "../lib/domain/Pricing.h"
#include "../lib/domain/StateMachine.h"
#include "../lib/protocol/FrameCodec.h"

namespace {

constexpr uint16_t kIterations = 1000U;
volatile uint32_t benchmarkSink = 0U;

float benchmarkTransitions() {
  firmware::Bike bike{"bench", 100, firmware::BikeState::Available};
  uint16_t successfulTransitions = 0U;
  const unsigned long startedAt = micros();

  for (uint16_t i = 0U; i < kIterations; ++i) {
    const firmware::BikeTrigger trigger =
        bike.state == firmware::BikeState::Available
            ? firmware::BikeTrigger::Reserve
            : firmware::BikeTrigger::Timeout;
    if (firmware::StateMachine::apply(&bike, trigger)) ++successfulTransitions;
  }

  const unsigned long elapsed = micros() - startedAt;
  benchmarkSink = successfulTransitions + static_cast<uint8_t>(bike.state);
  return static_cast<float>(elapsed) / static_cast<float>(kIterations);
}

float benchmarkFrameCodec() {
  uint8_t payload[4] = {0U, 1U, 2U, 3U};
  uint8_t decoded[4] = {};
  firmware::Frame frame{};
  uint32_t checksum = 0U;
  const unsigned long startedAt = micros();

  for (uint16_t i = 0U; i < kIterations; ++i) {
    payload[0] = static_cast<uint8_t>(i);
    payload[1] = static_cast<uint8_t>(i >> 8U);
    if (firmware::FrameCodec::encode(0x42U, payload, sizeof(payload), &frame) &&
        firmware::FrameCodec::decode(&frame, decoded, sizeof(decoded))) {
      checksum += decoded[0] + decoded[1] + decoded[2] + decoded[3];
    }
  }

  const unsigned long elapsed = micros() - startedAt;
  benchmarkSink = checksum;
  return static_cast<float>(elapsed) / static_cast<float>(kIterations);
}

float benchmarkPricing() {
  uint32_t totalCents = 0U;
  const unsigned long startedAt = micros();

  for (uint16_t i = 0U; i < kIterations; ++i) {
    const unsigned long durationMs = static_cast<unsigned long>(i) * 1000UL;
    totalCents += static_cast<uint32_t>(firmware::Pricing::calculate(durationMs));
  }

  const unsigned long elapsed = micros() - startedAt;
  benchmarkSink = totalCents;
  return static_cast<float>(elapsed) / static_cast<float>(kIterations);
}

void printMetric(const char* name, float microsecondsPerIteration) {
  Serial.print("BENCH ");
  Serial.print(name);
  Serial.print("_us=");
  Serial.print(microsecondsPerIteration, 2);
  Serial.print(" n=");
  Serial.println(kIterations);
}

}  // namespace

void setup() {
  Serial.begin(115200UL);

  printMetric("transition", benchmarkTransitions());
  printMetric("framecodec_encode_decode", benchmarkFrameCodec());
  printMetric("pricing_calculate", benchmarkPricing());
}

void loop() {}
