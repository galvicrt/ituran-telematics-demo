# Telematics Gateway

Telematics control unit (TCU) gateway built on eCAL. It covers the in-vehicle
side of a connected-car / stolen-vehicle-recovery product:

- **CAN decoder:** turns raw powertrain CAN frames into a `VehicleState`.
- **GNSS publisher:** reads the GNSS receiver and publishes vehicle position for tracking and recovery.
- **Event recorder:** keeps a rolling buffer of vehicle data and stores a crash record when an impact is detected.
- **Immobilizer service:** executes remote immobilize / release commands from the backend.

Each component is a separate eCAL process. Logic lives in the `telematics_core`
library, which has no eCAL dependency and is unit tested. The eCAL processes in
`apps/` are thin wrappers around it.

## Build

```bash
cmake -S . -B build -DECAL_BUILD_TELEMATICS_GATEWAY=ON -DECAL_BUILD_TESTS=ON
cmake --build build --target test_telematics_gateway
```

## Coding rules

See [AGENTS.md](AGENTS.md). The same rules are used by engineers, by coding
assistants and by automated code review, and are enforced on every pull
request through the repository's `.coderabbit.yaml`.
