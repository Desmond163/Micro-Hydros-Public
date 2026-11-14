#pragma once

#include <cstdint>

#include "measurement_data.h"

// Plausibility checks: detects values a working sensor cannot produce (NaN, far outside the
// physical range of the environment). These are not crop setpoints; see AlarmEvaluator.
class Validator {
public:
    // ChannelBit mask of channels that are non-finite or outside the guardrails.
    static std::uint8_t implausibleMask(const MeasurementData& measurement);
    static bool isValid(const MeasurementData& measurement);
};
