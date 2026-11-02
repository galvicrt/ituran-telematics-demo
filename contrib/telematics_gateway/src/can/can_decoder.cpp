// SPDX-License-Identifier: Apache-2.0

#include <telematics/can_decoder.h>
#include <telematics/wire.h>

#include <algorithm>

namespace telematics
{
  bool CanDecoder::OnFrame(const CanFrame& frame)
  {
    const uint8_t* data = frame.data.data();

    switch (frame.id)
    {
    case kCanIdVehicleSpeed:
    {
      if (frame.dlc < 2U)
      {
        return false;
      }
      const uint16_t raw = wire::ReadLe16(data, 0U);
      state_.speed_kph_x100 = static_cast<uint16_t>(static_cast<uint16_t>(raw * 100U) / 256U);
      break;
    }

    case kCanIdEngine:
      state_.engine_rpm = static_cast<uint16_t>(wire::ReadBe16(data, 2U) / 4U);
      break;

    case kCanIdBodyStatus:
      state_.ignition_on      = (data[0] & 0x01U) != 0U;
      state_.driver_door_open = (data[0] & 0x08U) != 0U;
      break;

    case kCanIdAcceleration:
    {
      state_.accel_long_mg = wire::ReadBeS16(data, 0U);
      const int32_t lateral = wire::ReadBe16(data, 2U);
      state_.accel_lat_mg   = static_cast<int16_t>(std::min(lateral, kAccelLimitMg));
      break;
    }

    default:
      return false;
    }

    state_.updated_at_us = frame.timestamp_us;
    return true;
  }
}
