#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "SparkFun_BMI270_Arduino_Library.h"
#include <BME688.h>

// --- WiFi ---
const char* ssid = "IoT_EAA8";
const char* password = "26815718";

// --- MQTT ---
const char* mqtt_server = "broker.hivemq.com"; // or test.mosquitto.org
const int mqtt_port = 1883;
const char* mqtt_topic = "esp32/sensor/stream";

WiFiClient espClient;
PubSubClient client(espClient);

// --- LED Pins ---
const int GREEN_LED_PIN = 13;
const int RED_LED_PIN = 12;

// --- BMI270 ---
BMI270 imu;
uint8_t i2cAddress = BMI2_I2C_PRIM_ADDR;

// --- BME688 ---
BME688 bme;

// --- Timing (ODR) ---
const unsigned int readIntervalMicros = 833; // ~1200 Hz
unsigned long lastReadMicros = 0;

// --- ODR measurement ---
unsigned long sampleStartTime = 0;
unsigned int sampleCounter = 0;
const unsigned int sampleBatchSize = 1000;

// --- Cached BME688 data ---
float tempC = NAN;
float pressure = NAN;
float humidity = NAN;
unsigned long lastBMEread = 0;
const unsigned long bmeInterval = 1000; // 1 second

// --- Packed struct ---
struct SensorDataPacket {
  uint32_t timestamp;
  float tempC;
  float pressure;
  float humidity;
  float ax, ay, az;
  float gx, gy, gz;
};

void connectToWiFi() {
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\nWiFi connected.");
}

void connectToMQTT() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    String clientId = "ESP32Client-" + String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println(" connected!");
    } else {
      Serial.print(" failed. State: ");
      Serial.println(client.state());
      delay(1000);
    }
  }
}

void updateODR() {
  sampleCounter++;
  if (sampleCounter >= sampleBatchSize) {
    unsigned long now = millis();
    float durationSec = (now - sampleStartTime) / 1000.0;
    float effectiveODR = sampleCounter / durationSec;

    Serial.print("Effective ODR: ");
    Serial.print(effectiveODR, 2);
    Serial.println(" Hz");

    sampleCounter = 0;
    sampleStartTime = now;
  }
}

void setup() {
  Serial.begin(1000000);
  Wire.begin();
  Wire.setClock(1000000);

  connectToWiFi();
  client.setServer(mqtt_server, mqtt_port);
  connectToMQTT();

  // Init BMI270
  while (imu.beginI2C(i2cAddress) != BMI2_OK) {
    Serial.println("Error: BMI270 not connected.");
    delay(1000);
  }
  imu.setAccelODR(1200);
  imu.setGyroODR(1200);
  Serial.println("BMI270 connected.");

  // Init BME688
  if (!bme.begin()) {
    Serial.println("BME688 init failed!");
    while (1);
  }
  Serial.println("BME688 connected.");

  sampleStartTime = millis();
}

#define BATCH_SIZE 100
SensorDataPacket packetBuffer[BATCH_SIZE];
uint8_t batchIndex = 0;

void loop() {
  // MQTT maintenance
  static unsigned long lastMQTTLoop = 0;
  if (millis() - lastMQTTLoop >= 100) {
    if (!client.connected()) {
      Serial.println("MQTT disconnected. Reconnecting...");
      connectToMQTT();
    }
    client.loop();
    lastMQTTLoop = millis();
  }

  unsigned long nowMicros = micros();
  if (nowMicros - lastReadMicros >= readIntervalMicros) {
    lastReadMicros = nowMicros;
    imu.getSensorData();

    if (millis() - lastBMEread >= bmeInterval) {
      lastBMEread = millis();
      tempC = bme.readTemperature();
      pressure = bme.readPressure();
      humidity = bme.readHumidity();
    }

    SensorDataPacket packet;
    packet.timestamp = millis();
    packet.tempC = tempC;
    packet.pressure = pressure;
    packet.humidity = humidity;
    packet.ax = imu.data.accelX;
    packet.ay = imu.data.accelY;
    packet.az = imu.data.accelZ;
    packet.gx = imu.data.gyroX;
    packet.gy = imu.data.gyroY;
    packet.gz = imu.data.gyroZ;

    // Add to batch
    packetBuffer[batchIndex++] = packet;

    // MQTT publish status
    /*bool sent = client.publish(mqtt_topic, (uint8_t*)&packet, sizeof(packet), false);
    if (!sent) {
      Serial.println("❌ MQTT publish failed.");
    }*/

    // Once batch is full, send
    if (batchIndex >= BATCH_SIZE) {
      bool sent = client.publish(mqtt_topic, (uint8_t*)packetBuffer, sizeof(packetBuffer), false);
      if (!sent) {
        Serial.println("❌ MQTT batch publish failed.");
      } /*else {
      Serial.println("✅ MQTT batch sent");
      }*/
      batchIndex = 0;
    }
    /*
    Serial.print("Time: "); Serial.print(packet.timestamp); Serial.print(" ms | ");
    Serial.print("T:"); Serial.print(packet.tempC, 1); Serial.print("C ");
    Serial.print("P:"); Serial.print(packet.pressure, 1); Serial.print("Pa ");
    Serial.print("H:"); Serial.print(packet.humidity, 1); Serial.print("% | ");
    Serial.print("AX:"); Serial.print(packet.ax, 3); Serial.print(" ");
    Serial.print("AY:"); Serial.print(packet.ay, 3); Serial.print(" ");
    Serial.print("AZ:"); Serial.print(packet.az, 3); Serial.print(" | ");
    Serial.print("GX:"); Serial.print(packet.gx, 3); Serial.print(" ");
    Serial.print("GY:"); Serial.print(packet.gy, 3); Serial.print(" ");
    Serial.print("GZ:"); Serial.println(packet.gz, 3);
    */

  updateODR();

  }
}

