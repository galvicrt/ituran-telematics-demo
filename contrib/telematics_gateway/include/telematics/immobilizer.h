// SPDX-License-Identifier: Apache-2.0
//
// Remote immobilizer for stolen vehicle recovery. The backend sends signed
// immobilize / release commands; the TCU inhibits or re-enables the starter.

#pragma once

#include <telematics/clock.h>
#include <telematics/hsm.h>
#include <telematics/platform.h>
#include <telematics/vehicle_state.h>

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>

namespace telematics
{
  enum class ImmobilizerAction : uint8_t
  {
    kImmobilize = 1,
    kRelease    = 2,
  };

  enum class CommandResult : uint8_t
  {
    kAccepted,
    kRejectedMalformed,
    kRejectedAuthentication,
    kRejectedExpired,
    kRejectedUnsafe,
  };

  // Wire format (big-endian), 49 bytes:
  //   [0..7]   command_id
  //   [8..15]  issued_at_ms (UTC epoch ms, backend clock)
  //   [16]     action
  //   [17..48] HMAC-SHA256 over bytes [0..16]
  constexpr std::size_t kImmobilizerCommandSize = 49U;
  constexpr std::size_t kImmobilizerSignedSize  = 17U;

  struct ImmobilizerCommand
  {
    uint64_t          command_id   = 0;
    uint64_t          issued_at_ms = 0;
    ImmobilizerAction action       = ImmobilizerAction::kImmobilize;
    Mac256            mac          = {};
  };

  std::optional<ImmobilizerCommand> ParseImmobilizerCommand(const uint8_t* data, std::size_t size);

  // Commands older than this are rejected.
  constexpr uint64_t kCommandValidityMs = 30U * 1000U;

  // Below this speed the vehicle is considered stationary.
  constexpr uint16_t kStationarySpeedKphX100 = 100U;

  // Threading:
  //   OnVehicleState() is called on the eCAL subscriber thread.
  //   HandleRequest()  is called on the eCAL service thread.
  //   state_ is shared and protected by state_mutex_.
  class ImmobilizerController
  {
  public:
    ImmobilizerController(const Hsm& hsm, const Clock& clock, StarterRelay& relay);

    void          OnVehicleState(const VehicleState& state);
    CommandResult HandleRequest(const uint8_t* data, std::size_t size);

  private:
    bool IsAuthentic(const uint8_t* data, const Mac256& mac) const;
    bool IsWithinValidityWindow(const ImmobilizerCommand& command) const;
    bool IsSafeToImmobilize() const;

    const Hsm&    hsm_;
    const Clock&  clock_;
    StarterRelay& relay_;

    mutable std::mutex state_mutex_;
    VehicleState       state_;
  };
}
