// SPDX-License-Identifier: Apache-2.0

#include <telematics/event_recorder.h>
#include <telematics/log.h>

#include <fstream>
#include <utility>

namespace telematics
{
  namespace
  {
    struct RecordHeader
    {
      char     magic[4]     = { 'T', 'C', 'U', 'C' };
      uint16_t version      = 1U;
      uint16_t sample_count = 0U;
    };
  }

  EventRecorder::EventRecorder(std::string output_dir)
    : output_dir_(std::move(output_dir))
  {
  }

  void EventRecorder::OnVehicleState(const VehicleState& state)
  {
    RecorderSample sample;
    sample.t_us           = state.updated_at_us;
    sample.speed_kph_x100 = state.speed_kph_x100;
    sample.accel_long_mg  = state.accel_long_mg;
    sample.accel_lat_mg   = state.accel_lat_mg;
    sample.ignition_on    = state.ignition_on ? 1U : 0U;

    const std::lock_guard<std::mutex> lock(mutex_);

    buffer_[head_] = sample;
    if (++head_ > kRecorderCapacity)
    {
      head_ = 0U;
    }
    if (count_ < kRecorderCapacity)
    {
      ++count_;
    }

    if (!triggered_)
    {
      if (sample.accel_long_mg > kCrashThresholdMg)
      {
        triggered_              = true;
        post_trigger_remaining_ = kRecorderPostTriggerSamples;
        TCU_LOG_WARN("recorder: crash detected (%d mg)", sample.accel_long_mg);
      }
    }
    else if (post_trigger_remaining_ > 0U)
    {
      --post_trigger_remaining_;
    }
  }

  bool EventRecorder::PollAndPersist()
  {
    if (!triggered_ || (post_trigger_remaining_ != 0U))
    {
      return false;
    }

    std::array<RecorderSample, kRecorderCapacity> snapshot;
    std::size_t                                   count = 0U;
    {
      const std::lock_guard<std::mutex> lock(mutex_);
      const std::size_t oldest = (head_ + kRecorderCapacity - count_) % kRecorderCapacity;
      for (std::size_t i = 0U; i < count_; ++i)
      {
        snapshot[i] = buffer_[(oldest + i) % kRecorderCapacity];
      }
      count      = count_;
      triggered_ = false;
    }

    return WriteRecord(snapshot, count);
  }

  bool EventRecorder::WriteRecord(const std::array<RecorderSample, kRecorderCapacity>& samples, std::size_t count)
  {
    const std::string path = output_dir_ + "/crash_" + std::to_string(record_seq_++) + ".bin";

    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out)
    {
      TCU_LOG_ERROR("recorder: cannot open %s", path.c_str());
      return false;
    }

    RecordHeader header;
    header.sample_count = static_cast<uint16_t>(count);
    out.write(reinterpret_cast<const char*>(&header), sizeof(header));
    out.write(reinterpret_cast<const char*>(samples.data()), static_cast<std::streamsize>(count * sizeof(RecorderSample)));

    TCU_LOG_INFO("recorder: crash record written to %s (%zu samples)", path.c_str(), count);
    return out.good();
  }
}
