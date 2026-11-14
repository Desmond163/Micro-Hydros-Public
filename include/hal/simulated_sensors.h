#pragma once

#include <cstdint>

#include "hal/clock.h"
#include "hal/sensors.h"

namespace hal {

// Describes a simulated signal: a daily cycle (driven by the grow light) plus small noise.
struct SignalProfile {
    float base;        // mean value
    float amplitude;   // daily swing; negative values invert the cycle (e.g. humidity)
    float periodS;     // cycle length in simulated seconds (86400 = one day)
    float phaseS;      // shifts the cycle, e.g. water lags behind air temperature
    float noise;       // max random deviation per reading (+/-)
};

enum class Fault : std::uint8_t {
    None,
    ReadError,   // driver reports failure (e.g. sensor unplugged, CRC error)
    NotANumber,  // driver returns NaN
    Spike,       // physically impossible jump
};

// One simulated measurement channel. Deterministic for a given seed and clock, which makes
// it testable. timeScale > 1 compresses simulated time so a full day cycle is visible in a
// short demo.
class SimulatedChannel {
public:
    SimulatedChannel(const Clock& clock, SignalProfile profile, std::uint32_t seed,
                     std::uint32_t timeScale = 1);

    bool read(float& value);

    // The next `count` reads will exhibit `fault`.
    void injectFault(Fault fault, std::uint16_t count = 1);
    // Adds a constant offset, e.g. to simulate a heat event from lamps or a broken fan.
    void setOffset(float offset) { offset_ = offset; }
    float offset() const { return offset_; }

    // Noise-free signal value at the current time (useful for tests and plausibility checks).
    float idealValue() const;

private:
    float nextNoise();

    const Clock& clock_;
    SignalProfile profile_;
    std::uint32_t rngState_;
    std::uint32_t timeScale_;
    float offset_ = 0.0f;
    Fault pendingFault_ = Fault::None;
    std::uint16_t pendingFaultCount_ = 0;
};

class SimulatedTemperatureSensor : public ITemperatureSensor {
public:
    SimulatedTemperatureSensor(const Clock& clock, SignalProfile profile, std::uint32_t seed,
                               std::uint32_t timeScale = 1)
        : channel_(clock, profile, seed, timeScale) {}

    bool begin() override { return true; }
    bool readCelsius(float& celsius) override { return channel_.read(celsius); }
    SimulatedChannel& channel() { return channel_; }

private:
    SimulatedChannel channel_;
};

class SimulatedHumiditySensor : public IHumiditySensor {
public:
    SimulatedHumiditySensor(const Clock& clock, SignalProfile profile, std::uint32_t seed,
                            std::uint32_t timeScale = 1)
        : channel_(clock, profile, seed, timeScale) {}

    bool begin() override { return true; }
    bool readRelativeHumidity(float& percent) override;
    SimulatedChannel& channel() { return channel_; }

private:
    SimulatedChannel channel_;
};

}  // namespace hal
