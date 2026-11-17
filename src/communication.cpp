#include <Arduino.h>

#include "communication.h"

#if defined(ENABLE_MQTT)
#include <PubSubClient.h>
#include <WiFi.h>

#include "payload.h"
#include "payload_buffer.h"

#if defined(WOKWI)
// Wokwi simulator: built-in guest network and a public broker (no credentials needed).
// Public brokers are readable by anyone - only simulated, non-sensitive data is sent.
#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASSWORD ""
#define MQTT_HOST "broker.hivemq.com"
#define MQTT_PORT 1883
#else
#include "secrets.h"  // copy include/secrets.example.h -> include/secrets.h
#endif

namespace {
WiFiClient networkClient;
PubSubClient mqttClient(networkClient);

constexpr std::uint32_t kWifiRetryMs = 10000;
constexpr std::uint32_t kMqttRetryMs = 5000;
constexpr std::size_t kFlushPerLoop = 5;

PayloadBuffer<20, payload::kMaxPayloadSize> pending;
char deviceIdBuf[32] = "mh-01";
char measurementTopic[64];
char statusTopic[64];
std::uint32_t lastWifiAttemptMs = 0;
std::uint32_t lastMqttAttemptMs = 0;

bool tryConnectMqtt() {
    // Last Will: the broker publishes "offline" if the device disappears without notice.
    const bool ok = mqttClient.connect(deviceIdBuf, statusTopic, 1, true, "offline");
    if (ok) {
        mqttClient.publish(statusTopic, "online", true);
        Serial.println("INFO: MQTT connected");
    } else {
        Serial.print("WARNING: MQTT connect failed, state=");
        Serial.println(mqttClient.state());
    }
    return ok;
}

void flushPending() {
    for (std::size_t i = 0; i < kFlushPerLoop && !pending.empty(); ++i) {
        if (!mqttClient.publish(measurementTopic, pending.front())) {
            return;  // try again on the next loop
        }
        pending.pop();
    }
}
}  // namespace
#endif

void Communication::begin(const char* deviceId) {
#if defined(ENABLE_MQTT)
    strncpy(deviceIdBuf, deviceId, sizeof(deviceIdBuf) - 1);
    snprintf(measurementTopic, sizeof(measurementTopic), "microhydros/%s/measurement", deviceIdBuf);
    snprintf(statusTopic, sizeof(statusTopic), "microhydros/%s/status", deviceIdBuf);

    mqttClient.setServer(MQTT_HOST, MQTT_PORT);
    // PubSubClient's default packet limit (256 B) is smaller than our payload.
    mqttClient.setBufferSize(512);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    lastWifiAttemptMs = millis();
#else
    (void)deviceId;
#endif
}

void Communication::loop() {
#if defined(ENABLE_MQTT)
    const std::uint32_t now = millis();
    if (WiFi.status() != WL_CONNECTED) {
        if (now - lastWifiAttemptMs >= kWifiRetryMs) {
            lastWifiAttemptMs = now;
            Serial.println("WARNING: Wi-Fi disconnected, reconnecting");
            WiFi.disconnect();
            WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        }
        return;
    }
    if (!mqttClient.connected()) {
        if (now - lastMqttAttemptMs >= kMqttRetryMs) {
            lastMqttAttemptMs = now;
            tryConnectMqtt();
        }
        return;
    }
    mqttClient.loop();
    flushPending();
#endif
}

void Communication::send(const char* payloadText) {
    Serial.println(payloadText);

#if defined(ENABLE_MQTT)
    // Keep order: older buffered payloads go first.
    if (mqttClient.connected() && pending.empty() &&
        mqttClient.publish(measurementTopic, payloadText)) {
        return;
    }
    if (!pending.push(payloadText)) {
        Serial.println("WARNING: offline buffer full, oldest payload dropped");
    }
#endif
}

bool Communication::mqttConnected() const {
#if defined(ENABLE_MQTT)
    return mqttClient.connected();
#else
    return false;
#endif
}

std::uint32_t Communication::bufferedCount() const {
#if defined(ENABLE_MQTT)
    return pending.size();
#else
    return 0;
#endif
}

std::uint32_t Communication::droppedCount() const {
#if defined(ENABLE_MQTT)
    return pending.dropped();
#else
    return 0;
#endif
}
