#!/usr/bin/env python3
"""
Blebox Erasmus Internship — Task 3
Reads the CSV file produced by Task 1 and provides:
  1. Temperature at a given timestamp (with linear interpolation if needed)
  2. Rain percentage over a given time range
"""

import sys
import csv
from datetime import datetime

# ─── Constants ──────────────────────────────────────────────────────────────
CSV_FILE  = "../task1/data.csv"
DT_FORMAT = "%Y-%m-%d %H:%M:%S"


# ─── CSV Loading ─────────────────────────────────────────────────────────────
def load_csv(filepath: str) -> list[dict]:
    """Reads the CSV file and returns a list of records sorted by time."""
    records = []
    with open(filepath, newline="", encoding="utf-8") as f:
        reader = csv.DictReader(f)
        for row in reader:
            records.append({
                "dt":          datetime.strptime(row["date_time"], DT_FORMAT),
                "temperature": float(row["temperature"]),
                "rain":        int(row["rain"])
            })
    records.sort(key=lambda r: r["dt"])
    return records


# ─── Task 3a: Temperature Query (with interpolation) ────────────────────────
def get_temperature(records: list[dict], query_dt: datetime) -> float:
    """
    Returns the temperature at the given timestamp.
    If no exact match exists, uses linear interpolation between
    the nearest preceding and following records.
    """
    # Exact match
    for r in records:
        if r["dt"] == query_dt:
            return r["temperature"]

    before = [r for r in records if r["dt"] < query_dt]
    after  = [r for r in records if r["dt"] > query_dt]

    if not before:
        raise ValueError(f"Query time ({query_dt}) is before the dataset start.")
    if not after:
        raise ValueError(f"Query time ({query_dt}) is after the dataset end.")

    r0 = before[-1]
    r1 = after[0]

    # Linear interpolation: T = T0 + (T1 - T0) * (t - t0) / (t1 - t0)
    t0 = r0["dt"].timestamp()
    t1 = r1["dt"].timestamp()
    t  = query_dt.timestamp()

    ratio = (t - t0) / (t1 - t0)
    interpolated = r0["temperature"] + (r1["temperature"] - r0["temperature"]) * ratio
    return round(interpolated, 2)


# ─── Task 3b: Rain Percentage ────────────────────────────────────────────────
def get_rain_percentage(records: list[dict], start_dt: datetime, end_dt: datetime) -> float:
    """
    Returns the percentage of time it was raining within the given range.
    """
    in_range = [r for r in records if start_dt <= r["dt"] <= end_dt]

    if not in_range:
        raise ValueError(f"No records found between {start_dt} and {end_dt}.")

    rain_count = sum(1 for r in in_range if r["rain"] == 1)
    return round((rain_count / len(in_range)) * 100, 1)


# ─── Usage ───────────────────────────────────────────────────────────────────
def print_usage():
    print("""
Usage:

  Temperature query:
    python3 query.py temp "2026-03-22 00:40:00"

  Rain percentage:
    python3 query.py rain "2026-03-22 00:35:00" "2026-03-22 00:45:00"
""")


# ─── Main ────────────────────────────────────────────────────────────────────
def main():
    if len(sys.argv) < 3:
        print_usage()
        sys.exit(1)

    command = sys.argv[1].lower()
    records = load_csv(CSV_FILE)

    if command == "temp":
        query_dt = datetime.strptime(sys.argv[2], DT_FORMAT)
        temp = get_temperature(records, query_dt)
        print(f"Temperature [{sys.argv[2]}]: {temp}°C")

    elif command == "rain":
        if len(sys.argv) < 4:
            print("Error: 'rain' command requires start and end timestamps.")
            print_usage()
            sys.exit(1)
        start_dt = datetime.strptime(sys.argv[2], DT_FORMAT)
        end_dt   = datetime.strptime(sys.argv[3], DT_FORMAT)
        pct = get_rain_percentage(records, start_dt, end_dt)
        print(f"Rain percentage [{sys.argv[2]} — {sys.argv[3]}]: {pct}%")

    else:
        print(f"Unknown command: '{command}'")
        print_usage()
        sys.exit(1)


if __name__ == "__main__":
    main()