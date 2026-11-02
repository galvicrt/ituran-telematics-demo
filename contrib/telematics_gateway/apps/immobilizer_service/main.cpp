// SPDX-License-Identifier: Apache-2.0
//
// eCAL process: remote immobilizer service.
//   subscribes: telematics/vehicle_state (VehicleState)
//   service:    telematics_immobilizer / execute (ImmobilizerCommand wire format)

#include <telematics/clock.h>
#include <telematics/immobilizer.h>
#include <telematics/log.h>
#include <telematics/platform.h>
#include <telematics/vehicle_state.h>

#include <ecal/ecal.h>

#include <chrono>
#include <cstring>
#include <memory>
#include <string>
#include <thread>

int main()
{
  eCAL::Initialize("tcu_immobilizer_service");

  const std::unique_ptr<telematics::Hsm>          hsm   = telematics::CreatePlatformHsm();
  const std::unique_ptr<telematics::StarterRelay> relay = telematics::CreatePlatformStarterRelay();
  const telematics::SystemClock                   clock;

  telematics::ImmobilizerController controller(*hsm, clock, *relay);

  eCAL::CSubscriber state_subscriber("telematics/vehicle_state");
  state_subscriber.SetReceiveCallback(
    [&controller](const eCAL::STopicId& /*topic_id*/, const eCAL::SDataTypeInformation& /*type_info*/, const eCAL::SReceiveCallbackData& data)
    {
      if (data.buffer_size != sizeof(telematics::VehicleState))
      {
        return;
      }
      telematics::VehicleState state;
      std::memcpy(&state, data.buffer, sizeof(state));
      controller.OnVehicleState(state);
    });

  eCAL::CServiceServer server("telematics_immobilizer");
  server.SetMethodCallback({ "execute", {}, {} },
    [&controller](const eCAL::SServiceMethodInformation& /*method_info*/, const std::string& request, std::string& response) -> int
    {
      const telematics::CommandResult result =
        controller.HandleRequest(reinterpret_cast<const uint8_t*>(request.data()), request.size());
      response.assign(1U, static_cast<char>(result));
      return 0;
    });

  TCU_LOG_INFO("immobilizer service running");
  while (eCAL::Ok())
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  eCAL::Finalize();
  return 0;
}
