#pragma once

#include <cstdint>

#include "hal/sensors.h"
#include "measurement_data.h"

// Reads all four channels through the HAL and produces one MeasurementData record,
// including fault, plausibility and alarm information. It has no knowledge of whether the
// sensors are simulated or physical.
class SensorManager {
public:
    SensorManager(hal::ITemperatureSensor& airInside, hal::ITemperatureSensor& airOutside,
                  hal::ITemperatureSensor& water, hal::IHumiditySensor& humidityInside);

    // Initialises all sensors; returns false if any sensor failed to start.
    bool begin();
    MeasurementData read(std::uint32_t timestampMs);

private:
    hal::ITemperatureSensor& airInside_;
    hal::ITemperatureSensor& airOutside_;
    hal::ITemperatureSensor& water_;
    hal::IHumiditySensor& humidityInside_;
    std::uint32_t nextSequence_ = 0;
};
