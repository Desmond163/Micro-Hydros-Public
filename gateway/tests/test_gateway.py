import io
import json

import pytest

from gateway.receiver import Receiver
from gateway.schema import PayloadError, parse_measurement
from gateway.sequence import SequenceTracker
from gateway.storage import Store
from gateway.trend import analyse, recent_window, slope

VALID = {
    "schema": 1, "device": "mh-test", "seq": 0, "uptime_ms": 0,
    "air_inside_c": 23.4, "air_outside_c": 21.0, "water_c": 20.5, "humidity_inside_pct": 61.8,
    "valid": True, "faults": [], "alarms": [],
}


def payload(**changes):
    data = dict(VALID)
    data.update(changes)
    return json.dumps(data)


# --- schema --------------------------------------------------------------------------

def test_parses_valid_payload():
    m = parse_measurement(payload())
    assert m.device == "mh-test" and m.seq == 0 and m.valid
    assert m.values["water_c"] == 20.5


def test_null_value_for_faulty_channel():
    m = parse_measurement(payload(water_c=None, valid=False, faults=["water"]))
    assert m.values["water_c"] is None and m.faults == ["water"]


@pytest.mark.parametrize("raw", [
    "not json",
    "[1,2,3]",
    payload(water_c="hot"),
    payload(valid="yes"),
    payload(seq=-1),
    payload(water_c=None),  # marked valid but value missing
    json.dumps({k: v for k, v in VALID.items() if k != "humidity_inside_pct"}),
])
def test_rejects_bad_payloads(raw):
    with pytest.raises(PayloadError):
        parse_measurement(raw)


def test_accepts_legacy_demo_format():
    legacy = {"timestamp_ms": 5000, "air_inside_c": 22.2, "air_outside_c": 18.1, "water_c": 20.1,
              "humidity_inside_pct": 56.5, "valid": True}
    m = parse_measurement(json.dumps(legacy))
    assert m.device == "legacy" and m.seq is None and m.uptime_ms == 5000


# --- sequence ------------------------------------------------------------------------

def test_sequence_gap_duplicate_restart():
    t = SequenceTracker()
    assert t.observe("d", 0) is None
    assert t.observe("d", 1) is None
    gap = t.observe("d", 4)
    assert gap.kind == "gap" and gap.missing == 2
    assert t.observe("d", 4).kind == "duplicate"
    assert t.observe("d", 0).kind == "restart"
    assert t.observe("other", 7) is None  # tracked per device


# --- trend ---------------------------------------------------------------------------

def test_slope_and_time_to_limit():
    points = [(float(i), 20.0 + 0.1 * i) for i in range(10)]  # +0.1 per minute
    assert slope(points) == pytest.approx(0.1)
    t = analyse(points, (18.0, 28.0))
    assert t.limit == 28.0
    assert t.minutes_to_limit == pytest.approx((28.0 - 20.9) / 0.1)


def test_flat_signal_has_no_limit_eta():
    t = analyse([(float(i), 22.0) for i in range(10)], (18.0, 28.0))
    assert t.slope_per_min == 0 and t.minutes_to_limit is None


# --- receiver end to end --------------------------------------------------------------

def test_receiver_stores_rejects_and_detects_gap():
    store = Store(":memory:")
    out = io.StringIO()
    r = Receiver(store, out=out)
    r.handle(payload(seq=0))
    r.handle("garbage")
    r.handle(payload(seq=3, uptime_ms=15000))
    assert r.stats == {"stored": 2, "rejected": 1, "gaps": 2, "warnings": 0}
    kinds = [e["kind"] for e in store.events()]
    assert "invalid_payload" in kinds and "gap" in kinds
    assert len(store.measurements("mh-test")) == 2


def test_receiver_early_warning_on_rising_temperature():
    store = Store(":memory:")
    r = Receiver(store, out=io.StringIO())
    for i in range(40):  # +0.15 C per minute from 22 C -> passes the 1 h horizon to 28 C
        r.handle(payload(seq=i, uptime_ms=i * 60000, air_inside_c=22.0 + 0.15 * i))
    assert r.stats["warnings"] == 1
    assert any(e["kind"] == "early_warning" for e in store.events())


def test_time_scale_converts_to_environment_minutes():
    m = parse_measurement(payload(uptime_ms=60000, time_scale=120))
    assert m.env_minutes == pytest.approx(120.0)
    with pytest.raises(PayloadError):
        parse_measurement(payload(time_scale=0))


def test_daily_cycle_at_120x_does_not_raise_early_warning():
    import math
    store = Store(":memory:")
    r = Receiver(store, out=io.StringIO())
    for i in range(300):  # 5 s readings at 120x = 10 simulated minutes apart
        sim_min = i * 10
        value = 23.5 + 2.5 * math.sin(2 * math.pi * sim_min / 1440)
        r.handle(payload(seq=i, uptime_ms=i * 5000, time_scale=120, air_inside_c=round(value, 2)))
    assert r.stats["warnings"] == 0


def test_recent_window_uses_time_but_keeps_min_points():
    points = [(float(i * 10), float(i)) for i in range(20)]  # one point per 10 minutes
    assert len(recent_window(points, 30.0, 6)) == 6
    dense = [(i / 12.0, 1.0) for i in range(1000)]  # one point per 5 s
    assert len(recent_window(dense, 30.0, 6)) == 361


def test_no_trend_warning_from_first_noisy_seconds():
    store = Store(":memory:")
    r = Receiver(store, out=io.StringIO())
    noise = [0.0, 0.3, -0.2, 0.4, -0.1, 0.5, 0.6, 0.7]
    for i, n in enumerate(noise):  # 40 s of data with a steep apparent slope
        r.handle(payload(seq=i, uptime_ms=i * 5000, air_inside_c=26.0 + n))
    assert r.stats["warnings"] == 0
