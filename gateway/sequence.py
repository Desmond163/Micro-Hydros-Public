"""Detects lost, duplicated and restarted message sequences per device."""

from __future__ import annotations

from dataclasses import dataclass


@dataclass
class SequenceEvent:
    kind: str  # "gap", "duplicate" or "restart"
    detail: str
    missing: int = 0


class SequenceTracker:
    def __init__(self) -> None:
        self._last: dict[str, int] = {}

    def observe(self, device: str, seq: int | None) -> SequenceEvent | None:
        if seq is None:  # legacy payloads have no sequence number
            return None
        last = self._last.get(device)
        self._last[device] = seq
        if last is None or seq == last + 1:
            return None
        if seq == last:
            return SequenceEvent("duplicate", f"seq {seq} received twice")
        if seq > last + 1:
            missing = seq - last - 1
            return SequenceEvent("gap", f"{missing} message(s) lost between seq {last} and {seq}", missing)
        # The firmware starts at 0 after boot, so a lower number means a restart.
        return SequenceEvent("restart", f"seq went from {last} to {seq} (device restart?)")
