# Blebox Erasmus Internship — Technical Task

**Candidate:** Serhat Varli  
**University:** Kocaeli University, Electronics and Communication Engineering  

---

## Task 1 — Sensor Data Logger (C++)

Collects temperature and rain data from the API every 2 seconds for 10 minutes and saves results to `data.csv` in MST timezone (UTC-7).

**Dependencies:**
- libcurl4-openssl-dev
- nlohmann-json3-dev

**Build & Run:**
```bash
cd task1
g++ data_logger.cpp -o data_logger -lcurl -std=c++17
./data_logger
```

**Output format (data.csv):**
```
date_time,temperature,rain
2026-03-22 00:35:06,-16.2,0
```

---

## Task 2 — Server Location

The server referenced by `innov8dev.com` is located in **France**.

- IP: `176.31.121.176`
- City: Lille, Hauts-de-France
- Hosting: OVH SAS (AS16276)

---

## Task 3 — Query Script (Python)

Reads the CSV from Task 1 and returns:
- Temperature for a given timestamp (with linear interpolation if needed)
- Rain percentage for a given time range

**Usage:**
```bash
cd task3

# Temperature at a specific time
python3 query.py temp "2026-03-22 00:40:00"

# Rain percentage over a time range
python3 query.py rain "2026-03-22 00:35:00" "2026-03-22 00:45:00"
```
