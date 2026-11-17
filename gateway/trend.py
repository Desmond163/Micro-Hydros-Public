"""Simple trend analysis: least-squares slope and time until a threshold is reached."""

from __future__ import annotations

from dataclasses import dataclass


@dataclass
class Trend:
    slope_per_min: float       # change per minute of device uptime
    latest: float
    minutes_to_limit: float | None  # None if not heading towards a limit
    limit: float | None


def slope(points: list[tuple[float, float]]) -> float | None:
    """Least-squares slope for (x, y) points. Returns None if it cannot be computed."""
    n = len(points)
    if n < 3:
        return None
    mean_x = sum(p[0] for p in points) / n
    mean_y = sum(p[1] for p in points) / n
    sxx = sum((p[0] - mean_x) ** 2 for p in points)
    if sxx == 0:
        return None
    sxy = sum((p[0] - mean_x) * (p[1] - mean_y) for p in points)
    return sxy / sxx


def recent_window(points_min: list[tuple[float, float]], minutes: float, min_points: int):
    """Points from the last `minutes`, extended backwards to at least `min_points`."""
    if not points_min:
        return []
    start = points_min[-1][0] - minutes
    window = [p for p in points_min if p[0] >= start]
    if len(window) < min_points:
        window = points_min[-min_points:]
    return window


def analyse(points_min: list[tuple[float, float]], limits: tuple[float, float] | None) -> Trend | None:
    """points_min: (minutes, value) pairs in time order."""
    s = slope(points_min)
    if s is None:
        return None
    latest = points_min[-1][1]
    minutes = None
    limit = None
    if limits is not None:
        low, high = limits
        if s > 0 and latest < high:
            limit, minutes = high, (high - latest) / s
        elif s < 0 and latest > low:
            limit, minutes = low, (latest - low) / -s
    return Trend(slope_per_min=s, latest=latest, minutes_to_limit=minutes, limit=limit)
