"""Receives MicroHydros measurements, stores them in SQLite and raises early warnings.

Examples:
    # From an MQTT broker (the ESP32 / Wokwi publishes here)
    python -m gateway.receiver --mqtt-host broker.hivemq.com --device mh-grupp6

    # From JSON lines on stdin (host simulation or a serial port dump)
    .pio/build/native_sim/program --count 200 | python -m gateway.receiver --stdin
"""

from __future__ import annotations

import argparse
import sys
from collections import deque

from gateway.config import CHANNELS, EARLY_WARNING_MINUTES, THRESHOLDS, TREND_MIN_POINTS, TREND_MIN_SPAN_MINUTES, TREND_MINUTES
from gateway.schema import PayloadError, parse_measurement
from gateway.sequence import SequenceTracker
from gateway.storage import Store
from gateway.trend import analyse, recent_window

SHORT = {"air_inside_c": "in", "air_outside_c": "out", "water_c": "water", "humidity_inside_pct": "rh"}


class Receiver:
    def __init__(self, store: Store, out=sys.stdout) -> None:
        self.store = store
        self.out = out
        self.tracker = SequenceTracker()
        self.history: dict[tuple[str, str], deque] = {}
        self.warning_active: set[tuple[str, str]] = set()
        self.stats = {"stored": 0, "rejected": 0, "gaps": 0, "warnings": 0}

    def log(self, message: str) -> None:
        print(message, file=self.out, flush=True)

    def handle(self, raw: str | bytes) -> None:
        try:
            m = parse_measurement(raw)
        except PayloadError as exc:
            # Bad input must not stop the gateway: record it and continue.
            self.stats["rejected"] += 1
            self.store.add_event(None, "invalid_payload", str(exc))
            self.log(f"REJECTED  {exc}")
            return

        event = self.tracker.observe(m.device, m.seq)
        if event:
            self.store.add_event(m.device, event.kind, event.detail)
            if event.kind == "gap":
                self.stats["gaps"] += event.missing
            self.log(f"{event.kind.upper():9} {m.device}: {event.detail}")

        self.store.add_measurement(m)
        self.stats["stored"] += 1

        status = "OK" if m.valid else "FAULT " + ",".join(m.faults)
        alarms = (" ALARM " + ",".join(m.alarms)) if m.alarms else ""
        values = " ".join(
            f"{SHORT[c]}={'--' if m.values[c] is None else format(m.values[c], '.2f')}"
            for c in CHANNELS
        )
        self.log(f"{m.device} seq={m.seq} {values} {status}{alarms}")
        self._early_warning(m)

    def _early_warning(self, m) -> None:
        minutes = m.env_minutes
        if minutes is None:
            return
        for channel in CHANNELS:
            value = m.values[channel]
            limits = THRESHOLDS[channel]
            if value is None or limits is None:
                continue
            key = (m.device, channel)
            history = self.history.setdefault(key, deque(maxlen=2000))
            if history and minutes < history[-1][0]:
                history.clear()  # device restarted, uptime reset
            history.append((minutes, value))
            if len(history) < TREND_MIN_POINTS or history[-1][0] - history[0][0] < TREND_MIN_SPAN_MINUTES:
                continue
            trend = analyse(recent_window(list(history), TREND_MINUTES, TREND_MIN_POINTS), limits)
            low, high = limits
            already_outside = not (low <= value <= high)
            heading_out = (
                trend is not None
                and trend.minutes_to_limit is not None
                and trend.minutes_to_limit <= EARLY_WARNING_MINUTES
            )
            if heading_out and not already_outside and key not in self.warning_active:
                self.warning_active.add(key)
                self.stats["warnings"] += 1
                detail = (
                    f"{channel} {trend.slope_per_min * 60:+.2f}/h, reaches {trend.limit} in "
                    f"~{trend.minutes_to_limit:.0f} min (environment time)"
                )
                self.store.add_event(m.device, "early_warning", detail)
                self.log(f"WARNING   {m.device}: {detail}")
            elif not heading_out and key in self.warning_active:
                self.warning_active.discard(key)


def run_stdin(receiver: Receiver) -> None:
    for line in sys.stdin:
        line = line.strip()
        if line.startswith("{"):
            receiver.handle(line)
        elif line:
            receiver.log(f"device log: {line}")  # e.g. "INFO: MQTT connected" from Serial


def run_mqtt(receiver: Receiver, host: str, port: int, device: str) -> None:
    try:
        import paho.mqtt.client as mqtt
    except ImportError:
        sys.exit("paho-mqtt is required: pip install -r requirements.txt")

    measurement_topic = f"microhydros/{device}/measurement"
    status_topic = f"microhydros/{device}/status"

    def on_connect(client, userdata, flags, reason_code, properties):
        receiver.log(f"connected to {host}:{port} ({reason_code}); subscribing to {measurement_topic}")
        client.subscribe([(measurement_topic, 1), (status_topic, 1)])

    def on_message(client, userdata, message):
        if message.topic.endswith("/status"):
            state = message.payload.decode(errors="replace")
            dev = message.topic.split("/")[1]
            receiver.store.add_event(dev, "status", state)
            receiver.log(f"STATUS    {dev}: {state}")
        else:
            receiver.handle(message.payload)

    def on_disconnect(client, userdata, flags, reason_code, properties):
        receiver.log(f"disconnected ({reason_code}); paho will reconnect automatically")

    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    client.on_connect = on_connect
    client.on_message = on_message
    client.on_disconnect = on_disconnect
    client.reconnect_delay_set(min_delay=1, max_delay=30)
    client.connect(host, port, keepalive=30)
    client.loop_forever()


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--db", default="data/microhydros.db", help="SQLite file for history")
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument("--stdin", action="store_true", help="read JSON lines from stdin")
    source.add_argument("--mqtt-host", help="MQTT broker host")
    parser.add_argument("--mqtt-port", type=int, default=1883)
    parser.add_argument("--device", default="+", help="device id to subscribe to ('+' = all)")
    args = parser.parse_args(argv)

    store = Store(args.db)
    receiver = Receiver(store)
    try:
        if args.stdin:
            run_stdin(receiver)
        else:
            run_mqtt(receiver, args.mqtt_host, args.mqtt_port, args.device)
    except KeyboardInterrupt:
        pass
    finally:
        s = receiver.stats
        receiver.log(
            f"summary: stored={s['stored']} rejected={s['rejected']} lost={s['gaps']} "
            f"early_warnings={s['warnings']} db={args.db}"
        )
        store.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
