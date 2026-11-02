// SPDX-License-Identifier: Apache-2.0

#include <telematics/vehicle_state.h>

namespace telematics
{
  bool IsVehicleStateFresh(const VehicleState& state, uint64_t now_us)
  {
    if (state.updated_at_us == 0U)
    {
      return false;
    }
    if (now_us < state.updated_at_us)
    {
      return false;
    }
    return (now_us - state.updated_at_us) <= kVehicleStateMaxAgeUs;
  }
}
