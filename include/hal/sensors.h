#pragma once

// Hardware Abstraction Layer (HAL) for MicroHydros sensors.
//
// The application (SensorManager, validation, alarms, communication) only depends on
// these interfaces. A simulated implementation and real-hardware drivers implement the
// same interfaces, so switching from simulation to physical sensors means changing which
// objects are constructed in main.cpp, not the application logic.

namespace hal {

class ITemperatureSensor {
public:
    virtual ~ITemperatureSensor() = default;
    // Returns false if the sensor could not be initialised.
    virtual bool begin() = 0;
    // Returns false on a read/communication error. On success, `celsius` holds the value
    // (which may still be implausible; plausibility is checked by Validator).
    virtual bool readCelsius(float& celsius) = 0;
};

class IHumiditySensor {
public:
    virtual ~IHumiditySensor() = default;
    virtual bool begin() = 0;
    // Relative humidity in percent (0-100).
    virtual bool readRelativeHumidity(float& percent) = 0;
};

}  // namespace hal
