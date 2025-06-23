# 📡 ESP32-S2 High-Frequency Sensor Streaming via MQTT

This project streams high-frequency sensor data from an ESP32-S2 microcontroller to an MQTT broker at ~1 kHz. The data includes IMU (BMI270) and environmental (BME688) readings and is transmitted in binary format using MQTT. The receiver logs the data locally in **NDJSON** format and optionally uploads to MongoDB.

---

## 🔧 Hardware & Libraries Used

- **ESP32-S2** (SparkFun Thing Plus)
- **BMI270 IMU** via [SparkFun_BMI270_Arduino_Library](https://github.com/sparkfun/SparkFun_BMI270_Arduino_Library)
- **BME688** Environmental Sensor via lightweight library by Saurav Sajeev
- **MQTT Library:** PubSubClient
- **WiFi:** 2.4 GHz network (tested with `IoT_EAA8` SSID)

---

## 📦 Features

- 📈 High-frequency BMI270 readings at ~1200 Hz
- 🌡️ BME688 temperature, pressure, and humidity readings at 1 Hz
- 📤 MQTT publish rate ~1 kHz using binary-packed struct arrays
- 📝 Local logging to **NDJSON** format for fast disk writing
- 🗄️ Optional MongoDB uploads (hybrid model)
- 💡 LED indicators on ESP32:
  - ✅ Green ON → Connected to MQTT and publishing
  - ❌ Red ON → Not connected to MQTT

---

## 📁 Project Structure

📂 ESP32-MQTT-Logger
├── firmware/ # Arduino code for ESP32-S2
│ └── main.ino
├── logger/
│ ├── mqtt_logger.py # MQTT subscriber (logs to NDJSON + MongoDB)
│ └── analyze_odr.py # ODR analysis of timestamp gaps
└── data/
└── data.json # NDJSON log output


---

## ⚙️ Firmware Details

### Struct Definition

```cpp
struct SensorDataPacket {
  uint32_t timestamp;
  float tempC;
  float pressure;
  float humidity;
  float ax, ay, az;
  float gx, gy, gz;
};


Sample Size: 40 bytes

Batch Size: 100 samples per MQTT message

MQTT Payload Size: 4000 bytes

MQTT Topic: esp32/sensor/stream

#define MQTT_MAX_PACKET_SIZE 4096

🧠 Python Logger Summary
mqtt_logger.py
Subscribes to esp32/sensor/stream

Unpacks binary packets into structured JSON

Appends data to data.json in NDJSON format

Uploads new data to MongoDB every 10 seconds (optional)

analyze_odr.py
Loads data.json

Calculates timestamp gaps

Prints mean gap and estimated ODR

Plots histogram of timestamp intervals