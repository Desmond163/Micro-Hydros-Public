# Sensor Decisions

## Criteria

DHT11 and DHT22 are prohibited (the customer's previous supplier had poor quality and stability with them).
We weighted the criteria for MicroHydros as follows:

1. **Suitability for a humid, enclosed growing environment** (condensation, drift at high RH)
2. **Accuracy and repeatability** good enough to see trends and set alarm thresholds
3. **Digital interface** supported by the ESP32 and with error detection (CRC)
4. **Integration effort and availability** for a small team, with established Arduino libraries
5. **Cost and power**, lower priority because the unit is mains powered

## Decision

| Measurement | Sensor | Key data (manufacturer) | Why |
|---|---|---|---|
| Inside air temperature + RH | **Sensirion SHT40** (I2C) | RH ±1.8 %RH typ., T ±0.2 °C typ.; response τ63 4 s (RH), 2 s (T); 1.08–3.6 V; I2C with CRC-8 [1] | One component gives both inside values; datasheet states it is functional in condensing environments and has a heater to remove condensation/creep [1] |
| Water / nutrient temperature | **DS18B20** in a sealed stainless probe (1-Wire) | ±0.5 °C from −10 to +85 °C; 9–12 bit; max 750 ms conversion at 12 bit; 3.0–5.5 V [2] | Widely available as waterproof probe; digital, so cable length does not add analog error |
| Outside air temperature | **DS18B20** on the same 1-Wire bus | as above; unique 64-bit ROM code allows several sensors on one pin [2][3] | Only temperature is needed outside; sharing the bus saves a pin and a driver |

Gayathri's earlier comparison (see Git history of this file) also proposed the SHT40 for the inside; we kept it. For the outside we chose
a DS18B20 instead of a second SHT40 because two SHT40-A parts share I2C address 0x44 [1] and would need a
second I2C bus or a different address variant.

### Caveats

- Sustained operation above 80 %RH can temporarily offset the SHT40 humidity signal (datasheet example:
  +3 %RH after 60 h); liquid water on the sensor blocks RH response until it evaporates [1]. Mount it away
  from droplets, preferably a variant with protective membrane, and evaluate periodic heater pulses.
- The DS18B20 IC is not waterproof by itself; verify the probe's seal, cable entry and wetted materials.
  Clone ICs are common, so buy from a traceable source.
- Accuracy figures are manufacturer values for the bare IC. They are not measurements made by us. Before
  claiming accuracy, compare each assembled channel with a reference instrument.

## Alternatives considered

- **SHT31:** established I2C T/RH sensor with similar integration; we followed the earlier SHT40 choice.
- **BME280:** adds pressure, which is not needed.
- **TMP117:** precise temperature only; does not cover humidity and is not a water probe.

## Integration notes (for the next phase)

SHT40 on the ESP32 default I2C pins (SDA 21, SCL 22) with the breakout's pull-ups. DS18B20 bus on GPIO 4
with a 4.7 kΩ pull-up, sensors identified by ROM address (the prototype driver uses bus index, which is not
stable). Keep the inside sensor away from LED drivers and the pump; shade and ventilate the outside sensor;
keep electronics out of the reservoir.

## Sources

1. Sensirion, *Datasheet SHT4x*, v7.3: https://sensirion.com/media/documents/33FD6951/6A7C10A0/HT_DS_Datasheet_SHT4x_V7.3.pdf
2. Analog Devices, *DS18B20 datasheet* (Rev. 6): https://www.analog.com/media/en/technical-documentation/data-sheets/DS18B20.pdf
3. Analog Devices, DS18B20 product page: https://www.analog.com/en/products/ds18b20.html
