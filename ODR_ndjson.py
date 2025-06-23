import json
import numpy as np
import matplotlib.pyplot as plt

# --- Load NDJSON Data ---
timestamps = []
with open("data.ndjson", "r") as f:
    for line in f:
        if line.strip():  # Avoid empty lines
            doc = json.loads(line)
            timestamps.append(doc["timestamp"])

# --- Compute Deltas ---
timestamps = np.array(timestamps)
deltas = np.diff(timestamps)

# --- Calculate Statistics ---
mean_delta = np.mean(deltas)
unique_gaps = sorted(set(deltas))
effective_odr = 1000 / mean_delta if mean_delta != 0 else 0

print(f"Average gap between timestamps: {mean_delta:.3f} ms")
print(f"Unique time gaps found: {unique_gaps}")
print(f"Estimated ODR: {effective_odr:.2f} Hz")

# --- Plot Histogram ---
plt.hist(deltas, bins=range(int(min(deltas)), int(max(deltas))+2))
plt.title("Histogram of Timestamp Gaps (ms)")
plt.xlabel("Time Gap (ms)")
plt.ylabel("Count")
plt.grid(True)
plt.show()
