#include "hal/simulated_sensors.h"

#include <cmath>
#include <limits>

namespace hal {

namespace {
constexpr double kTwoPi = 6.283185307179586;
}

SimulatedChannel::SimulatedChannel(const Clock& clock, SignalProfile profile, std::uint32_t seed,
                                   std::uint32_t timeScale)
    : clock_(clock),
      profile_(profile),
      rngState_(seed == 0 ? 0x9E3779B9u : seed),
      timeScale_(timeScale == 0 ? 1 : timeScale) {}

float SimulatedChannel::idealValue() const {
    const double simulatedSeconds = (static_cast<double>(clock_.nowMs()) / 1000.0) * timeScale_;
    const double angle = kTwoPi * (simulatedSeconds + profile_.phaseS) / profile_.periodS;
    return profile_.base + profile_.amplitude * static_cast<float>(std::sin(angle)) + offset_;
}

float SimulatedChannel::nextNoise() {
    // xorshift32: small, deterministic and identical on ESP32 and host.
    rngState_ ^= rngState_ << 13;
    rngState_ ^= rngState_ >> 17;
    rngState_ ^= rngState_ << 5;
    const float unit = static_cast<float>(rngState_ & 0xFFFFu) / 65535.0f;  // 0..1
    return (unit * 2.0f - 1.0f) * profile_.noise;
}

void SimulatedChannel::injectFault(Fault fault, std::uint16_t count) {
    pendingFault_ = fault;
    pendingFaultCount_ = (fault == Fault::None) ? 0 : count;
}

bool SimulatedChannel::read(float& value) {
    if (pendingFaultCount_ > 0) {
        --pendingFaultCount_;
        const Fault fault = pendingFault_;
        if (pendingFaultCount_ == 0) {
            pendingFault_ = Fault::None;
        }
        switch (fault) {
            case Fault::ReadError:
                return false;
            case Fault::NotANumber:
                value = std::numeric_limits<float>::quiet_NaN();
                return true;
            case Fault::Spike:
                value = idealValue() + 150.0f;
                return true;
            case Fault::None:
                break;
        }
    }
    value = idealValue() + nextNoise();
    return true;
}

bool SimulatedHumiditySensor::readRelativeHumidity(float& percent) {
    // The default profile stays well inside 0-100 %, so no clamping is needed. Injected
    // faults (NaN, spike) are passed through on purpose so validation can catch them.
    return channel_.read(percent);
}

}  // namespace hal
