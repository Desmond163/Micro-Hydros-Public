"""Analyses stored history: statistics, faults, alarms, trends, plus a graph.

    python -m gateway.analyze --db data/microhydros.db --out reports

Writes reports/<device>_rapport.md and reports/<device>_graf.png.
"""

from __future__ import annotations

import argparse
import json
import sys
from collections import Counter
from pathlib import Path
from statistics import mean

from gateway.config import CHANNELS, THRESHOLDS, TREND_WINDOW
from gateway.storage import Store
from gateway.trend import analyse

SV_LABELS = {
    "air_inside_c": "Luft inne (°C)",
    "air_outside_c": "Luft ute (°C)",
    "water_c": "Vatten (°C)",
    "humidity_inside_pct": "Luftfuktighet inne (%RH)",
}


def channel_stats(rows, channel):
    values = [r[channel] for r in rows if r[channel] is not None]
    points = [(r["uptime_ms"] / 60000.0 * r["time_scale"], r[channel]) for r in rows
              if r[channel] is not None and r["uptime_ms"] is not None]
    trend = analyse(points[-TREND_WINDOW:], THRESHOLDS[channel]) if len(points) >= 3 else None
    return {
        "count": len(values),
        "missing": len(rows) - len(values),
        "min": min(values) if values else None,
        "max": max(values) if values else None,
        "mean": mean(values) if values else None,
        "latest": values[-1] if values else None,
        "trend": trend,
    }


def plot(rows, device: str, path: Path) -> bool:
    try:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
    except ImportError:
        return False

    fig, axes = plt.subplots(4, 1, figsize=(10, 10), sharex=True)
    for ax, channel in zip(axes, CHANNELS):
        xs = [r["uptime_ms"] / 60000.0 * r["time_scale"] for r in rows if r[channel] is not None and r["uptime_ms"] is not None]
        ys = [r[channel] for r in rows if r[channel] is not None and r["uptime_ms"] is not None]
        ax.plot(xs, ys, linewidth=1.4, color="#1f7a5a")
        faults = [r["uptime_ms"] / 60000.0 * r["time_scale"] for r in rows if r[channel] is None and r["uptime_ms"] is not None]
        if faults and ys:
            ax.scatter(faults, [min(ys)] * len(faults), marker="x", color="#c0392b", label="sensorfel", zorder=3)
        limits = THRESHOLDS[channel]
        if limits:
            for limit in limits:
                ax.axhline(limit, linestyle="--", color="#e67e22", linewidth=1)
        ax.set_ylabel(SV_LABELS[channel])
        ax.grid(alpha=0.3)
        if faults:
            ax.legend(loc="upper right")
    axes[-1].set_xlabel("Tid (minuter, simulerad miljötid)" if rows and rows[-1]["time_scale"] > 1 else "Drifttid (minuter)")
    fig.suptitle(f"MicroHydros {device} – mätvärden över tid (streckat = larmgräns)")
    fig.tight_layout()
    fig.savefig(path, dpi=120)
    plt.close(fig)
    return True


def fmt(value, digits=2):
    return "–" if value is None else f"{value:.{digits}f}"


