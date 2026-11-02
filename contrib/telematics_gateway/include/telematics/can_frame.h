// SPDX-License-Identifier: Apache-2.0
//
// Classic CAN frame as delivered by the CAN driver bridge on the eCAL topic
// "telematics/can_rx".

#pragma once

#include <array>
#include <cstdint>

namespace telematics
{
  struct CanFrame
  {
    uint32_t               id           = 0;  // 11-bit standard identifier
    uint8_t                dlc          = 0;  // number of valid bytes in data (0..8)
    std::array<uint8_t, 8> data         = {};
    uint64_t               timestamp_us = 0;  // monotonic receive time
  };
}
