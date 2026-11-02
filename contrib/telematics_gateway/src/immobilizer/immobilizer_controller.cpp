// SPDX-License-Identifier: Apache-2.0

#include <telematics/immobilizer.h>
#include <telematics/log.h>

#include <cinttypes>
#include <cstring>

namespace telematics
{
  namespace
  {
    uint64_t ReadBe64(const uint8_t* buf, std::size_t offset)
    {
      uint64_t value = 0U;
      for (std::size_t i = 0U; i < 8U; ++i)
      {
        value = (value << 8U) | buf[offset + i];
      }
      return value;
    }
  }

  std::optional<ImmobilizerCommand> ParseImmobilizerCommand(const uint8_t* data, std::size_t size)
  {
    if ((data == nullptr) || (size != kImmobilizerCommandSize))
    {
      return std::nullopt;
    }

    const uint8_t action = data[16];
    if ((action != static_cast<uint8_t>(ImmobilizerAction::kImmobilize))
     && (action != static_cast<uint8_t>(ImmobilizerAction::kRelease)))
    {
      return std::nullopt;
    }

    ImmobilizerCommand command;
    command.command_id   = ReadBe64(data, 0U);
    command.issued_at_ms = ReadBe64(data, 8U);
    command.action       = static_cast<ImmobilizerAction>(action);
    std::memcpy(command.mac.data(), &data[kImmobilizerSignedSize], command.mac.size());
    return command;
  }

  ImmobilizerController::ImmobilizerController(const Hsm& hsm, const Clock& clock, StarterRelay& relay)
    : hsm_(hsm)
    , clock_(clock)
    , relay_(relay)
  {
  }

  void ImmobilizerController::OnVehicleState(const VehicleState& state)
  {
    const std::lock_guard<std::mutex> lock(state_mutex_);
    state_ = state;
  }

  CommandResult ImmobilizerController::HandleRequest(const uint8_t* data, std::size_t size)
  {
    const std::optional<ImmobilizerCommand> command = ParseImmobilizerCommand(data, size);
    if (!command)
    {
      TCU_LOG_WARN("immobilizer: malformed command (%zu bytes)", size);
      return CommandResult::kRejectedMalformed;
    }

    // Releasing the starter can never put the vehicle in a dangerous state, so
    // it is applied right away. This keeps a customer from being stranded if
    // the backend clock or the HSM is temporarily unavailable.
    if (command->action == ImmobilizerAction::kRelease)
    {
      relay_.SetInhibit(false);
      TCU_LOG_INFO("immobilizer: released (command %" PRIu64 ")", command->command_id);
      return CommandResult::kAccepted;
    }

    if (!IsAuthentic(data, command->mac))
    {
      TCU_LOG_WARN("immobilizer: authentication failed (command %" PRIu64 ")", command->command_id);
      return CommandResult::kRejectedAuthentication;
    }

    if (!IsWithinValidityWindow(*command))
    {
      TCU_LOG_WARN("immobilizer: expired (command %" PRIu64 ")", command->command_id);
      return CommandResult::kRejectedExpired;
    }

    if (!IsSafeToImmobilize())
    {
      TCU_LOG_WARN("immobilizer: vehicle moving, refused (command %" PRIu64 ")", command->command_id);
      return CommandResult::kRejectedUnsafe;
    }

    relay_.SetInhibit(true);
    TCU_LOG_INFO("immobilizer: immobilized (command %" PRIu64 ")", command->command_id);
    return CommandResult::kAccepted;
  }

  bool ImmobilizerController::IsAuthentic(const uint8_t* data, const Mac256& mac) const
  {
    const Mac256 expected = hsm_.ComputeCommandMac(data, kImmobilizerSignedSize);
    return std::memcmp(expected.data(), mac.data(), mac.size()) == 0;
  }

  bool ImmobilizerController::IsWithinValidityWindow(const ImmobilizerCommand& command) const
  {
    const uint64_t now_ms = clock_.WallMs();
    return (now_ms - command.issued_at_ms) <= kCommandValidityMs;
  }

  bool ImmobilizerController::IsSafeToImmobilize() const
  {
    const std::lock_guard<std::mutex> lock(state_mutex_);
    return state_.speed_kph_x100 < kStationarySpeedKphX100;
  }
}
