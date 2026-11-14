#pragma once

#include <cstdint>

#include "measurement_data.h"

// Compares plausible readings against the growing-environment thresholds in config.h.
// Channels that failed or are implausible never raise threshold alarms (a broken sensor is
// reported as a fault, not as "too hot").
class AlarmEvaluator {
public:
    static std::uint8_t evaluate(const MeasurementData& measurement);
};
