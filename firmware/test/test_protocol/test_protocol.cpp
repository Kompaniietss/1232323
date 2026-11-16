#include <unity.h>

#include "../../lib/protocol/Crc8.h"
#include "../../lib/protocol/FrameCodec.h"

using firmware::Crc8;
using firmware::Frame;
using firmware::FrameCodec;

void test_crc8_standard_check_vector(void) {
  const uint8_t data[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
  TEST_ASSERT_EQUAL_UINT8(0xF4U, Crc8::compute(data, 9U));
}

void test_crc8_known_value_for_three_bytes(void) {
  const uint8_t data[] = {0x01U, 0x02U, 0x03U};
  TEST_ASSERT_EQUAL_UINT8(0x48U, Crc8::compute(data, 3U));
}

void test_encode_sets_crc_over_header_and_payload(void) {
  const uint8_t payload[] = {0x01U, 0x02U, 0x03U};
  const uint8_t covered[] = {0x10U, 0x03U, 0x01U, 0x02U, 0x03U};
  Frame frame = {};
  TEST_ASSERT_TRUE(FrameCodec::encode(0x10U, payload, 3U, &frame));
  TEST_ASSERT_EQUAL_UINT8(0x10U, frame.id);
  TEST_ASSERT_EQUAL_UINT8(3U, frame.length);
  TEST_ASSERT_EQUAL_UINT8(Crc8::compute(covered, 5U), frame.crc);
}

void test_encode_decode_roundtrip(void) {
  const uint8_t payload[] = {0xAAU, 0xBBU};
  uint8_t out[16] = {};
  Frame frame = {};
  TEST_ASSERT_TRUE(FrameCodec::encode(0x21U, payload, 2U, &frame));
  TEST_ASSERT_TRUE(FrameCodec::decode(&frame, out, 16U));
  TEST_ASSERT_EQUAL_UINT8_ARRAY(payload, out, 2U);
}

void test_decode_rejects_corrupted_crc_and_payload(void) {
  const uint8_t payload[] = {0x01U, 0x02U};
  uint8_t out[16] = {};
  Frame frame = {};
  TEST_ASSERT_TRUE(FrameCodec::encode(0x05U, payload, 2U, &frame));
  frame.crc ^= 0x01U;
  TEST_ASSERT_FALSE(FrameCodec::decode(&frame, out, 16U));
  TEST_ASSERT_TRUE(FrameCodec::encode(0x05U, payload, 2U, &frame));
  frame.payload[0] ^= 0x80U;
  TEST_ASSERT_FALSE(FrameCodec::decode(&frame, out, 16U));
}

void test_encode_rejects_oversized_payload(void) {
  uint8_t big[16] = {};
  Frame frame = {};
  TEST_ASSERT_FALSE(FrameCodec::encode(0x01U, big, 16U, &frame));
  TEST_ASSERT_FALSE(FrameCodec::encode(0x01U, big, 1U, nullptr));
}

void test_serialize_parse_roundtrip(void) {
  const uint8_t payload[] = {0x11U, 0x22U, 0x33U};
  uint8_t wire[FrameCodec::kMaxWireBytes] = {};
  Frame sent = {};
  Frame received = {};
  TEST_ASSERT_TRUE(FrameCodec::encode(0x42U, payload, 3U, &sent));
  const uint8_t size = FrameCodec::serialize(sent, wire, sizeof(wire));
  TEST_ASSERT_EQUAL_UINT8(7U, size);
  TEST_ASSERT_EQUAL_UINT8(0xA5U, wire[0]);
  TEST_ASSERT_TRUE(FrameCodec::parse(wire, size, &received));
  TEST_ASSERT_EQUAL_UINT8(0x42U, received.id);
  TEST_ASSERT_EQUAL_UINT8(3U, received.length);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(payload, received.payload, 3U);
}

void test_parse_rejects_bad_start_truncated_and_bad_crc(void) {
  const uint8_t payload[] = {0x01U};
  uint8_t wire[FrameCodec::kMaxWireBytes] = {};
  Frame frame = {};
  Frame parsed = {};
  TEST_ASSERT_TRUE(FrameCodec::encode(0x07U, payload, 1U, &frame));
  const uint8_t size = FrameCodec::serialize(frame, wire, sizeof(wire));
  TEST_ASSERT_EQUAL_UINT8(5U, size);

  wire[0] = 0x00U;
  TEST_ASSERT_FALSE(FrameCodec::parse(wire, size, &parsed));
  wire[0] = FrameCodec::kStartByte;

  TEST_ASSERT_FALSE(FrameCodec::parse(wire, static_cast<uint8_t>(size - 1U), &parsed));

  wire[size - 1U] ^= 0xFFU;
  TEST_ASSERT_FALSE(FrameCodec::parse(wire, size, &parsed));
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_crc8_standard_check_vector);
  RUN_TEST(test_crc8_known_value_for_three_bytes);
  RUN_TEST(test_encode_sets_crc_over_header_and_payload);
  RUN_TEST(test_encode_decode_roundtrip);
  RUN_TEST(test_decode_rejects_corrupted_crc_and_payload);
  RUN_TEST(test_encode_rejects_oversized_payload);
  RUN_TEST(test_serialize_parse_roundtrip);
  RUN_TEST(test_parse_rejects_bad_start_truncated_and_bad_crc);
  return UNITY_END();
}
