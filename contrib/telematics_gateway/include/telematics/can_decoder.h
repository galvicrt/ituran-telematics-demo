// SPDX-License-Identifier: Apache-2.0
//
// Decodes powertrain and body CAN frames into a VehicleState.
//
// Signal definitions (from the vehicle DBC, all Motorola / big-endian):
//
//   0x0F0 VEHICLE_SPEED  DLC 2  bytes 0-1  speed           unsigned  1/256 km/h
//   0x1A0 ENGINE         DLC 4  bytes 2-3  engine speed    unsigned  0.25 rpm
//   0x2C0 BODY_STATUS    DLC 1  byte  0    bit 0 ignition on, bit 3 driver door open
//   0x3E0 ACCELERATION   DLC 4  bytes 0-1  longitudinal    signed    1 mg
//                               bytes 2-3  lateral         signed    1 mg

#pragma once

#include <telematics/can_frame.h>
#include <telematics/vehicle_state.h>

#include <cstdint>

namespace telematics
{
  constexpr uint32_t kCanIdVehicleSpeed = 0x0F0U;
  constexpr uint32_t kCanIdEngine       = 0x1A0U;
  constexpr uint32_t kCanIdBodyStatus   = 0x2C0U;
  constexpr uint32_t kCanIdAcceleration = 0x3E0U;

  // Accelerometer measurement range.
  constexpr int32_t kAccelLimitMg = 16000;

  // Threading: single-threaded, owned by the eCAL CAN receive callback.
  class CanDecoder
  {
  public:
    // Returns true if the frame updated the vehicle state.
    bool OnFrame(const CanFrame& frame);

    const VehicleState& state() const { return state_; }

  private:
    VehicleState state_;
  };
}
