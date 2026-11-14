#include "payload.h"

#include <cstdarg>
#include <cstdio>

namespace payload {

namespace {

class Writer {
public:
    Writer(char* out, std::size_t size) : out_(out), size_(size) {
        if (size_ > 0) out_[0] = '\0';
    }

    void append(const char* format, ...) {
        if (failed_) return;
        va_list args;
        va_start(args, format);
        const int written = std::vsnprintf(out_ + length_, size_ - length_, format, args);
        va_end(args);
        if (written < 0 || static_cast<std::size_t>(written) >= size_ - length_) {
            failed_ = true;
            return;
        }
        length_ += static_cast<std::size_t>(written);
    }

    std::size_t result() const { return failed_ ? 0 : length_; }

private:
    char* out_;
    std::size_t size_;
    std::size_t length_ = 0;
    bool failed_ = false;
};

void appendValue(Writer& w, const char* key, float value, bool faulty) {
    if (faulty) {
        w.append(",\"%s\":null", key);
    } else {
        w.append(",\"%s\":%.2f", key, static_cast<double>(value));
    }
}

struct NamedBit {
    std::uint8_t bit;
    const char* name;
};

constexpr NamedBit kChannelNames[] = {
    {kAirInside, "air_inside"},
    {kAirOutside, "air_outside"},
    {kWater, "water"},
    {kHumidityInside, "humidity_inside"},
};

constexpr NamedBit kAlarmNames[] = {
    {kAlarmAirInsideLow, "air_inside_low"}, {kAlarmAirInsideHigh, "air_inside_high"},
    {kAlarmWaterLow, "water_low"},          {kAlarmWaterHigh, "water_high"},
    {kAlarmHumidityLow, "humidity_low"},    {kAlarmHumidityHigh, "humidity_high"},
};

template <std::size_t N>
void appendNameList(Writer& w, const char* key, std::uint8_t mask, const NamedBit (&names)[N]) {
    w.append(",\"%s\":[", key);
    bool first = true;
    for (const NamedBit& entry : names) {
        if (mask & entry.bit) {
            w.append(first ? "\"%s\"" : ",\"%s\"", entry.name);
            first = false;
        }
    }
    w.append("]");
}

}  // namespace

std::size_t toJson(const MeasurementData& m, const char* deviceId, char* out, std::size_t size,
                   unsigned long timeScale) {
    if (out == nullptr || size == 0) return 0;

    const std::uint8_t faulty = m.readFailures | m.implausible;
    Writer w(out, size);
    w.append("{\"schema\":%d,\"device\":\"%s\",\"seq\":%lu,\"uptime_ms\":%lu,\"time_scale\":%lu",
             kSchemaVersion, deviceId, static_cast<unsigned long>(m.sequence),
             static_cast<unsigned long>(m.timestampMs), timeScale);
    appendValue(w, "air_inside_c", m.airInsideC, faulty & kAirInside);
    appendValue(w, "air_outside_c", m.airOutsideC, faulty & kAirOutside);
    appendValue(w, "water_c", m.waterC, faulty & kWater);
    appendValue(w, "humidity_inside_pct", m.humidityInsidePct, faulty & kHumidityInside);
    w.append(",\"valid\":%s", m.valid ? "true" : "false");
    appendNameList(w, "faults", faulty, kChannelNames);
    appendNameList(w, "alarms", m.alarms, kAlarmNames);
    w.append("}");

    const std::size_t length = w.result();
    if (length == 0) out[0] = '\0';
    return length;
}

}  // namespace payload
