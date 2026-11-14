#pragma once

#include "hal/simulated_sensors.h"

// Simulated environment for a MicroHydros unit indoors with a grow light on a daily cycle.
// The numbers are plausible assumptions for demonstration, not measured data.
namespace simulation {

constexpr float kDayS = 86400.0f;

// Inside air warms up when the grow light is on.
constexpr hal::SignalProfile kAirInside{23.5f, 2.5f, kDayS, 0.0f, 0.15f};
// Room air outside the unit varies less.
constexpr hal::SignalProfile kAirOutside{21.0f, 1.0f, kDayS, 0.0f, 0.10f};
// Water has thermal mass: smaller swing and lags about two hours behind the air.
constexpr hal::SignalProfile kWater{20.5f, 0.8f, kDayS, -7200.0f, 0.05f};
// Relative humidity drops when the air warms (inverse cycle).
constexpr hal::SignalProfile kHumidityInside{62.0f, -8.0f, kDayS, 0.0f, 0.8f};

}  // namespace simulation
