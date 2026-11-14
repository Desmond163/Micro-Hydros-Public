#include "validator.h"

#include <cmath>

namespace {
bool inRange(float value, float minimum, float maximum) {
    return std::isfinite(value) && value >= minimum && value <= maximum;
}
}  // namespace

std::uint8_t Validator::implausibleMask(const MeasurementData& measurement) {
    std::uint8_t mask = 0;
    if (!inRange(measurement.airInsideC, -10.0f, 60.0f)) mask |= kAirInside;
    if (!inRange(measurement.airOutsideC, -30.0f, 60.0f)) mask |= kAirOutside;
    if (!inRange(measurement.waterC, 0.0f, 50.0f)) mask |= kWater;
    if (!inRange(measurement.humidityInsidePct, 0.0f, 100.0f)) mask |= kHumidityInside;
    return mask;
}

bool Validator::isValid(const MeasurementData& measurement) {
    return implausibleMask(measurement) == 0;
}
