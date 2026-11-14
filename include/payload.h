#pragma once

#include <cstddef>

#include "measurement_data.h"

namespace payload {

constexpr int kSchemaVersion = 1;
constexpr std::size_t kMaxPayloadSize = 384;

// Serialises a measurement to compact JSON. Faulty channels are written as null (NaN is not
// valid JSON). Returns the number of characters written, or 0 if `size` was too small.
//
// Example:
// {"schema":1,"device":"mh-01","seq":42,"uptime_ms":210000,"air_inside_c":23.41,
//  "air_outside_c":21.02,"water_c":20.55,"humidity_inside_pct":61.80,"valid":true,
//  "faults":[],"alarms":[]}
std::size_t toJson(const MeasurementData& measurement, const char* deviceId, char* out,
                   std::size_t size);

}  // namespace payload
