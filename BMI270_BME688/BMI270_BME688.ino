#include <Wire.h>
#include "SparkFun_BMI270_Arduino_Library.h"
#include <BME688.h>  // Saurav Sajeev's lightweight BME688 lib

// --- BMI270 ---
BMI270 imu;
uint8_t i2cAddress = BMI2_I2C_PRIM_ADDR;

// --- BME688 ---
BME688 bme;

// --- Timing ---
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

void setup() {
  Serial.begin(1000000);
  Wire.begin();
  Wire.setClock(1000000); // Fast I2C

  while (imu.beginI2C(i2cAddress) != BMI2_OK) {
    Serial.println("Error: BMI270 not connected.");
    delay(1000);
  }
  imu.setAccelODR(1200);
  imu.setGyroODR(1200);
  Serial.println("BMI270 connected.");

  if (!bme.begin()) {
    Serial.println("BME688 init failed!");
    while (1);
  }
  Serial.println("BME688 connected.");

  sampleStartTime = millis();
}

void loop() {
  unsigned long nowMicros = micros();
  if (nowMicros - lastReadMicros >= readIntervalMicros)
  {
    lastReadMicros = nowMicros;

    imu.getSensorData();

    // Only update BME688 data every 1 second
    if (millis() - lastBMEread >= bmeInterval) {
      lastBMEread = millis();
      tempC = bme.readTemperature();
      pressure = bme.readPressure();
      humidity = bme.readHumidity();
    }

    unsigned long timestamp = millis();
    
    // Output Debug
    /*
    Serial.print("Time: "); Serial.print(timestamp); Serial.print(" ms | ");
    Serial.print("T:"); Serial.print(tempC, 1); Serial.print("C ");
    Serial.print("P:"); Serial.print(pressure, 1); Serial.print("Pa ");
    Serial.print("H:"); Serial.print(humidity, 1); Serial.print("% | ");

    Serial.print("AX:"); Serial.print(imu.data.accelX, 3); Serial.print(" ");
    Serial.print("AY:"); Serial.print(imu.data.accelY, 3); Serial.print(" ");
    Serial.print("AZ:"); Serial.print(imu.data.accelZ, 3); Serial.print(" | ");

    Serial.print("GX:"); Serial.print(imu.data.gyroX, 3); Serial.print(" ");
    Serial.print("GY:"); Serial.print(imu.data.gyroY, 3); Serial.print(" ");
    Serial.print("GZ:"); Serial.println(imu.data.gyroZ, 3);*/

    // Measure ODR
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
}