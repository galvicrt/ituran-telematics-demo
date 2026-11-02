// SPDX-License-Identifier: Apache-2.0
//
// Interface to the TCU hardware security module. Keys never leave the HSM;
// callers only receive computed MACs.

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace telematics
{
  using Mac256 = std::array<uint8_t, 32>;

  class Hsm
  {
  public:
    virtual ~Hsm() = default;

    // HMAC-SHA256 over `data` with the backend command key provisioned at end of line.
    virtual Mac256 ComputeCommandMac(const uint8_t* data, std::size_t size) const = 0;
  };
}
