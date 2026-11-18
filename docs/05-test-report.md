# Test Plan and Results

## Evidence rule

A test is marked **Passed** only after it has been run and the output observed. Simulation does not prove
sensor accuracy; Serial output does not prove MQTT delivery; a local broker does not prove the public
broker. Each row states where it was run.

## Test environment (runs on 14 Nov 2026)

Linux build machine (Desmond, with AI assistance): PlatformIO Core 6.2.0, espressif32 platform 8.0.0,
g++ 13.3.0, Python 3.13, paho-mqtt 2.2.0, matplotlib 3.11.2, Mosquitto 2.0.18 (local broker).
**Still to run by the team** on their own machines and in Wokwi: see "Open tests" below.

## How to run

```bash
pio test -e native                    # firmware unit tests
python -m pytest gateway              # gateway tests
pio run -e esp32dev -e wokwi -e esp32dev_real -e native_sim
```

## Results

### Unit and build tests

| ID | What | How | Expected | Actual | Status |
|---|---|---|---|---|---|
| T-01 | Plausibility validation (normal, boundaries, out of range, NaN, per-channel mask, spike) | `pio test -e native` → test_validator | 6 tests pass | 6/6 passed | Passed |
| T-02 | Simulated sensors: deterministic, stay in profile over a day, time scaling, fault injection (read error, NaN, spike), offset | test_simulation | 6 tests pass | 6/6 passed | Passed |
| T-03 | SensorManager through HAL: valid reading, sequence numbers, read failure isolated to one channel, implausible value is a fault not an alarm, threshold alarms, works with simulated HAL and with hand-written fakes | test_sensor_manager | 6 tests pass | 6/6 passed | Passed |
| T-04 | JSON payload: exact format, `null` for faulty channels, no `nan`, worst case fits, too-small buffer returns 0 | test_payload | 4 tests pass | 4/4 passed | Passed |
| T-05 | Offline buffer: FIFO order, overflow drops oldest and counts, safe truncation | test_buffer | 3 tests pass | 3/3 passed | Passed |
| T-06 | Gateway: payload validation (7 bad inputs rejected, legacy format accepted), sequence gap/duplicate/restart, trend and time to limit, receiver stores/rejects/detects gaps, early warning on rising temperature, no warning on a normal day at 120x or in the first noisy seconds, time scale | `python -m pytest gateway` | 19 tests pass | 19/19 passed | Passed |
| T-07 | Firmware builds | `pio run` for esp32dev, wokwi, esp32dev_real, native_sim | All succeed | All succeed. esp32dev: RAM 6.9 %, flash 21.1 %; wokwi (Wi-Fi + MQTT): RAM 17.0 %, flash 68.5 %; esp32dev_real: RAM 7.3 %, flash 22.6 % | Passed |

### End-to-end tests (host build of the firmware core)

| ID | What | How | Expected | Actual | Status |
|---|---|---|---|---|---|
| T-08 | Repeated measurements, fault handling and storage | 400 readings, 120x time, faults injected at reading 100, heat event at 300, three lines removed (148→152), one malformed payload; `program … \| gateway.receiver --stdin` | All good readings stored; faulty channels stored as missing; gap of 3 detected; malformed payload rejected; system continues | 397 stored (394 valid, 3 with faults: water ×3, humidity ×2), gap "3 message(s) lost between seq 148 and 152", 1 rejected payload, receiver kept running | Passed |
| T-09 | Early warning before alarm | Same run as T-08 | Warning before `air_inside_high` | Warning at reading 302 ("+2.42/h, reaches 28.0 in ~38 min"), first alarm at reading 305 (= 30 simulated minutes later) | Passed |
| T-10 | No false warnings in normal operation | 2000 readings each at 1x, 10x and 120x, no heat event | 0 warnings | 0 / 0 / 0 | Passed |
| T-11 | MQTT delivery and status | Local Mosquitto; receiver subscribed to `microhydros/+/measurement`; 60 readings (faults at 20) published with QoS 1; retained `online` on status topic; one non-JSON message | 60 stored, status event, non-JSON rejected | 60 stored (57 valid), status `online` logged, 1 rejected ("invalid JSON") | Passed |
| T-12 | Analysis report and graph | `python -m gateway.analyze` on T-08 data | Statistics per channel, faults, alarms, insights, PNG | Report and graph produced; faults shown as missing values, not 0 | Passed |
| T-13 | Firmware in Wokwi publishes to public broker | `pio run -e wokwi`, start Wokwi, receiver with `--mqtt-host broker.hivemq.com --device mh-grupp6` | Readings arrive every 5 s | Run 18 Nov 2026 on Desmond's MacBook: 43+ readings stored in SQLite, one every ~10 s wall clock (5 s device time at the simulator's ~51 % speed), `online`/`offline` status events logged, restart detected (seq 5→0). LED state not checked | Passed (LED not verified) |
| T-14 | Fault button in Wokwi | Press blue button | `faults` lists water/humidity for a few readings; then recovers | Readings 29–31: `faults` = water + humidity_inside, values stored as missing, `valid`=0; from reading 32 `faults`=[] again. Other channels kept reporting. Red LED not checked | Passed (LED not verified) |
| T-15 | Heat event in Wokwi | Press orange button | Receiver warning before `air_inside_high` | Early warning at 19:08:09 ("+1.98/h, reaches 28.0 in ~20 min"), `air_inside_high` at reading 29 (28.28 °C), air inside plateaus at ~33 °C; second early warning for humidity (−4.30/h). Red LED not checked | Passed (LED not verified) |

## Open tests (team)

| ID | What | How | Expected | Owner | Status |
|---|---|---|---|---|---|
| T-16 | Broker outage | Stop receiver/disconnect network in Wokwi or block broker, then restore | Device buffers up to 20 payloads, sends them after reconnect; gateway reports a gap only if more were lost | Khaled | Not run |
| T-17 | Long run (stability) | Wokwi or physical ESP32 for ≥ 30 min | No reboot (`seq` keeps increasing), no growing gaps | Khaled | Not run |
| T-18 | Tests on a second machine | Run `pio test -e native` and `pytest` on Windows | Same results as above | Gayathri, Khaled | Not run |
| T-19 | Physical sensors | Requires hardware | Readings within datasheet accuracy vs reference | — | Out of scope (no hardware) |

## Reasonableness of the values

- The simulated values follow a daily cycle driven by the grow light: inside air 21–26 °C, outside air
  20–22 °C, water 19.7–21.3 °C lagging behind the air, humidity 54–70 %RH moving opposite to the
  temperature (warmer air → lower relative humidity). Those are plausible for an indoor hydroponic unit,
  but they are **our assumptions**, not measurements.
- During the simulated heat event the inside air reached 33.1 °C and humidity fell to 42.9 %RH; the
  alarm triggered above 28 °C as designed.
- Values a working sensor cannot produce (NaN, 150 °C spike, negative humidity) are rejected as faults and
  stored as missing, so they never distort statistics or trigger misleading alarms.
- Threshold values (inside 18–28 °C, water 18–24 °C, RH 40–80 %) are assumptions and must be confirmed with
  HydroGreen Fingers.
