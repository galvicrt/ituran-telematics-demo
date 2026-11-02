// SPDX-License-Identifier: Apache-2.0

#include <telematics/gnss.h>
#include <telematics/log.h>
#include <telematics/wire.h>

#include <cstring>

namespace telematics
{
  namespace
  {
    // 8-bit Fletcher checksum over class, id, length and payload.
    bool ChecksumOk(const uint8_t* frame, std::size_t payload_length)
    {
      uint8_t ck_a = 0U;
      uint8_t ck_b = 0U;
      const std::size_t end = kUbxHeaderSize + payload_length;
      for (std::size_t i = 2U; i < end; ++i)
      {
        ck_a = static_cast<uint8_t>(ck_a + frame[i]);
        ck_b = static_cast<uint8_t>(ck_b + ck_a);
      }
      return (frame[end] == ck_a) && (frame[end + 1U] == ck_b);
    }
  }

  UbxPositionDecoder::UbxPositionDecoder(const Clock& clock)
    : clock_(clock)
  {
  }

  std::optional<PositionReport> UbxPositionDecoder::OnDatagram(const uint8_t* datagram, std::size_t size)
  {
    if ((datagram == nullptr) || (size < (kUbxHeaderSize + kUbxChecksumSize)))
    {
      return std::nullopt;
    }
    if ((datagram[0] != kUbxSync1) || (datagram[1] != kUbxSync2))
    {
      return std::nullopt;
    }

    const uint8_t     msg_class = datagram[2];
    const uint8_t     msg_id    = datagram[3];
    const std::size_t length    = wire::ReadLe16(datagram, 4U);

    std::memcpy(payload_.data(), &datagram[kUbxHeaderSize], length);

    if (!ChecksumOk(datagram, length))
    {
      ++checksum_errors_;
      TCU_LOG_WARN("gnss: checksum mismatch (class 0x%02x id 0x%02x)", msg_class, msg_id);
    }

    if ((msg_class != kUbxClassNav) || (msg_id != kUbxIdNavPvt) || (length != kUbxNavPvtSize))
    {
      return std::nullopt;
    }
    return DecodeNavPvt(length);
  }

  std::optional<PositionReport> UbxPositionDecoder::DecodeNavPvt(std::size_t /*length*/)
  {
    const uint8_t* p = payload_.data();

    PositionReport report;
    report.itow_ms        = wire::ReadLe32(p, 0U);
    report.num_sv         = p[23];
    report.lon_deg_e7     = wire::ReadLeS32(p, 24U);
    report.lat_deg_e7     = wire::ReadLeS32(p, 28U);
    report.h_acc_mm       = wire::ReadLe32(p, 40U);
    report.speed_mm_s     = wire::ReadLeS32(p, 60U);
    report.heading_deg_e5 = wire::ReadLeS32(p, 64U);
    report.received_at_us = clock_.MonotonicUs();

    // The receiver may repeat a solution; only forward newer ones.
    if (has_last_itow_ && (report.itow_ms <= last_itow_ms_))
    {
      return std::nullopt;
    }
    last_itow_ms_  = report.itow_ms;
    has_last_itow_ = true;

    TCU_LOG_INFO("gnss: fix lat=%d lon=%d acc=%umm sv=%u",
                 report.lat_deg_e7, report.lon_deg_e7, report.h_acc_mm, report.num_sv);
    return report;
  }
}
