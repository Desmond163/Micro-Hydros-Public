"""SQLite storage of measurement history and gateway events."""

from __future__ import annotations

import json
import sqlite3
from datetime import datetime, timezone
from pathlib import Path

from gateway.config import CHANNELS
from gateway.schema import Measurement

SCHEMA = """
CREATE TABLE IF NOT EXISTS measurements (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    received_at TEXT NOT NULL,
    device TEXT NOT NULL,
    seq INTEGER,
    uptime_ms INTEGER,
    time_scale INTEGER NOT NULL DEFAULT 1,
    air_inside_c REAL,
    air_outside_c REAL,
    water_c REAL,
    humidity_inside_pct REAL,
    valid INTEGER NOT NULL,
    faults TEXT NOT NULL,
    alarms TEXT NOT NULL
);
CREATE INDEX IF NOT EXISTS idx_measurements_device ON measurements(device, id);
CREATE TABLE IF NOT EXISTS events (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    received_at TEXT NOT NULL,
    device TEXT,
    kind TEXT NOT NULL,
    detail TEXT NOT NULL
);
"""


def utc_now() -> str:
    return datetime.now(timezone.utc).isoformat(timespec="seconds")


class Store:
    def __init__(self, path: str | Path) -> None:
        if str(path) != ":memory:":
            Path(path).parent.mkdir(parents=True, exist_ok=True)
        self.conn = sqlite3.connect(str(path))
        self.conn.row_factory = sqlite3.Row
        self.conn.executescript(SCHEMA)

    def add_measurement(self, m: Measurement, received_at: str | None = None) -> None:
        self.conn.execute(
            "INSERT INTO measurements (received_at, device, seq, uptime_ms, time_scale, air_inside_c, "
            "air_outside_c, water_c, humidity_inside_pct, valid, faults, alarms) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
            (
                received_at or utc_now(),
                m.device,
                m.seq,
                m.uptime_ms,
                m.time_scale,
                *(m.values[c] for c in CHANNELS),
                int(m.valid),
                json.dumps(m.faults),
                json.dumps(m.alarms),
            ),
        )
        self.conn.commit()

    def add_event(self, device: str | None, kind: str, detail: str) -> None:
        self.conn.execute(
            "INSERT INTO events (received_at, device, kind, detail) VALUES (?, ?, ?, ?)",
            (utc_now(), device, kind, detail),
        )
        self.conn.commit()

    def measurements(self, device: str | None = None) -> list[sqlite3.Row]:
        if device:
            return self.conn.execute(
                "SELECT * FROM measurements WHERE device = ? ORDER BY id", (device,)
            ).fetchall()
        return self.conn.execute("SELECT * FROM measurements ORDER BY id").fetchall()

    def events(self, device: str | None = None) -> list[sqlite3.Row]:
        if device:
            return self.conn.execute(
                "SELECT * FROM events WHERE device = ? OR device IS NULL ORDER BY id", (device,)
            ).fetchall()
        return self.conn.execute("SELECT * FROM events ORDER BY id").fetchall()

    def devices(self) -> list[str]:
        return [r[0] for r in self.conn.execute("SELECT DISTINCT device FROM measurements ORDER BY 1")]

    def close(self) -> None:
        self.conn.close()
