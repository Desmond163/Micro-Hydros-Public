#include <Arduino.h>

#include <algorithm>

#include "communication.h"
#include "config.h"
#include "payload.h"
#include "sensor_manager.h"

#if USE_REAL_SENSORS
#include <Wire.h>

#include "hal/real_sensors.h"
#else
#include "hal/arduino_clock.h"
#include "hal/simulated_sensors.h"
#include "simulation_profiles.h"
#endif

namespace {

// Pins (see diagram.json for the Wokwi wiring).
constexpr std::uint8_t kAlarmLedPin = 25;    // on while any threshold alarm or fault is active
constexpr std::uint8_t kOnlineLedPin = 26;   // on while connected to the MQTT broker
constexpr std::uint8_t kHeatButtonPin = 32;  // simulation only: toggles a heat event
constexpr std::uint8_t kFaultButtonPin = 33; // simulation only: injects sensor faults
constexpr std::uint8_t kOneWirePin = 4;      // real sensors only: DS18B20 bus

#if USE_REAL_SENSORS
hal::Sht4xDevice sht4x(Wire);
hal::Sht4xTemperature airInside(sht4x);
hal::Sht4xHumidity humidityInside(sht4x);
OneWire oneWire(kOneWirePin);
DallasTemperature dallas(&oneWire);
hal::Ds18b20Sensor water(dallas, 0);
hal::Ds18b20Sensor airOutside(dallas, 1);
#else
hal::ArduinoClock clock;
hal::SimulatedTemperatureSensor airInside(clock, simulation::kAirInside, 101, config::kSimTimeScale);
hal::SimulatedTemperatureSensor airOutside(clock, simulation::kAirOutside, 202, config::kSimTimeScale);
hal::SimulatedTemperatureSensor water(clock, simulation::kWater, 303, config::kSimTimeScale);
hal::SimulatedHumiditySensor humidityInside(clock, simulation::kHumidityInside, 404, config::kSimTimeScale);

// Heat event: offsets ramp gradually so the trend is visible before the alarm triggers.
bool heatEventActive = false;
float heatOffsetC = 0.0f;
constexpr float kHeatTargetC = 7.0f;
constexpr float kHeatStepC = 0.5f;

class Button {
public:
    explicit Button(std::uint8_t pin) : pin_(pin) {}
    void begin() { pinMode(pin_, INPUT_PULLUP); }
    // True once per press (active low, 50 ms debounce).
    bool pressed() {
        const bool down = digitalRead(pin_) == LOW;
        const std::uint32_t now = millis();
        if (down != lastDown_ && now - lastChangeMs_ > 50) {
            lastChangeMs_ = now;
            lastDown_ = down;
            return down;
        }
        return false;
    }

private:
    std::uint8_t pin_;
    bool lastDown_ = false;
    std::uint32_t lastChangeMs_ = 0;
};

Button heatButton(kHeatButtonPin);
Button faultButton(kFaultButtonPin);

void handleSimulationButtons() {
    if (heatButton.pressed()) {
        heatEventActive = !heatEventActive;
        Serial.println(heatEventActive ? "SIM: heat event ON" : "SIM: heat event OFF");
    }
    if (faultButton.pressed()) {
        water.channel().injectFault(hal::Fault::ReadError, 3);
        humidityInside.channel().injectFault(hal::Fault::NotANumber, 2);
        Serial.println("SIM: injected 3 water read errors and 2 NaN humidity readings");
    }
}

void stepHeatEvent() {
    const float target = heatEventActive ? kHeatTargetC : 0.0f;
    if (heatOffsetC < target) heatOffsetC = std::min(target, heatOffsetC + kHeatStepC);
    if (heatOffsetC > target) heatOffsetC = std::max(target, heatOffsetC - kHeatStepC);
    airInside.channel().setOffset(heatOffsetC);
    // Warmer air holds more moisture, so relative humidity falls.
    humidityInside.channel().setOffset(-1.5f * heatOffsetC);
}
#endif

SensorManager sensorManager(airInside, airOutside, water, humidityInside);
Communication communication;
std::uint32_t lastMeasurementMs = 0;
char payloadBuffer[payload::kMaxPayloadSize];

}  // namespace

void setup() {
    Serial.begin(115200);
    pinMode(kAlarmLedPin, OUTPUT);
    pinMode(kOnlineLedPin, OUTPUT);

#if USE_REAL_SENSORS
    Wire.begin();
    Serial.println("MicroHydros prototype started (REAL sensors)");
#else
    heatButton.begin();
    faultButton.begin();
    Serial.println("MicroHydros prototype started (SIMULATED sensors)");
#endif

    if (!sensorManager.begin()) {
        Serial.println("WARNING: one or more sensors failed to start; faults will be reported");
    }
    communication.begin(config::kDeviceId);
}

void loop() {
    communication.loop();
    digitalWrite(kOnlineLedPin, communication.mqttConnected() ? HIGH : LOW);

#if !USE_REAL_SENSORS
    handleSimulationButtons();
#endif

    const std::uint32_t now = millis();
    if (now - lastMeasurementMs < config::kMeasurementIntervalMs) {
        return;  // non-blocking schedule; no delay()
    }
    lastMeasurementMs = now;

#if !USE_REAL_SENSORS
    stepHeatEvent();
#endif

    const MeasurementData measurement = sensorManager.read(now);
    digitalWrite(kAlarmLedPin, (measurement.alarms != 0 || !measurement.valid) ? HIGH : LOW);

    if (payload::toJson(measurement, config::kDeviceId, payloadBuffer, sizeof(payloadBuffer)) > 0) {
        communication.send(payloadBuffer);
    } else {
        Serial.println("ERROR: payload did not fit in buffer");
    }
}
