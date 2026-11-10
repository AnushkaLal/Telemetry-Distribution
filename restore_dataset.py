import csv
import random

SOURCE = "data/segments_official.csv"
FULL_OUTPUT = "data/ops_sat_segments.csv"
SAMPLE_OUTPUT = "data/telemetry_real.csv"

rows = []

with open(SOURCE, "r", newline="") as source:
    reader = csv.DictReader(source)

    for row in reader:
        rows.append({
            "timestamp": row["timestamp"],
            "satellite": "OPS-SAT",
            "sensor": row["channel"],
            "value": row["value"],
            "anomaly": row["anomaly"]
        })

# Recreate the full dataset in the project's original schema.
fields = ["timestamp", "satellite", "sensor", "value", "anomaly"]

with open(FULL_OUTPUT, "w", newline="") as output:
    writer = csv.DictWriter(output, fieldnames=fields)
    writer.writeheader()
    writer.writerows(rows)

# Recreate the deterministic 5,000-record sample.
random.seed(42)
sample = random.sample(rows, 5000)
sample.sort(key=lambda row: row["timestamp"])

with open(SAMPLE_OUTPUT, "w", newline="") as output:
    writer = csv.DictWriter(output, fieldnames=fields)
    writer.writeheader()
    writer.writerows(sample)

print("Dataset restoration complete.")
print(f"Full dataset: {len(rows)} records")
print(f"Simulation sample: {len(sample)} records")
print(f"Created: {FULL_OUTPUT}")
print(f"Created: {SAMPLE_OUTPUT}")
