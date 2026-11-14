#pragma once

// Physical sensor drivers behind the same HAL interfaces as the simulation.
// Compiled only with -D USE_REAL_SENSORS=1 (environment esp32dev_real).
// STATUS: compiles, but NOT tested against hardware - the team has no components.

#if USE_REAL_SENSORS

#include <cstdint>

#include <Adafruit_SHT4x.h>
#include <DallasTemperature.h>
#include <OneWire.h>

#include "hal/sensors.h"

namespace hal {

// One SHT40 measures both air temperature and relative humidity inside the unit.
// A single read is shared by the two adapters below.
class Sht4xDevice {
public:
    explicit Sht4xDevice(TwoWire& wire) : wire_(wire) {}
    bool begin();
    // Performs a new measurement if the cached one is older than maxAgeMs.
    bool read(float& celsius, float& humidityPct, std::uint32_t maxAgeMs = 500);

private:
    TwoWire& wire_;
    Adafruit_SHT4x sht_;
    bool started_ = false;
    bool lastOk_ = false;
    std::uint32_t lastReadMs_ = 0;
    float lastC_ = 0.0f;
    float lastRh_ = 0.0f;
};

class Sht4xTemperature : public ITemperatureSensor {
public:
    explicit Sht4xTemperature(Sht4xDevice& device) : device_(device) {}
    bool begin() override { return device_.begin(); }
    bool readCelsius(float& celsius) override;

private:
    Sht4xDevice& device_;
};

class Sht4xHumidity : public IHumiditySensor {
public:
    explicit Sht4xHumidity(Sht4xDevice& device) : device_(device) {}
    bool begin() override { return device_.begin(); }
    bool readRelativeHumidity(float& percent) override;

private:
    Sht4xDevice& device_;
};

// DS18B20 on a shared 1-Wire bus: index 0 = water probe, index 1 = outside-air probe.
// Next phase: map sensors by ROM address instead of bus index so the order is stable.
class Ds18b20Sensor : public ITemperatureSensor {
public:
    Ds18b20Sensor(DallasTemperature& bus, std::uint8_t index) : bus_(bus), index_(index) {}
    bool begin() override;
    bool readCelsius(float& celsius) override;

private:
    DallasTemperature& bus_;
    std::uint8_t index_;
};

}  // namespace hal

#endif  // USE_REAL_SENSORS