def build_report(store: Store, device: str, graph_name: str | None) -> str:
    rows = store.measurements(device)
    events = store.events(device)
    if not rows:
        return f"# MicroHydros {device}\n\nIngen data lagrad.\n"

    valid_count = sum(r["valid"] for r in rows)
    alarm_counter = Counter(a for r in rows for a in json.loads(r["alarms"]))
    fault_counter = Counter(f for r in rows for f in json.loads(r["faults"]))
    event_counter = Counter(e["kind"] for e in events)
    span_min = None
    if rows[0]["uptime_ms"] is not None and rows[-1]["uptime_ms"] is not None:
        span_min = (rows[-1]["uptime_ms"] - rows[0]["uptime_ms"]) / 60000.0
    scale = rows[-1]["time_scale"]

    lines = [
        f"# MicroHydros {device} – dataanalys",
        "",
        f"Mätningar: **{len(rows)}** ({valid_count} giltiga, {len(rows) - valid_count} med sensorfel)"
        + (f" över {span_min:.0f} minuters drifttid." if span_min is not None else ".")
        + (f" Simuleringen gick {scale}× snabbare än verklig tid; trender anges i simulerad tid." if scale > 1 else ""),
        f"Tappade meddelanden (sekvensluckor): {sum(1 for e in events if e['kind'] == 'gap')} tillfällen. "
        f"Omstarter: {event_counter.get('restart', 0)}. Avvisade payloads: {event_counter.get('invalid_payload', 0)}.",
        "",
        "## Statistik per mätpunkt",
        "",
        "| Mätpunkt | Antal | Saknas | Min | Max | Medel | Senaste | Trend (/h) | Larmgräns nås om |",
        "|---|---|---|---|---|---|---|---|---|",
    ]
    insights = []
    for channel in CHANNELS:
        s = channel_stats(rows, channel)
        t = s["trend"]
        slope_h = None if t is None else t.slope_per_min * 60
        eta = "–"
        if t is not None and t.minutes_to_limit is not None:
            eta = (f"~{t.minutes_to_limit:.0f} min ({t.limit})" if t.minutes_to_limit <= 1440
                   else "> 1 dygn")
        lines.append(
            f"| {SV_LABELS[channel]} | {s['count']} | {s['missing']} | {fmt(s['min'])} | {fmt(s['max'])} | "
            f"{fmt(s['mean'])} | {fmt(s['latest'])} | {fmt(slope_h, 2)} | {eta} |"
        )
        limits = THRESHOLDS[channel]
        if t is not None and t.minutes_to_limit is not None and t.minutes_to_limit <= 60 and limits:
            insights.append(
                f"- **Tidig varning:** {SV_LABELS[channel]} ändras {slope_h:+.2f} per timme och når "
                f"gränsen {t.limit} om cirka {t.minutes_to_limit:.0f} minuter om trenden håller i sig."
            )

    lines += ["", "## Larm och fel", ""]
    if alarm_counter:
        for alarm, count in alarm_counter.most_common():
            lines.append(f"- Larm `{alarm}`: {count} mätningar")
    else:
        lines.append("- Inga tröskellarm.")
    if fault_counter:
        for fault, count in fault_counter.most_common():
            lines.append(f"- Sensorfel `{fault}`: {count} mätningar (värdet lagrat som saknat, inte som 0)")
    else:
        lines.append("- Inga sensorfel.")
    if event_counter.get("early_warning"):
        lines.append(f"- Tidiga varningar från mottagaren: {event_counter['early_warning']}")

    lines += ["", "## Insikter", ""]
    lines += insights or ["- Inga mätpunkter är på väg mot en larmgräns inom den närmaste timmen."]
    if fault_counter:
        lines.append(
            "- Sensorfel hanterades utan att systemet stannade: övriga mätpunkter fortsatte att "
            "rapporteras och felet syns i datan."
        )
    lines += [
        "",
        "## Rimlighet",
        "",
        "Värdena är simulerade (daglig cykel från odlingslampan plus brus) och bevisar inte "
        "sensornoggrannhet. Rimlighetskontroll: luft inne −10–60 °C, luft ute −30–60 °C, "
        "vatten 0–50 °C, fukt 0–100 %. Larmgränserna (luft inne 18–28 °C, vatten 18–24 °C, "
        "fukt 40–80 %) är antaganden som ska bekräftas av HydroGreen Fingers.",
    ]
    if graph_name:
        lines += ["", f"![Mätvärden över tid]({graph_name})"]
    return "\n".join(lines) + "\n"


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--db", default="data/microhydros.db")
    parser.add_argument("--device", help="device id (default: all devices in the database)")
    parser.add_argument("--out", default="reports", help="output directory")
    args = parser.parse_args(argv)

    if not Path(args.db).exists():
        print(f"database not found: {args.db}", file=sys.stderr)
        return 1
    store = Store(args.db)
    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)
    devices = [args.device] if args.device else store.devices()
    for device in devices:
        graph = out / f"{device}_graf.png"
        has_graph = plot(store.measurements(device), device, graph)
        report = build_report(store, device, graph.name if has_graph else None)
        (out / f"{device}_rapport.md").write_text(report, encoding="utf-8")
        print(f"wrote {out / (device + '_rapport.md')}" + (f" and {graph}" if has_graph else ""))
    store.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
