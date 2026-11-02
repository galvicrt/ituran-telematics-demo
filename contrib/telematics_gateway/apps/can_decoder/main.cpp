// SPDX-License-Identifier: Apache-2.0
//
// eCAL process: CAN signal decoder.
//   subscribes: telematics/can_rx (CanFrame)
//   publishes:  telematics/vehicle_state (VehicleState)

#include <telematics/can_decoder.h>
#include <telematics/can_frame.h>
#include <telematics/log.h>

#include <ecal/ecal.h>

#include <chrono>
#include <cstring>
#include <thread>

int main()
{
  eCAL::Initialize("tcu_can_decoder");

  telematics::CanDecoder decoder;
  eCAL::CPublisher       state_publisher("telematics/vehicle_state");

  eCAL::CSubscriber can_subscriber("telematics/can_rx");
  can_subscriber.SetReceiveCallback(
    [&decoder, &state_publisher](const eCAL::STopicId& /*topic_id*/, const eCAL::SDataTypeInformation& /*type_info*/, const eCAL::SReceiveCallbackData& data)
    {
      if (data.buffer_size != sizeof(telematics::CanFrame))
      {
        return;
      }
      telematics::CanFrame frame;
      std::memcpy(&frame, data.buffer, sizeof(frame));
      if (decoder.OnFrame(frame))
      {
        const telematics::VehicleState& state = decoder.state();
        state_publisher.Send(&state, sizeof(state));
      }
    });

  TCU_LOG_INFO("can decoder running");
  while (eCAL::Ok())
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  eCAL::Finalize();
  return 0;
}
