#pragma once

#include <cstdint>

// Build-time configuration. Values can be overridden with -D flags in platformio.ini.

#ifndef DEVICE_ID
#define DEVICE_ID "mh-01"
#endif

#ifndef MEASUREMENT_INTERVAL_MS
#define MEASUREMENT_INTERVAL_MS 5000
#endif

#ifndef SIM_TIME_SCALE
#define SIM_TIME_SCALE 1
#endif

namespace config {

constexpr const char* kDeviceId = DEVICE_ID;
constexpr std::uint32_t kMeasurementIntervalMs = MEASUREMENT_INTERVAL_MS;
constexpr std::uint32_t kSimTimeScale = SIM_TIME_SCALE;

// Alarm thresholds for the growing environment. These are ASSUMPTIONS for the prototype and
// must be confirmed with HydroGreen Fingers before they are used for real crops.
// (Plausibility guardrails that detect broken sensors live in Validator, not here.)
constexpr float kAirInsideMinC = 18.0f;
constexpr float kAirInsideMaxC = 28.0f;
constexpr float kWaterMinC = 18.0f;
constexpr float kWaterMaxC = 24.0f;
constexpr float kHumidityMinPct = 40.0f;
constexpr float kHumidityMaxPct = 80.0f;

}  // namespace config
