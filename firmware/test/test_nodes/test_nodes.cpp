#include <unity.h>

#include "../../lib/domain/Bike.h"
#include "../../lib/domain/Rental.h"
#include "../../lib/domain/Station.h"
#include "../../lib/domain/User.h"
#include "../../lib/hal/fake/FakeButton.h"
#include "../../lib/hal/fake/FakeClock.h"
#include "../../lib/hal/fake/FakeLed.h"
#include "../../lib/hal/fake/FakeLink.h"
#include "../../lib/hal/fake/FakeUserTerminal.h"
#include "../../lib/nodes/ActuatorNode.h"
#include "../../lib/nodes/SensorNode.h"
#include "../../lib/protocol/generated/frame_ids.h"

namespace {

struct Fixture {
  firmware::FakeButton button;
  firmware::FakeClock clock;
  firmware::FakeLink sensorLink;
  firmware::FakeLink actuatorLink;
  firmware::FakeLed green{firmware::LedColor::Green};
  firmware::FakeLed yellow{firmware::LedColor::Yellow};
  firmware::FakeLed red{firmware::LedColor::Red};
  firmware::FakeUserTerminal terminal;
  firmware::Bike bike{"bike-1", 100, firmware::BikeState::Available};
  firmware::Station station{"station-1", 10};
  firmware::User user{"user-1", "Rider", "simulated"};
  firmware::Rental rental{"rental-1", 0UL, 0UL, 0, &user, &bike};
  firmware::SensorNode sensor{&button, &sensorLink, &clock, &terminal};
  firmware::ActuatorNode actuator{
      &bike, &green, &yellow, &red, &actuatorLink, &clock, &rental, &station};

  Fixture() {
    sensorLink.connect(actuatorLink);
    actuatorLink.connect(sensorLink);
    station.dockBike(&bike);
  }

  void press(unsigned long durationMs) {
    button.setPressDurationMs(durationMs);
    button.setPressed(true);
    sensor.tick();
    button.setPressed(false);
    sensor.tick();
  }

