#include "alarm.h"

#include "config.h"

std::uint8_t AlarmEvaluator::evaluate(const MeasurementData& m) {
    const std::uint8_t faulty = m.readFailures | m.implausible;
    std::uint8_t alarms = 0;

    if (!(faulty & kAirInside)) {
        if (m.airInsideC < config::kAirInsideMinC) alarms |= kAlarmAirInsideLow;
        if (m.airInsideC > config::kAirInsideMaxC) alarms |= kAlarmAirInsideHigh;
    }
    if (!(faulty & kWater)) {
        if (m.waterC < config::kWaterMinC) alarms |= kAlarmWaterLow;
        if (m.waterC > config::kWaterMaxC) alarms |= kAlarmWaterHigh;
    }
    if (!(faulty & kHumidityInside)) {
        if (m.humidityInsidePct < config::kHumidityMinPct) alarms |= kAlarmHumidityLow;
        if (m.humidityInsidePct > config::kHumidityMaxPct) alarms |= kAlarmHumidityHigh;
    }
    return alarms;
}
