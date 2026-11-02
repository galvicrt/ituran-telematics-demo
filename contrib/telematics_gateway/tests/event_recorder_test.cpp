// SPDX-License-Identifier: Apache-2.0

#include <telematics/event_recorder.h>

#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>
#include <string>

using namespace telematics;

namespace
{
  VehicleState Sample(uint64_t t_us, int16_t accel_long_mg)
  {
    VehicleState state;
    state.updated_at_us  = t_us;
    state.speed_kph_x100 = 5000U;
    state.accel_long_mg  = accel_long_mg;
    state.ignition_on    = true;
    return state;
  }

  std::streamoff FileSize(const std::string& path)
  {
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    return in ? static_cast<std::streamoff>(in.tellg()) : -1;
  }
}

TEST(EventRecorder, NoRecordWithoutImpact)
{
  EventRecorder recorder(::testing::TempDir());
  for (uint64_t i = 0U; i < 300U; ++i)
  {
    recorder.OnVehicleState(Sample(i * 10'000U, 200));
  }
  EXPECT_FALSE(recorder.PollAndPersist());
}

TEST(EventRecorder, WritesRecordAfterPostTriggerWindow)
{
  const std::string dir = ::testing::TempDir();
  std::remove((dir + "/crash_0.bin").c_str());

  EventRecorder recorder(dir);
  for (uint64_t i = 0U; i < 100U; ++i)
  {
    recorder.OnVehicleState(Sample(i * 10'000U, 100));
  }
  recorder.OnVehicleState(Sample(1'000'000U, 6000));
  EXPECT_FALSE(recorder.PollAndPersist());

  for (uint64_t i = 0U; i < kRecorderPostTriggerSamples; ++i)
  {
    recorder.OnVehicleState(Sample(1'010'000U + i * 10'000U, 100));
  }
  ASSERT_TRUE(recorder.PollAndPersist());

  const std::size_t expected_samples = 100U + 1U + kRecorderPostTriggerSamples;
  EXPECT_GT(FileSize(dir + "/crash_0.bin"), static_cast<std::streamoff>(expected_samples * sizeof(RecorderSample)));
}
