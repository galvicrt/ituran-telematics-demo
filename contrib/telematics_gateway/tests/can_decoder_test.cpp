// SPDX-License-Identifier: Apache-2.0

#include <telematics/can_decoder.h>

#include <gtest/gtest.h>

using namespace telematics;

namespace
{
  CanFrame Frame(uint32_t id, uint8_t dlc, std::array<uint8_t, 8> data, uint64_t t_us = 1'000U)
  {
    CanFrame frame;
    frame.id           = id;
    frame.dlc          = dlc;
    frame.data         = data;
    frame.timestamp_us = t_us;
    return frame;
  }
}

TEST(CanDecoder, DecodesVehicleSpeed)
{
  CanDecoder decoder;
  // 2.00 km/h = 512 * 1/256 km/h
  ASSERT_TRUE(decoder.OnFrame(Frame(kCanIdVehicleSpeed, 2U, { 0x00, 0x02 })));
  EXPECT_EQ(decoder.state().speed_kph_x100, 200U);
  EXPECT_EQ(decoder.state().updated_at_us, 1'000U);
}

TEST(CanDecoder, DecodesEngineSpeed)
{
  CanDecoder decoder;
  // 3000 rpm = 12000 * 0.25 rpm
  ASSERT_TRUE(decoder.OnFrame(Frame(kCanIdEngine, 4U, { 0x00, 0x00, 0x2E, 0xE0 })));
  EXPECT_EQ(decoder.state().engine_rpm, 3000U);
}

TEST(CanDecoder, DecodesBodyStatus)
{
  CanDecoder decoder;
  ASSERT_TRUE(decoder.OnFrame(Frame(kCanIdBodyStatus, 1U, { 0x09 })));
  EXPECT_TRUE(decoder.state().ignition_on);
  EXPECT_TRUE(decoder.state().driver_door_open);
}

TEST(CanDecoder, DecodesAcceleration)
{
  CanDecoder decoder;
  // longitudinal -1500 mg, lateral +800 mg
  ASSERT_TRUE(decoder.OnFrame(Frame(kCanIdAcceleration, 4U, { 0xFA, 0x24, 0x03, 0x20 })));
  EXPECT_EQ(decoder.state().accel_long_mg, -1500);
  EXPECT_EQ(decoder.state().accel_lat_mg, 800);
}

TEST(CanDecoder, IgnoresUnknownFrames)
{
  CanDecoder decoder;
  EXPECT_FALSE(decoder.OnFrame(Frame(0x7DFU, 8U, { 0x02, 0x01, 0x0D })));
}
