// SPDX-License-Identifier: Apache-2.0
//
// Vehicle state as seen by the telematics control unit (TCU).
// Populated by the CAN decoder and published on the eCAL topic
// "telematics/vehicle_state".

#pragma once

#include <cstdint>

namespace telematics
{
  struct VehicleState
  {
    uint16_t speed_kph_x100 = 0;  // vehicle speed, 0.01 km/h per bit
    uint16_t engine_rpm     = 0;  // engine speed, 1 rpm per bit
    bool     ignition_on    = false;
    bool     driver_door_open = false;
    int16_t  accel_long_mg  = 0;  // longitudinal acceleration, +forward, milli-g
    int16_t  accel_lat_mg   = 0;  // lateral acceleration, +left, milli-g
    uint64_t updated_at_us  = 0;  // monotonic time of last update, 0 = never received
  };

  // Maximum age of a VehicleState before consumers must treat it as unknown.
  constexpr uint64_t kVehicleStateMaxAgeUs = 200U * 1000U;

  // True if `state` has been received at least once and is not older than
  // kVehicleStateMaxAgeUs at monotonic time `now_us`.
  bool IsVehicleStateFresh(const VehicleState& state, uint64_t now_us);
}