  void pump() {
    for (unsigned int i = 0; i < 4U; ++i) {
      actuator.tick();
      sensor.tick();
    }
  }
};

void test_rent_messages_drive_reserve_unlock_receipt_and_show_receipt(void) {
  Fixture fixture;

  fixture.press(250UL);
  fixture.actuator.tick();
  TEST_ASSERT_EQUAL(firmware::BikeState::Reserved, fixture.bike.state);
  TEST_ASSERT_EQUAL(firmware::LedColor::Yellow, fixture.yellow.color());
  TEST_ASSERT_TRUE(fixture.yellow.on());
  TEST_ASSERT_FALSE(fixture.green.on());
  TEST_ASSERT_FALSE(fixture.red.on());
  TEST_ASSERT_EQUAL_UINT8(
      static_cast<uint8_t>(firmware::FrameId::RENT_RESERVED), fixture.actuatorLink.sentId(0U));
  fixture.sensor.tick();
  fixture.actuator.tick();
  fixture.sensor.tick();

  TEST_ASSERT_EQUAL(firmware::BikeState::InUse, fixture.bike.state);
  TEST_ASSERT_NULL(fixture.station.bikes[0]);
  TEST_ASSERT_EQUAL(firmware::LedColor::Red, fixture.red.color());
  TEST_ASSERT_TRUE(fixture.red.on());
  TEST_ASSERT_FALSE(fixture.green.on());
  TEST_ASSERT_FALSE(fixture.yellow.on());
  TEST_ASSERT_EQUAL_UINT32(100U, fixture.terminal.receiptCents());
  TEST_ASSERT_EQUAL_UINT32(1U, fixture.terminal.receiptCount());
  TEST_ASSERT_EQUAL_UINT8(
      static_cast<uint8_t>(firmware::FrameId::RENT_REQUEST), fixture.sensorLink.sentId(0U));
  TEST_ASSERT_EQUAL_UINT8(
      static_cast<uint8_t>(firmware::FrameId::RENT_UNLOCK), fixture.sensorLink.sentId(1U));
  TEST_ASSERT_EQUAL_UINT8(
      static_cast<uint8_t>(firmware::FrameId::RENT_RESERVED), fixture.actuatorLink.sentId(0U));
  TEST_ASSERT_EQUAL_UINT8(
      static_cast<uint8_t>(firmware::FrameId::RENT_STARTED), fixture.actuatorLink.sentId(1U));
  TEST_ASSERT_EQUAL_UINT8(
      static_cast<uint8_t>(firmware::FrameId::RENT_ACTIVE), fixture.actuatorLink.sentId(2U));
  TEST_ASSERT_EQUAL_UINT8(
      static_cast<uint8_t>(firmware::FrameId::RENT_RECEIPT), fixture.actuatorLink.sentId(3U));
  TEST_ASSERT_EQUAL_UINT32(0U, fixture.terminal.errorCount());

  fixture.press(250UL);
  fixture.clock.advance(firmware::SensorNode::kDoublePressWindowMs - 50UL);
  fixture.press(250UL);
  fixture.pump();

  TEST_ASSERT_EQUAL_UINT8(
      static_cast<uint8_t>(firmware::FrameId::RENT_SHOWRECEIPT), fixture.sensorLink.sentId(2U));
  TEST_ASSERT_EQUAL_UINT32(2U, fixture.terminal.receiptCount());
}

void test_return_prompts_confirms_receipt_closes_and_replies(void) {
  Fixture fixture;
  fixture.press(250UL);
  fixture.pump();
  fixture.clock.advance(90000UL);

  fixture.press(900UL);
  fixture.actuator.tick();
  fixture.sensor.tick();
  TEST_ASSERT_EQUAL(firmware::BikeState::InUse, fixture.bike.state);
  TEST_ASSERT_EQUAL_UINT32(130U, fixture.terminal.paymentPromptCents());
  TEST_ASSERT_EQUAL_UINT32(1U, fixture.terminal.paymentPromptCount());
  TEST_ASSERT_EQUAL_UINT8(
      static_cast<uint8_t>(firmware::FrameId::RETURN_DOCKED), fixture.actuatorLink.sentId(4U));
  TEST_ASSERT_EQUAL_UINT8(
      static_cast<uint8_t>(firmware::FrameId::RETURN_PAYMENTREQUEST),
      fixture.actuatorLink.sentId(5U));

  fixture.press(250UL);
  fixture.actuator.tick();
  fixture.sensor.tick();
  TEST_ASSERT_EQUAL(firmware::BikeState::Available, fixture.bike.state);
  TEST_ASSERT_EQUAL(firmware::LedColor::Green, fixture.green.color());
  TEST_ASSERT_TRUE(fixture.green.on());
  TEST_ASSERT_FALSE(fixture.red.on());
  TEST_ASSERT_EQUAL_PTR(&fixture.bike, fixture.station.bikes[0]);
  TEST_ASSERT_EQUAL_UINT32(130U, fixture.terminal.receiptCents());
  TEST_ASSERT_EQUAL_UINT32(2U, fixture.terminal.receiptCount());
  TEST_ASSERT_EQUAL_UINT8(
      static_cast<uint8_t>(firmware::FrameId::RETURN_AVAILABLE), fixture.actuatorLink.sentId(6U));
  TEST_ASSERT_EQUAL_UINT8(
      static_cast<uint8_t>(firmware::FrameId::RETURN_RECEIPT), fixture.actuatorLink.sentId(7U));

  fixture.press(250UL);
  fixture.actuator.tick();
  fixture.sensor.tick();
  TEST_ASSERT_EQUAL_UINT8(
      static_cast<uint8_t>(firmware::FrameId::RETURN_CLOSE), fixture.sensorLink.sentId(4U));
  TEST_ASSERT_EQUAL_UINT8(
      static_cast<uint8_t>(firmware::FrameId::RETURN_ENDED), fixture.actuatorLink.sentId(8U));
  TEST_ASSERT_EQUAL_UINT32(0U, fixture.terminal.errorCount());
}

}  // namespace

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_rent_messages_drive_reserve_unlock_receipt_and_show_receipt);
  RUN_TEST(test_return_prompts_confirms_receipt_closes_and_replies);
  return UNITY_END();
}
