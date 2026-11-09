#include <unity.h>

#include "../lib/protocol/Crc8.h"
#include "../lib/protocol/FrameCodec.h"

void test_crc_roundtrip(void) {
  const uint8_t payload[] = {0x01U, 0x02U, 0x03U};
  firmware::Frame frame = {};
  TEST_ASSERT_TRUE(firmware::FrameCodec::encode(0x10U, payload, 3U, &frame));
  TEST_ASSERT_EQUAL_UINT8(0x10U, frame.id);
  TEST_ASSERT_EQUAL_UINT8(3U, frame.length);
  TEST_ASSERT_EQUAL_UINT8(firmware::Crc8::compute(payload, 3U), frame.crc);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_crc_roundtrip);
  return UNITY_END();
}
