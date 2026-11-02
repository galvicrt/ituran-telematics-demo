# AGENTS.md — Telematics Gateway

Coding rules for `contrib/telematics_gateway`. These apply to every change in
this directory, whoever or whatever writes it: engineers, IDE assistants,
coding agents, and code review. They extend the repository-wide rules in the
root `AGENTS.md` and the checks in the root `.clang-tidy`.

The rules are derived from our OEM supplier requirements: MISRA C++:2023 and
AUTOSAR C++14 style guidance, ISO 26262 (functional safety) and
ISO/SAE 21434 / UN R155 (cybersecurity).

## 1. Context

The gateway runs on the telematics control unit (TCU). It is split into
separate eCAL processes that talk over eCAL topics and services:

| Process | Input | Output |
|---|---|---|
| CAN decoder | `telematics/can_rx` (`CanFrame`) | `telematics/vehicle_state` (`VehicleState`) |
| GNSS publisher | GNSS receiver, UDP | `telematics/position` |
| Event recorder | `telematics/vehicle_state` | crash records on flash |
| Immobilizer service | backend command via eCAL service | `StarterRelay` (starter / fuel pump inhibit) |

Every byte that arrives over CAN, UDP or an eCAL service is **untrusted
external input**.

## 2. Input handling

- R2.1 Validate every length, count and index taken from external input before
  using it. No `memcpy`, array index or pointer arithmetic on an unchecked length.
- R2.2 For CAN frames, check `dlc` before reading a signal. A frame that is
  too short for the signal is rejected, never partially decoded.
- R2.3 Take the byte order and signedness of each signal from its signal
  definition and use the matching `telematics::wire` helper. Signed signals
  must be sign-extended.
- R2.4 Verify integrity checks (checksums, CRCs, MACs) **before** acting on or
  publishing any data from the message. A failed check drops the message.

## 3. Types and arithmetic

- R3.1 Use fixed-width integer types (`uint16_t`, `int32_t`, ...) for wire,
  CAN and persisted data.
- R3.2 Put the unit in the name: `_kph_x100`, `_mg`, `_ms`, `_us`, `_deg_e7`.
- R3.3 No silent narrowing, signed/unsigned mixing or overflow in scaling.
  Widen before you multiply.
- R3.4 Timestamps that can wrap (GNSS time of week, 32-bit counters) must be
  compared with wrap-aware arithmetic.

## 4. Safety

- R4.1 Commands that actuate the vehicle (immobilizer, door lock, horn) must
  fail safe: unknown, stale (see `IsVehicleStateFresh`) or implausible
  vehicle state is treated as unsafe and the command is refused.
- R4.2 The engine must never be immobilized while the vehicle is moving.
- R4.3 Safety-relevant detections (crash, rollover, tow-away) must consider
  every axis and direction the sensor can report.
- R4.4 Safety records (crash data) must survive power loss at any point: write
  to a temporary file, flush and `fsync`, then atomically `rename`.

## 5. Concurrency

- R5.1 eCAL receive and service callbacks run on eCAL worker threads. Any state
  shared between a callback and another thread must be protected by a mutex or
  be `std::atomic`.
- R5.2 Document which thread owns each member at the top of the class.
- R5.3 No dynamic allocation on 100 Hz paths after initialization; use
  fixed-capacity containers (`std::array`).

## 6. Security

- R6.1 Every remote command is authenticated with a MAC from the `Hsm`
  interface. No command, including "release" or "cancel", bypasses
  authentication.
- R6.2 Compare MACs and other secrets in constant time. Never `memcmp`.
- R6.3 Every remote command is replay-protected: reject commands outside the
  validity window **and** commands whose `command_id` has already been seen.

## 7. Privacy

- R7.1 Never log location (latitude / longitude), VIN, IMEI or driver
  identifiers at INFO level or above. INFO and above is uploaded to the
  backend. Use DEBUG, or log a coarse/obfuscated value.

## 8. Tests

- R8.1 Every new decoder, parser or safety rule ships with unit tests in
  `tests/`, including malformed and boundary inputs, not only the happy path.
