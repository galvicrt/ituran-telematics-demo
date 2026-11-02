// SPDX-License-Identifier: Apache-2.0
//
// eCAL process: GNSS position publisher.
//   input:      UBX frames from the GNSS receiver bridge, UDP port 5010
//   publishes:  telematics/position (PositionReport)

#include <telematics/clock.h>
#include <telematics/gnss.h>
#include <telematics/log.h>

#include <ecal/ecal.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <array>
#include <cstdint>
#include <optional>

namespace
{
  constexpr uint16_t kGnssBridgePort = 5010U;
}

int main()
{
  eCAL::Initialize("tcu_gnss_publisher");

  const int sock = ::socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0)
  {
    TCU_LOG_ERROR("gnss: socket() failed");
    return 1;
  }

  sockaddr_in addr{};
  addr.sin_family      = AF_INET;
  addr.sin_port        = htons(kGnssBridgePort);
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  if (::bind(sock, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) != 0)
  {
    TCU_LOG_ERROR("gnss: bind() failed");
    ::close(sock);
    return 1;
  }

  timeval timeout{};
  timeout.tv_usec = 200 * 1000;
  ::setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

  const telematics::SystemClock  clock;
  telematics::UbxPositionDecoder decoder(clock);
  eCAL::CPublisher               publisher("telematics/position");

  std::array<uint8_t, 2048> buffer{};
  while (eCAL::Ok())
  {
    const ssize_t received = ::recv(sock, buffer.data(), buffer.size(), 0);
    if (received <= 0)
    {
      continue;
    }

    const std::optional<telematics::PositionReport> report =
      decoder.OnDatagram(buffer.data(), static_cast<std::size_t>(received));
    if (report)
    {
      publisher.Send(&*report, sizeof(*report));
    }
  }

  ::close(sock);
  eCAL::Finalize();
  return 0;
}
