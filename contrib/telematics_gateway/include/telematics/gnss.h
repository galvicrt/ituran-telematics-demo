// SPDX-License-Identifier: Apache-2.0
//
// GNSS receiver interface. The receiver bridge forwards UBX frames from the
// GNSS module as UDP datagrams; NAV-PVT solutions are turned into position
// reports for tracking and stolen vehicle recovery.

#pragma once

#include <telematics/clock.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace telematics
{
  // Published on the eCAL topic "telematics/position".
  struct PositionReport
  {
    int32_t  lat_deg_e7     = 0;  // latitude, 1e-7 degrees
    int32_t  lon_deg_e7     = 0;  // longitude, 1e-7 degrees
    uint32_t h_acc_mm       = 0;  // horizontal accuracy estimate
    int32_t  speed_mm_s     = 0;  // ground speed
    int32_t  heading_deg_e5 = 0;  // heading of motion, 1e-5 degrees
    uint8_t  num_sv         = 0;  // satellites used in the solution
    uint32_t itow_ms        = 0;  // GNSS time of week
    uint64_t received_at_us = 0;  // monotonic receive time
  };

  // UBX framing: sync (2) + class (1) + id (1) + length (2) + payload + checksum (2)
  constexpr uint8_t     kUbxSync1        = 0xB5U;
  constexpr uint8_t     kUbxSync2        = 0x62U;
  constexpr std::size_t kUbxHeaderSize   = 6U;
  constexpr std::size_t kUbxChecksumSize = 2U;
  constexpr uint8_t     kUbxClassNav     = 0x01U;
  constexpr uint8_t     kUbxIdNavPvt     = 0x07U;
  constexpr std::size_t kUbxNavPvtSize   = 92U;
  constexpr std::size_t kUbxMaxPayload   = 100U;

  // Threading: single-threaded, owned by the UDP receive loop.
  class UbxPositionDecoder
  {
  public:
    explicit UbxPositionDecoder(const Clock& clock);

    // Decodes one datagram. Returns a report if it contained a new NAV-PVT solution.
    std::optional<PositionReport> OnDatagram(const uint8_t* datagram, std::size_t size);

    uint32_t checksum_errors() const { return checksum_errors_; }

  private:
    std::optional<PositionReport> DecodeNavPvt(std::size_t length);

    const Clock&                           clock_;
    std::array<uint8_t, kUbxMaxPayload>    payload_ = {};
    uint32_t                               last_itow_ms_    = 0;
    bool                                   has_last_itow_   = false;
    uint32_t                               checksum_errors_ = 0;
  };
}
