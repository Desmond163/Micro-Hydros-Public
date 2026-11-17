#include "hal/real_sensors.h"

#if USE_REAL_SENSORS

namespace hal {

bool Sht4xDevice::begin() {
    if (!started_) {
        started_ = sht_.begin(&wire_);
        if (started_) {
            sht_.setPrecision(SHT4X_HIGH_PRECISION);
            sht_.setHeater(SHT4X_NO_HEATER);
        }
    }
    return started_;
}

bool Sht4xDevice::read(float& celsius, float& humidityPct, std::uint32_t maxAgeMs) {
    const std::uint32_t now = millis();
    if (!lastOk_ || now - lastReadMs_ > maxAgeMs) {
        sensors_event_t humidity;
        sensors_event_t temperature;
        lastOk_ = started_ && sht_.getEvent(&humidity, &temperature);
        lastReadMs_ = now;
        if (lastOk_) {
            lastC_ = temperature.temperature;
            lastRh_ = humidity.relative_humidity;
        }
    }
    celsius = lastC_;
    humidityPct = lastRh_;
    return lastOk_;
}

bool Sht4xTemperature::readCelsius(float& celsius) {
    float unusedHumidity = 0.0f;
    return device_.read(celsius, unusedHumidity);
}

bool Sht4xHumidity::readRelativeHumidity(float& percent) {
    float unusedTemperature = 0.0f;
    return device_.read(unusedTemperature, percent);
}

bool Ds18b20Sensor::begin() {
    bus_.begin();
    return bus_.getDeviceCount() > index_;
}

bool Ds18b20Sensor::readCelsius(float& celsius) {
    // Blocking conversion (~750 ms at 12-bit). Acceptable for a 5 s interval in the
    // prototype; the next phase should use setWaitForConversion(false).
    bus_.requestTemperaturesByIndex(index_);
    celsius = bus_.getTempCByIndex(index_);
    return celsius != DEVICE_DISCONNECTED_C;
}

}  // namespace hal

#endif  // USE_REAL_SENSORS
