// SPDX-License-Identifier: Apache-2.0
//
// Time sources. Injected as interfaces so that safety logic can be tested
// deterministically.

#pragma once

#include <chrono>
#include <cstdint>

namespace telematics
{
  class Clock
  {
  public:
    virtual ~Clock() = default;
    virtual uint64_t MonotonicUs() const = 0;  // never goes backwards, starts at boot
    virtual uint64_t WallMs() const = 0;       // UTC epoch milliseconds, may jump (NTP/GNSS sync)
  };

  class SystemClock final : public Clock
  {
  public:
    uint64_t MonotonicUs() const override
    {
      return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
    }

    uint64_t WallMs() const override
    {
      return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
    }
  };
}
