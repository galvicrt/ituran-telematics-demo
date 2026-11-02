// SPDX-License-Identifier: Apache-2.0
//
// eCAL process: crash event recorder.
//   subscribes: telematics/vehicle_state (VehicleState, 100 Hz)
//   output:     crash records in /data/tcu/crash

#include <telematics/event_recorder.h>
#include <telematics/log.h>
#include <telematics/vehicle_state.h>

#include <ecal/ecal.h>

#include <chrono>
#include <cstring>
#include <thread>

int main()
{
  eCAL::Initialize("tcu_event_recorder");

  telematics::EventRecorder recorder("/data/tcu/crash");

  eCAL::CSubscriber state_subscriber("telematics/vehicle_state");
  state_subscriber.SetReceiveCallback(
    [&recorder](const eCAL::STopicId& /*topic_id*/, const eCAL::SDataTypeInformation& /*type_info*/, const eCAL::SReceiveCallbackData& data)
    {
      if (data.buffer_size != sizeof(telematics::VehicleState))
      {
        return;
      }
      telematics::VehicleState state;
      std::memcpy(&state, data.buffer, sizeof(state));
      recorder.OnVehicleState(state);
    });

  std::thread writer([&recorder]()
    {
      while (eCAL::Ok())
      {
        recorder.PollAndPersist();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
      }
    });

  TCU_LOG_INFO("event recorder running");
  writer.join();

  eCAL::Finalize();
  return 0;
}
