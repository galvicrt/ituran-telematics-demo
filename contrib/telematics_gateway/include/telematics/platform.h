// SPDX-License-Identifier: Apache-2.0
//
// Hardware access provided by the TCU board support package (BSP). The BSP is
// only available in the target toolchain; unit tests use fakes instead.

#pragma once

#include <telematics/hsm.h>

#include <memory>

namespace telematics
{
  // Starter / fuel pump inhibit relay.
  class StarterRelay
  {
  public:
    virtual ~StarterRelay() = default;
    virtual void SetInhibit(bool inhibit) = 0;
  };

  std::unique_ptr<Hsm>          CreatePlatformHsm();
  std::unique_ptr<StarterRelay> CreatePlatformStarterRelay();
}
