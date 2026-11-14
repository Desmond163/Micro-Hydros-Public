#include "sensor_manager.h"

#include <limits>

#include "alarm.h"
#include "validator.h"

SensorManager::SensorManager(hal::ITemperatureSensor& airInside,
                             hal::ITemperatureSensor& airOutside, hal::ITemperatureSensor& water,
                             hal::IHumiditySensor& humidityInside)
    : airInside_(airInside),
      airOutside_(airOutside),
      water_(water),
      humidityInside_(humidityInside) {}

bool SensorManager::begin() {
    // Initialise every sensor even if one fails, so the remaining channels still work.
    bool ok = airInside_.begin();
    ok = airOutside_.begin() && ok;
    ok = water_.begin() && ok;
    ok = humidityInside_.begin() && ok;
    return ok;
}

MeasurementData SensorManager::read(std::uint32_t timestampMs) {
    constexpr float kMissing = std::numeric_limits<float>::quiet_NaN();

    MeasurementData m{};
    m.sequence = nextSequence_++;
    m.timestampMs = timestampMs;

    if (!airInside_.readCelsius(m.airInsideC)) {
        m.airInsideC = kMissing;
        m.readFailures |= kAirInside;
    }
    if (!airOutside_.readCelsius(m.airOutsideC)) {
        m.airOutsideC = kMissing;
        m.readFailures |= kAirOutside;
    }
    if (!water_.readCelsius(m.waterC)) {
        m.waterC = kMissing;
        m.readFailures |= kWater;
    }
    if (!humidityInside_.readRelativeHumidity(m.humidityInsidePct)) {
        m.humidityInsidePct = kMissing;
        m.readFailures |= kHumidityInside;
    }

    m.implausible = Validator::implausibleMask(m) & static_cast<std::uint8_t>(~m.readFailures);
    m.alarms = AlarmEvaluator::evaluate(m);
    m.valid = (m.readFailures | m.implausible) == 0;
    return m;
}
