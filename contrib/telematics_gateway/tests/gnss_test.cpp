// SPDX-License-Identifier: Apache-2.0

#include <telematics/gnss.h>

#include <gtest/gtest.h>

#include <vector>

using namespace telematics;

namespace
{
  class FakeClock : public Clock
  {
  public:
    uint64_t MonotonicUs() const override { return 5'000'000U; }
    uint64_t WallMs() const override { return 0U; }
  };

  void PutLe32(std::vector<uint8_t>& buf, std::size_t offset, uint32_t value)
  {
    for (std::size_t i = 0U; i < 4U; ++i)
    {
      buf[offset + i] = static_cast<uint8_t>(value >> (8U * i));
    }
  }

  std::vector<uint8_t> BuildNavPvt(uint32_t itow_ms, int32_t lat_e7, int32_t lon_e7)
  {
    std::vector<uint8_t> payload(kUbxNavPvtSize, 0U);
    PutLe32(payload, 0U, itow_ms);
    payload[20] = 3U;     // 3D fix
    payload[21] = 0x01U;  // gnssFixOK
    payload[23] = 12U;    // satellites
    PutLe32(payload, 24U, static_cast<uint32_t>(lon_e7));
    PutLe32(payload, 28U, static_cast<uint32_t>(lat_e7));
    PutLe32(payload, 40U, 1500U);

    std::vector<uint8_t> frame = { kUbxSync1, kUbxSync2, kUbxClassNav, kUbxIdNavPvt,
                                   static_cast<uint8_t>(kUbxNavPvtSize & 0xFFU),
                                   static_cast<uint8_t>(kUbxNavPvtSize >> 8U) };
    frame.insert(frame.end(), payload.begin(), payload.end());

    uint8_t ck_a = 0U;
    uint8_t ck_b = 0U;
    for (std::size_t i = 2U; i < frame.size(); ++i)
    {
      ck_a = static_cast<uint8_t>(ck_a + frame[i]);
      ck_b = static_cast<uint8_t>(ck_b + ck_a);
    }
    frame.push_back(ck_a);
    frame.push_back(ck_b);
    return frame;
  }
}

TEST(UbxPositionDecoder, DecodesNavPvt)
{
  FakeClock          clock;
  UbxPositionDecoder decoder(clock);

  const std::vector<uint8_t> frame = BuildNavPvt(100'000U, 320'853'000, 348'818'000);
  const auto report = decoder.OnDatagram(frame.data(), frame.size());

  ASSERT_TRUE(report.has_value());
  EXPECT_EQ(report->lat_deg_e7, 320'853'000);
  EXPECT_EQ(report->lon_deg_e7, 348'818'000);
  EXPECT_EQ(report->num_sv, 12U);
  EXPECT_EQ(report->h_acc_mm, 1500U);
  EXPECT_EQ(report->received_at_us, 5'000'000U);
}

TEST(UbxPositionDecoder, IgnoresRepeatedSolution)
{
  FakeClock          clock;
  UbxPositionDecoder decoder(clock);

  const std::vector<uint8_t> frame = BuildNavPvt(100'000U, 320'853'000, 348'818'000);
  ASSERT_TRUE(decoder.OnDatagram(frame.data(), frame.size()).has_value());
  EXPECT_FALSE(decoder.OnDatagram(frame.data(), frame.size()).has_value());
}

TEST(UbxPositionDecoder, IgnoresNonUbxData)
{
  FakeClock          clock;
  UbxPositionDecoder decoder(clock);

  const std::vector<uint8_t> nmea = { '$', 'G', 'P', 'G', 'G', 'A', ',', '1', '2' };
  EXPECT_FALSE(decoder.OnDatagram(nmea.data(), nmea.size()).has_value());
}
