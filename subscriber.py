import struct
import time
from datetime import datetime
from pymongo import MongoClient
import paho.mqtt.client as mqtt
import threading
from queue import Queue

# --- MQTT Settings ---
MQTT_BROKER = "broker.hivemq.com"
MQTT_PORT = 1883
MQTT_TOPIC = "esp32/sensor/stream"

# --- MongoDB Settings ---
MONGO_URI = "mongodb+srv://nicla_user:nicla@cluster0.fkzil.mongodb.net/?retryWrites=true&w=majority&appName=Cluster0"
MONGO_DB = "test"
MONGO_COLL = "Esp32"
BATCH_SIZE = 200  # Insert after collecting 200 samples

# --- Struct Format ---
STRUCT_FORMAT = "<Ifffffffff"
STRUCT_SIZE = struct.calcsize(STRUCT_FORMAT)
SAMPLES_PER_MQTT_MESSAGE = 100
MQTT_PAYLOAD_SIZE = STRUCT_SIZE * SAMPLES_PER_MQTT_MESSAGE

# --- Mongo Setup ---
mongo_client = MongoClient(MONGO_URI)
collection = mongo_client[MONGO_DB][MONGO_COLL]

# --- Thread-safe buffer and queue ---
big_batch = []
insert_queue = Queue()

# --- Background MongoDB Insertion Thread ---
def mongo_worker():
    while True:
        docs = insert_queue.get()
        try:
            collection.insert_many(docs, ordered=False)
            now = datetime.now().strftime("%H:%M:%S.%f")[:-3]
            print(f"📦 {now} | Inserted {len(docs)} samples to MongoDB")
        except Exception as e:
            print("❌ MongoDB Insert Error:", e)
        insert_queue.task_done()

# Start the thread
threading.Thread(target=mongo_worker, daemon=True).start()

# --- MQTT Callbacks ---
def on_connect(client, userdata, flags, rc):
    print("✅ Connected to MQTT Broker:", rc)
    client.subscribe(MQTT_TOPIC)

def on_message(client, userdata, msg):
    global big_batch

    if len(msg.payload) != MQTT_PAYLOAD_SIZE:
        print("⚠️ Invalid payload size:", len(msg.payload))
        return

    try:
        for i in range(SAMPLES_PER_MQTT_MESSAGE):
            offset = i * STRUCT_SIZE
            sample = msg.payload[offset:offset + STRUCT_SIZE]
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
            big_batch.append(doc)

        if len(big_batch) >= BATCH_SIZE:
            insert_queue.put(big_batch.copy())
            big_batch.clear()

    except Exception as e:
        print("❌ MQTT Message Error:", e)

# --- MQTT Client Setup ---
client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

print("🔌 Connecting to MQTT broker...")
client.connect(MQTT_BROKER, MQTT_PORT, keepalive=60)
client.loop_start()

# Keep the main thread alive
while True:
    time.sleep(1)
