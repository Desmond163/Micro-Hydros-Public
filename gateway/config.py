"""Thresholds shared by the receiver and analysis.

Keep in sync with include/config.h in the firmware. The values are assumptions for the
prototype and must be confirmed with HydroGreen Fingers.
"""

CHANNELS = ("air_inside_c", "air_outside_c", "water_c", "humidity_inside_pct")

LABELS = {
    "air_inside_c": "Air inside (°C)",
    "air_outside_c": "Air outside (°C)",
    "water_c": "Water (°C)",
    "humidity_inside_pct": "Humidity inside (%RH)",
}

# (min, max) alarm thresholds; None = no alarm for that channel.
THRESHOLDS = {
    "air_inside_c": (18.0, 28.0),
    "air_outside_c": None,
    "water_c": (18.0, 24.0),
    "humidity_inside_pct": (40.0, 80.0),
}

# Early warning: warn if the trend predicts a threshold within this many minutes.
EARLY_WARNING_MINUTES = 60.0
# Number of most recent valid readings used for the trend.
TREND_WINDOW = 36
