#pragma once

#include <cstdint>

// Sends payloads out of the device:
//  - always as one JSON line on Serial (works as a serial gateway and for debugging),
//  - and, with ENABLE_MQTT, to an MQTT broker. Payloads that cannot be published are kept in
//    a small FIFO and sent when the connection is back.
class Communication {
public:
    void begin(const char* deviceId);
    // Non-blocking: keeps Wi-Fi/MQTT connected and flushes buffered payloads.
    void loop();
    void send(const char* payload);

    bool mqttConnected() const;
    std::uint32_t bufferedCount() const;
    std::uint32_t droppedCount() const;
};
