// Host (PC) build of the same firmware core: HAL simulation, SensorManager, validation,
// alarms and JSON payload. Used for testing the data path without an ESP32:
//
//   pio run -e native_sim
//   .pio/build/native_sim/program --count 120 | python -m gateway.receiver --stdin
//
// It is not a replacement for the ESP32 firmware; Arduino-specific parts (Wi-Fi, MQTT,
// LEDs, buttons) only exist in src/main.cpp and src/communication.cpp.

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>

#include "config.h"
#include "hal/clock.h"
#include "hal/simulated_sensors.h"
#include "payload.h"
#include "sensor_manager.h"
#include "simulation_profiles.h"

namespace {

struct Options {
    long count = 20;            // 0 = run forever
    long intervalMs = 5000;     // device interval between readings
    long timeScale = 1;         // simulated-time acceleration
    long faultAt = -1;          // reading index where faults are injected
    long heatAt = -1;           // reading index where a heat event starts
    bool realtime = false;      // sleep between readings instead of advancing a fake clock
    const char* deviceId = "mh-host";
};

void usage() {
    std::puts(
        "usage: program [--count N] [--interval-ms MS] [--time-scale X] [--fault-at I]\n"
        "               [--heat-at I] [--realtime] [--device ID]");
}

bool parse(int argc, char** argv, Options& o) {
    for (int i = 1; i < argc; ++i) {
        auto next = [&](long& target) {
            if (i + 1 >= argc) return false;
            target = std::strtol(argv[++i], nullptr, 10);
            return true;
        };
        if (!std::strcmp(argv[i], "--count")) { if (!next(o.count)) return false; }
        else if (!std::strcmp(argv[i], "--interval-ms")) { if (!next(o.intervalMs)) return false; }
        else if (!std::strcmp(argv[i], "--time-scale")) { if (!next(o.timeScale)) return false; }
        else if (!std::strcmp(argv[i], "--fault-at")) { if (!next(o.faultAt)) return false; }
        else if (!std::strcmp(argv[i], "--heat-at")) { if (!next(o.heatAt)) return false; }
        else if (!std::strcmp(argv[i], "--realtime")) { o.realtime = true; }
        else if (!std::strcmp(argv[i], "--device") && i + 1 < argc) { o.deviceId = argv[++i]; }
        else { return false; }
    }
    return o.count >= 0 && o.intervalMs > 0 && o.timeScale > 0;
}

}  // namespace

int main(int argc, char** argv) {
    Options o;
    if (!parse(argc, argv, o)) {
        usage();
        return 2;
    }

    hal::ManualClock clock;
    const auto scale = static_cast<std::uint32_t>(o.timeScale);
    hal::SimulatedTemperatureSensor airInside(clock, simulation::kAirInside, 101, scale);
    hal::SimulatedTemperatureSensor airOutside(clock, simulation::kAirOutside, 202, scale);
    hal::SimulatedTemperatureSensor water(clock, simulation::kWater, 303, scale);
    hal::SimulatedHumiditySensor humidity(clock, simulation::kHumidityInside, 404, scale);
    SensorManager manager(airInside, airOutside, water, humidity);
    manager.begin();

    char buffer[payload::kMaxPayloadSize];
    float heatOffset = 0.0f;
    for (long i = 0; o.count == 0 || i < o.count; ++i) {
        clock.set(static_cast<std::uint32_t>(i * o.intervalMs));
        if (i == o.faultAt) {
            water.channel().injectFault(hal::Fault::ReadError, 3);
            humidity.channel().injectFault(hal::Fault::NotANumber, 2);
        }
        if (o.heatAt >= 0 && i >= o.heatAt && heatOffset < 7.0f) {
            heatOffset += 0.5f;
        }
        airInside.channel().setOffset(heatOffset);
        humidity.channel().setOffset(-1.5f * heatOffset);

        const MeasurementData m = manager.read(clock.nowMs());
        if (payload::toJson(m, o.deviceId, buffer, sizeof(buffer), static_cast<unsigned long>(o.timeScale)) > 0) {
            std::puts(buffer);
            std::fflush(stdout);
        }
        if (o.realtime) {
            std::this_thread::sleep_for(std::chrono::milliseconds(o.intervalMs));
        }
    }
    return 0;
}
