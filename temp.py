import json
import time

f = open("data.json", "a")
for i in range(15000):
    f.write(json.dumps({"sample": i}) + "\n")
    f.flush()
    time.sleep(0.001)  # simulate 1 kHz
f.close()
# This script appends 15,000 samples to "data.json" with a 1 ms interval between each sample.