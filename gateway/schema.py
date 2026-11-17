"""Parsing and validation of measurement payloads (schema version 1)."""

from __future__ import annotations

import json
import math
from dataclasses import dataclass, field

from gateway.config import CHANNELS


class PayloadError(ValueError):
    """Raised when a payload is not a usable measurement."""


@dataclass
class Measurement:
    device: str
    seq: int | None
    uptime_ms: int | None
    values: dict[str, float | None]
    valid: bool
    faults: list[str] = field(default_factory=list)
    alarms: list[str] = field(default_factory=list)
    schema: int = 1
    time_scale: int = 1  # simulation speed-up; 1 on real hardware

    @property
    def env_minutes(self) -> float | None:
        """Device uptime converted to environment (simulated) minutes."""
        if self.uptime_ms is None:
            return None
        return self.uptime_ms / 60000.0 * self.time_scale


def _number_or_none(payload: dict, key: str) -> float | None:
    if key not in payload:
        raise PayloadError(f"missing field '{key}'")
    value = payload[key]
    if value is None:
        return None
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        raise PayloadError(f"field '{key}' must be a number or null")
    value = float(value)
    if not math.isfinite(value):
        raise PayloadError(f"field '{key}' is not finite")
    return value


def parse_measurement(raw: str | bytes) -> Measurement:
    """Parse one JSON payload. Accepts schema 1 and the legacy format from tools/demo.py."""
    if isinstance(raw, bytes):
        try:
            raw = raw.decode("utf-8")
        except UnicodeDecodeError as exc:
            raise PayloadError("payload is not UTF-8") from exc
    try:
        payload = json.loads(raw)
    except json.JSONDecodeError as exc:
        raise PayloadError(f"invalid JSON: {exc.msg}") from exc
    if not isinstance(payload, dict):
        raise PayloadError("payload must be a JSON object")

    values = {key: _number_or_none(payload, key) for key in CHANNELS}

    valid = payload.get("valid")
    if not isinstance(valid, bool):
        raise PayloadError("field 'valid' must be true or false")

    seq = payload.get("seq")
    if seq is not None and (isinstance(seq, bool) or not isinstance(seq, int) or seq < 0):
        raise PayloadError("field 'seq' must be a non-negative integer")

    uptime = payload.get("uptime_ms", payload.get("timestamp_ms"))
    time_scale = payload.get("time_scale", 1)
    if isinstance(time_scale, bool) or not isinstance(time_scale, int) or time_scale < 1:
        raise PayloadError("field 'time_scale' must be an integer >= 1")
    faults = payload.get("faults", [])
    alarms = payload.get("alarms", [])
    if not isinstance(faults, list) or not isinstance(alarms, list):
        raise PayloadError("'faults' and 'alarms' must be lists")

    # A reading marked valid must contain all four values.
    if valid and any(v is None for v in values.values()):
        raise PayloadError("reading marked valid but has missing values")

    return Measurement(
        device=str(payload.get("device", "legacy")),
        seq=seq,
        uptime_ms=uptime if isinstance(uptime, int) else None,
        values=values,
        valid=valid,
        faults=[str(f) for f in faults],
        alarms=[str(a) for a in alarms],
        schema=int(payload.get("schema", 0)),
        time_scale=time_scale,
    )
