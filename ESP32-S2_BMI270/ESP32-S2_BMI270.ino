#include <Wire.h>
#include "SparkFun_BMI270_Arduino_Library.h"

BMI270 imu;
uint8_t i2cAddress = BMI2_I2C_PRIM_ADDR;

const unsigned int readIntervalMicros = 625; // 1600 Hz
//const unsigned int readIntervalMicros = 2500; // 400 Hz
unsigned long lastReadMicros = 0;

unsigned long sampleStartTime = 0;
unsigned int sampleCounter = 0;
const unsigned int sampleBatchSize = 1000;

void setup()
{
    Serial.begin(1000000);
    Serial.println("BMI270 - Sensor at 1200Hz, ESP32 reads at controlled rate");

    Wire.begin();
    Wire.setClock(1000000); // 1 MHz I2C

    while (imu.beginI2C(i2cAddress) != BMI2_OK)
    {
        Serial.println("Error: BMI270 not connected.");
        delay(1000);
    }

    Serial.println("BMI270 connected!");

    imu.setAccelODR(1200);
    imu.setGyroODR(1200);

    sampleStartTime = millis();
}

void loop()
{
    unsigned long nowMicros = micros();

    if (nowMicros - lastReadMicros >= readIntervalMicros)
    {
        lastReadMicros = nowMicros;

        imu.getSensorData(); // Fetch latest accel + gyro

        // Commented out: raw data print
        /*Serial.print(nowMicros);
        Serial.print(",");
        Serial.print(imu.data.accelX, 3);
        Serial.print(",");
        Serial.print(imu.data.accelY, 3);
        Serial.print(",");
        Serial.print(imu.data.accelZ, 3);
        Serial.print(",");
        Serial.print(imu.data.gyroX, 3);
        Serial.print(",");
        Serial.print(imu.data.gyroY, 3);
        Serial.print(",");
        Serial.println(imu.data.gyroZ, 3);*/

        sampleCounter++;

        if (sampleCounter >= sampleBatchSize)
        {
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
