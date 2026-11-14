#pragma once

#include <cstdint>

// Bit per measurement channel, used in fault and alarm masks.
enum ChannelBit : std::uint8_t {
    kAirInside = 1u << 0,
    kAirOutside = 1u << 1,
    kWater = 1u << 2,
    kHumidityInside = 1u << 3,
};

constexpr std::uint8_t kAllChannels = kAirInside | kAirOutside | kWater | kHumidityInside;

// Threshold alarms for plant-relevant limits (see include/config.h).
enum AlarmBit : std::uint8_t {
    kAlarmAirInsideLow = 1u << 0,
    kAlarmAirInsideHigh = 1u << 1,
    kAlarmWaterLow = 1u << 2,
    kAlarmWaterHigh = 1u << 3,
    kAlarmHumidityLow = 1u << 4,
    kAlarmHumidityHigh = 1u << 5,
};

struct MeasurementData {
    std::uint32_t sequence = 0;       // increments per reading; lets the receiver detect lost messages
    std::uint32_t timestampMs = 0;    // milliseconds since boot (not wall-clock time)
    float airInsideC = 0.0f;
    float airOutsideC = 0.0f;
    float waterC = 0.0f;
    float humidityInsidePct = 0.0f;
    std::uint8_t readFailures = 0;    // ChannelBit mask: HAL could not read the channel
    std::uint8_t implausible = 0;     // ChannelBit mask: value read but non-finite or out of range
    std::uint8_t alarms = 0;          // AlarmBit mask
    bool valid = false;               // true when all four channels were read and are plausible
};
