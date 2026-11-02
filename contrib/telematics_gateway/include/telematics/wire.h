// SPDX-License-Identifier: Apache-2.0
//
// Byte-order helpers for CAN signals and binary wire protocols.
// Callers are responsible for checking that `offset + width` is within the
// valid length of the buffer before calling these helpers.

#pragma once

#include <cstddef>
#include <cstdint>

namespace telematics
{
  namespace wire
  {
    // Motorola (big-endian) byte order, as used by most powertrain CAN signals.
    inline uint16_t ReadBe16(const uint8_t* buf, std::size_t offset)
    {
      return static_cast<uint16_t>((static_cast<uint16_t>(buf[offset]) << 8U) | buf[offset + 1U]);
    }

    inline int16_t ReadBeS16(const uint8_t* buf, std::size_t offset)
    {
      return static_cast<int16_t>(ReadBe16(buf, offset));
    }

    // Intel (little-endian) byte order, as used by GNSS receivers.
    inline uint16_t ReadLe16(const uint8_t* buf, std::size_t offset)
    {
      return static_cast<uint16_t>(buf[offset] | (static_cast<uint16_t>(buf[offset + 1U]) << 8U));
    }

    inline uint32_t ReadLe32(const uint8_t* buf, std::size_t offset)
    {
      return static_cast<uint32_t>(buf[offset])
           | (static_cast<uint32_t>(buf[offset + 1U]) << 8U)
           | (static_cast<uint32_t>(buf[offset + 2U]) << 16U)
           | (static_cast<uint32_t>(buf[offset + 3U]) << 24U);
    }

    inline int32_t ReadLeS32(const uint8_t* buf, std::size_t offset)
    {
      return static_cast<int32_t>(ReadLe32(buf, offset));
    }
  }
}
