// SPDX-License-Identifier: Apache-2.0
//
// Minimal logging for TCU processes. INFO and above is uploaded to the
// fleet backend; DEBUG stays on the device.

#pragma once

#include <cstdio>

#define TCU_LOG_DEBUG(...) do { std::fprintf(stderr, "[DEBUG] " __VA_ARGS__); std::fputc('\n', stderr); } while (0)
#define TCU_LOG_INFO(...)  do { std::fprintf(stderr, "[INFO ] " __VA_ARGS__); std::fputc('\n', stderr); } while (0)
#define TCU_LOG_WARN(...)  do { std::fprintf(stderr, "[WARN ] " __VA_ARGS__); std::fputc('\n', stderr); } while (0)
#define TCU_LOG_ERROR(...) do { std::fprintf(stderr, "[ERROR] " __VA_ARGS__); std::fputc('\n', stderr); } while (0)
