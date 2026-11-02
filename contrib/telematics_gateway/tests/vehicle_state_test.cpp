// SPDX-License-Identifier: Apache-2.0

#include <telematics/vehicle_state.h>

#include <gtest/gtest.h>

using telematics::IsVehicleStateFresh;
using telematics::VehicleState;
using telematics::kVehicleStateMaxAgeUs;

TEST(VehicleState, NeverReceivedIsNotFresh)
{
  const VehicleState state{};
  EXPECT_FALSE(IsVehicleStateFresh(state, 1'000'000U));
}

TEST(VehicleState, RecentUpdateIsFresh)
{
  VehicleState state{};
  state.updated_at_us = 1'000'000U;
  EXPECT_TRUE(IsVehicleStateFresh(state, 1'000'000U + kVehicleStateMaxAgeUs));
}

TEST(VehicleState, OldUpdateIsStale)
{
  VehicleState state{};
  state.updated_at_us = 1'000'000U;
  EXPECT_FALSE(IsVehicleStateFresh(state, 1'000'000U + kVehicleStateMaxAgeUs + 1U));
}

TEST(VehicleState, UpdateFromTheFutureIsStale)
{
  VehicleState state{};
  state.updated_at_us = 2'000'000U;
  EXPECT_FALSE(IsVehicleStateFresh(state, 1'000'000U));
}
