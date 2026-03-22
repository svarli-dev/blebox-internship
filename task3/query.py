#!/usr/bin/env python3
"""
Blebox Erasmus Staj Mülakatı — Görev 3
CSV dosyasından:
  1. Verilen timestamp için sıcaklık (gerekirse interpolasyon ile)
  2. Verilen zaman aralığında yağmur yüzdesi
"""

import sys
import csv
from datetime import datetime


# ─── Sabitler ───────────────────────────────────────────────────────────────
CSV_FILE    = "../task1/data.csv"          # Görev 1'in ürettiği CSV
DT_FORMAT   = "%Y-%m-%d %H:%M:%S"         # CSV'deki tarih formatı


# ─── CSV Okuma ───────────────────────────────────────────────────────────────
def load_csv(filepath: str) -> list[dict]:
    """CSV dosyasını okur, her satırı dict olarak döner."""
    records = []
    with open(filepath, newline="", encoding="utf-8") as f:
        reader = csv.DictReader(f)
        for row in reader:
            records.append({
                "dt":          datetime.strptime(row["date_time"], DT_FORMAT),
                "temperature": float(row["temperature"]),
                "rain":        int(row["rain"])
            })
    # Zaman sırasına göre sırala (güvenlik için)
    records.sort(key=lambda r: r["dt"])
    return records


# ─── Görev 3a: Sıcaklık Sorgusu (interpolasyon) ─────────────────────────────
def get_temperature(records: list[dict], query_dt: datetime) -> float:
    """
    Verilen timestamp için sıcaklığı döner.
    Tam eşleşme yoksa doğrusal interpolasyon (linear interpolation) yapar.
    """
    # Tam eşleşme var mı?
    for r in records:
        if r["dt"] == query_dt:
            return r["temperature"]

    # Sorgu zamanından önce ve sonra en yakın kayıtları bul
    before = [r for r in records if r["dt"] < query_dt]
    after  = [r for r in records if r["dt"] > query_dt]

    if not before:
        raise ValueError(f"Sorgu zamanı ({query_dt}) veri setinin başından önce.")
    if not after:
        raise ValueError(f"Sorgu zamanı ({query_dt}) veri setinin sonundan sonra.")

    r0 = before[-1]   # Hemen önceki kayıt
    r1 = after[0]     # Hemen sonraki kayıt

    # Doğrusal interpolasyon formülü:
    # T = T0 + (T1 - T0) * (t - t0) / (t1 - t0)
    t0 = r0["dt"].timestamp()
    t1 = r1["dt"].timestamp()
    t  = query_dt.timestamp()

    ratio = (t - t0) / (t1 - t0)
    interpolated = r0["temperature"] + (r1["temperature"] - r0["temperature"]) * ratio
    return round(interpolated, 2)


# ─── Görev 3b: Yağmur Yüzdesi ────────────────────────────────────────────────
def get_rain_percentage(records: list[dict], start_dt: datetime, end_dt: datetime) -> float:
    """
    Verilen zaman aralığındaki kayıtlarda yağmur yağma yüzdesini döner.
    """
    # Aralıktaki kayıtları filtrele (başlangıç ve bitiş dahil)
    in_range = [r for r in records if start_dt <= r["dt"] <= end_dt]

    if not in_range:
        raise ValueError(f"Belirtilen aralıkta ({start_dt} — {end_dt}) kayıt bulunamadı.")

    rain_count = sum(1 for r in in_range if r["rain"] == 1)
    percentage = (rain_count / len(in_range)) * 100
    return round(percentage, 1)


# ─── Kullanım Kılavuzu ────────────────────────────────────────────────────────
def print_usage():
    print("""
Kullanım:

  Sıcaklık sorgusu:
    python3 query.py temp "2026-03-22 00:40:00"

  Yağmur yüzdesi:
    python3 query.py rain "2026-03-22 00:35:00" "2026-03-22 00:45:00"
""")


# ─── Ana Program ─────────────────────────────────────────────────────────────
def main():
    if len(sys.argv) < 3:
        print_usage()
        sys.exit(1)

    command = sys.argv[1].lower()
    records = load_csv(CSV_FILE)

    if command == "temp":
        query_dt = datetime.strptime(sys.argv[2], DT_FORMAT)
        temp = get_temperature(records, query_dt)
        print(f"Sıcaklık [{sys.argv[2]}]: {temp}°C")

    elif command == "rain":
        if len(sys.argv) < 4:
            print("Hata: 'rain' komutu için başlangıç ve bitiş zamanı gerekli.")
            print_usage()
            sys.exit(1)
        start_dt = datetime.strptime(sys.argv[2], DT_FORMAT)
        end_dt   = datetime.strptime(sys.argv[3], DT_FORMAT)
        pct = get_rain_percentage(records, start_dt, end_dt)
        print(f"Yağmur yüzdesi [{sys.argv[2]} — {sys.argv[3]}]: %{pct}")

    else:
        print(f"Bilinmeyen komut: '{command}'")
        print_usage()
        sys.exit(1)


if __name__ == "__main__":
    main()