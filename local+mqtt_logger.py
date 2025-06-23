import struct
import time
import json
from datetime import datetime
import paho.mqtt.client as mqtt
from pymongo import MongoClient
import threading

# --- MQTT Settings ---
MQTT_BROKER = "broker.hivemq.com"
MQTT_PORT = 1883
MQTT_TOPIC = "esp32/sensor/stream"

# --- Struct Format ---
STRUCT_FORMAT = "<Ifffffffff"
STRUCT_SIZE = struct.calcsize(STRUCT_FORMAT)
SAMPLES_PER_MQTT_MESSAGE = 100
MQTT_PAYLOAD_SIZE = STRUCT_SIZE * SAMPLES_PER_MQTT_MESSAGE

# --- Output File ---
OUTPUT_FILE = "data.ndjson"
upload_buffer = []

# --- MongoDB Settings ---
MONGO_URI = "mongodb+srv://nicla_user:nicla@cluster0.fkzil.mongodb.net/?retryWrites=true&w=majority&appName=Cluster0"
MONGO_DB = "test"
MONGO_COLL = "Esp32"
collection = MongoClient(MONGO_URI)[MONGO_DB][MONGO_COLL]

# --- MQTT Callbacks ---
def on_connect(client, userdata, flags, rc):
    print("✅ Connected to MQTT Broker:", rc)
    client.subscribe(MQTT_TOPIC)

def on_message(client, userdata, msg):
    if len(msg.payload) != MQTT_PAYLOAD_SIZE:
        print("⚠️ Invalid payload size:", len(msg.payload))
        return

    try:
        with open(OUTPUT_FILE, "a") as f:
            for i in range(SAMPLES_PER_MQTT_MESSAGE):
                offset = i * STRUCT_SIZE
                sample = msg.payload[offset : offset + STRUCT_SIZE]
                unpacked = struct.unpack(STRUCT_FORMAT, sample)

                doc = {
                    "timestamp": unpacked[0],
                    "temperature": unpacked[1],
                    "pressure": unpacked[2],
                    "humidity": unpacked[3],
                    "accelX": unpacked[4],
                    "accelY": unpacked[5],
                    "accelZ": unpacked[6],
                    "gyroX": unpacked[7],
                    "gyroY": unpacked[8],
                    "gyroZ": unpacked[9]
                }

                json_line = json.dumps(doc)
                f.write(json_line + "\n")
                upload_buffer.append(doc)

        now = datetime.now().strftime("%H:%M:%S.%f")[:-3]
        print(f"💾 {now} | Appended {SAMPLES_PER_MQTT_MESSAGE} samples to {OUTPUT_FILE}")

    except Exception as e:
        print("❌ Error writing to file:", e)

# --- Periodic MongoDB Upload ---
def upload_loop():
    global upload_buffer
    while True:
        time.sleep(10)
        if upload_buffer:
            try:
                collection.insert_many(upload_buffer.copy(), ordered=False)
                print(f"📤 Uploaded {len(upload_buffer)} samples to MongoDB")
                upload_buffer.clear()
            except Exception as e:
                print("❌ MongoDB Upload Error:", e)

# --- Start MongoDB Uploader Thread ---
threading.Thread(target=upload_loop, daemon=True).start()

# --- MQTT Setup ---
client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

print("🔌 Connecting to MQTT broker...")
client.connect(MQTT_BROKER, MQTT_PORT, keepalive=60)
client.loop_start()

# --- Keep running ---
while True:
    time.sleep(1)
