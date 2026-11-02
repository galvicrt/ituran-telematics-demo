// SPDX-License-Identifier: Apache-2.0

#include <telematics/immobilizer.h>

#include <gtest/gtest.h>

#include <array>
#include <vector>

using namespace telematics;

namespace
{
  // Deterministic stand-in for HMAC: XOR-folds the data into the MAC.
  class FakeHsm : public Hsm
  {
  public:
    Mac256 ComputeCommandMac(const uint8_t* data, std::size_t size) const override
    {
      Mac256 mac{};
      for (std::size_t i = 0U; i < size; ++i)
      {
        mac[i % mac.size()] = static_cast<uint8_t>(mac[i % mac.size()] ^ data[i] ^ 0x5AU);
      }
      return mac;
    }
  };

  class FakeClock : public Clock
  {
  public:
    uint64_t MonotonicUs() const override { return monotonic_us; }
    uint64_t WallMs() const override { return wall_ms; }
    uint64_t monotonic_us = 10'000'000U;
    uint64_t wall_ms      = 1'790'000'000'000U;
  };

  class FakeRelay : public StarterRelay
  {
  public:
    void SetInhibit(bool inhibit) override { inhibited = inhibit; ++calls; }
    bool inhibited = false;
    int  calls     = 0;
  };

  std::vector<uint8_t> BuildCommand(const Hsm& hsm, uint64_t id, uint64_t issued_at_ms, ImmobilizerAction action)
  {
    std::vector<uint8_t> bytes(kImmobilizerCommandSize, 0U);
    for (std::size_t i = 0U; i < 8U; ++i)
    {
      bytes[i]      = static_cast<uint8_t>(id >> (56U - 8U * i));
      bytes[8U + i] = static_cast<uint8_t>(issued_at_ms >> (56U - 8U * i));
    }
    bytes[16] = static_cast<uint8_t>(action);
    const Mac256 mac = hsm.ComputeCommandMac(bytes.data(), kImmobilizerSignedSize);
    std::copy(mac.begin(), mac.end(), bytes.begin() + kImmobilizerSignedSize);
    return bytes;
  }

  class ImmobilizerTest : public ::testing::Test
  {
  protected:
    VehicleState ParkedState() const
    {
      VehicleState state;
      state.speed_kph_x100 = 0U;
      state.updated_at_us  = clock.monotonic_us;
      return state;
    }

    FakeHsm               hsm;
    FakeClock             clock;
    FakeRelay             relay;
    ImmobilizerController controller{ hsm, clock, relay };
  };
}

TEST_F(ImmobilizerTest, ParsesValidCommand)
{
  const std::vector<uint8_t> bytes = BuildCommand(hsm, 42U, clock.wall_ms, ImmobilizerAction::kImmobilize);
  const auto command = ParseImmobilizerCommand(bytes.data(), bytes.size());
  ASSERT_TRUE(command.has_value());
  EXPECT_EQ(command->command_id, 42U);
  EXPECT_EQ(command->issued_at_ms, clock.wall_ms);
  EXPECT_EQ(command->action, ImmobilizerAction::kImmobilize);
}

TEST_F(ImmobilizerTest, RejectsWrongSize)
{
  const std::vector<uint8_t> bytes(10U, 0U);
  EXPECT_EQ(controller.HandleRequest(bytes.data(), bytes.size()), CommandResult::kRejectedMalformed);
}

TEST_F(ImmobilizerTest, ImmobilizesParkedVehicle)
{
  controller.OnVehicleState(ParkedState());
  const std::vector<uint8_t> bytes = BuildCommand(hsm, 1U, clock.wall_ms, ImmobilizerAction::kImmobilize);
  EXPECT_EQ(controller.HandleRequest(bytes.data(), bytes.size()), CommandResult::kAccepted);
  EXPECT_TRUE(relay.inhibited);
}

TEST_F(ImmobilizerTest, RefusesWhileDriving)
{
  VehicleState state = ParkedState();
  state.speed_kph_x100 = 5000U;
  controller.OnVehicleState(state);
  const std::vector<uint8_t> bytes = BuildCommand(hsm, 2U, clock.wall_ms, ImmobilizerAction::kImmobilize);
  EXPECT_EQ(controller.HandleRequest(bytes.data(), bytes.size()), CommandResult::kRejectedUnsafe);
  EXPECT_FALSE(relay.inhibited);
}

TEST_F(ImmobilizerTest, RejectsBadMac)
{
  controller.OnVehicleState(ParkedState());
  std::vector<uint8_t> bytes = BuildCommand(hsm, 3U, clock.wall_ms, ImmobilizerAction::kImmobilize);
  bytes.back() = static_cast<uint8_t>(bytes.back() ^ 0xFFU);
  EXPECT_EQ(controller.HandleRequest(bytes.data(), bytes.size()), CommandResult::kRejectedAuthentication);
  EXPECT_FALSE(relay.inhibited);
}

TEST_F(ImmobilizerTest, RejectsExpiredCommand)
{
  controller.OnVehicleState(ParkedState());
  const std::vector<uint8_t> bytes =
    BuildCommand(hsm, 4U, clock.wall_ms - kCommandValidityMs - 1U, ImmobilizerAction::kImmobilize);
  EXPECT_EQ(controller.HandleRequest(bytes.data(), bytes.size()), CommandResult::kRejectedExpired);
}

TEST_F(ImmobilizerTest, ReleasesVehicle)
{
  relay.inhibited = true;
  const std::vector<uint8_t> bytes = BuildCommand(hsm, 5U, clock.wall_ms, ImmobilizerAction::kRelease);
  EXPECT_EQ(controller.HandleRequest(bytes.data(), bytes.size()), CommandResult::kAccepted);
  EXPECT_FALSE(relay.inhibited);
}
