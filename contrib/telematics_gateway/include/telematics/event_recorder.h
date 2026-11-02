// SPDX-License-Identifier: Apache-2.0
//
// Crash event recorder. Keeps a rolling window of vehicle data and stores a
// crash record (5 s before and 2 s after an impact) for accident
// reconstruction and insurance claims.

#pragma once

#include <telematics/vehicle_state.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>

namespace telematics
{
  struct RecorderSample
  {
    uint64_t t_us           = 0;
    uint16_t speed_kph_x100 = 0;
    int16_t  accel_long_mg  = 0;
    int16_t  accel_lat_mg   = 0;
    uint8_t  ignition_on    = 0;
  };

  constexpr std::size_t kRecorderPreTriggerSamples  = 500U;  // 5 s at 100 Hz
  constexpr std::size_t kRecorderPostTriggerSamples = 200U;  // 2 s at 100 Hz
  constexpr std::size_t kRecorderCapacity           = kRecorderPreTriggerSamples + kRecorderPostTriggerSamples;
  constexpr int16_t     kCrashThresholdMg           = 4000;  // 4 g

  // Threading:
  //   OnVehicleState()  is called on the eCAL subscriber thread at 100 Hz.
  //   PollAndPersist()  is called on the recorder's writer thread.
  //   buffer_, head_, count_, triggered_, post_trigger_remaining_ are shared
  //   and protected by mutex_.
  class EventRecorder
  {
  public:
    explicit EventRecorder(std::string output_dir);

    void OnVehicleState(const VehicleState& state);

    // Writes a crash record once the post-trigger window is complete.
    // Returns true if a record was written.
    bool PollAndPersist();

  private:
    bool WriteRecord(const std::array<RecorderSample, kRecorderCapacity>& samples, std::size_t count);

    const std::string output_dir_;
    uint32_t          record_seq_ = 0;  // writer thread only

    std::mutex                                     mutex_;
    std::array<RecorderSample, kRecorderCapacity>  buffer_ = {};
    std::size_t                                    head_   = 0;
    std::size_t                                    count_  = 0;
    bool                                           triggered_              = false;
    std::size_t                                    post_trigger_remaining_ = 0;
  };
}
