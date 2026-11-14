#pragma once

#include <Arduino.h>

#include "hal/clock.h"

namespace hal {

class ArduinoClock : public Clock {
public:
    std::uint32_t nowMs() const override { return millis(); }
};

}  // namespace hal
