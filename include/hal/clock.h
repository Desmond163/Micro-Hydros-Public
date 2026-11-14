#pragma once

#include <cstdint>

namespace hal {

// Time source abstraction so simulated sensors and tests do not depend on Arduino millis().
class Clock {
public:
    virtual ~Clock() = default;
    virtual std::uint32_t nowMs() const = 0;
};

// Deterministic clock for native tests and the host simulation.
class ManualClock : public Clock {
public:
    std::uint32_t nowMs() const override { return nowMs_; }
    void set(std::uint32_t ms) { nowMs_ = ms; }
    void advance(std::uint32_t ms) { nowMs_ += ms; }

private:
    std::uint32_t nowMs_ = 0;
};

}  // namespace hal
