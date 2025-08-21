/**
 * SensorTest.cpp
 * Standalone sensor test for LSM6DS3TR-C IMU and LIS3MDL Magnetometer.
 * Prints raw, offset, and filtered sensor values to Serial for debugging.
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_LIS3MDL.h>
#include <Adafruit_LSM6DS3TRC.h>

// Sensor objects
Adafruit_LIS3MDL magSensor;
Adafruit_LSM6DS3TRC imuSensor;

// Offsets
float accelOffsetX = 0.0f, accelOffsetY = 0.0f, accelOffsetZ = 0.0f;
float gyroOffsetX = 0.0f, gyroOffsetY = 0.0f, gyroOffsetZ = 0.0f;

// Filtered values (simple EMA)
float filteredAccelX = 0.0f, filteredAccelY = 0.0f, filteredAccelZ = 0.0f;
float filteredGyroX = 0.0f, filteredGyroY = 0.0f, filteredGyroZ = 0.0f;
float filteredMagX = 0.0f, filteredMagY = 0.0f, filteredMagZ = 0.0f;
const float FILTER_ALPHA = 0.5f;

// Calibration routine
void calibrateIMUOffsets(uint8_t numSamples) {
  float axSum = 0.0f, aySum = 0.0f, azSum = 0.0f;
  float gxSum = 0.0f, gySum = 0.0f, gzSum = 0.0f;
  for (uint8_t i = 0; i < numSamples; i++) {
    sensors_event_t accel, gyro, temp;
    imuSensor.getEvent(&accel, &gyro, &temp);
    axSum += accel.acceleration.y;
    aySum += accel.acceleration.x * -1;
    azSum += accel.acceleration.z * -1;
    gxSum += gyro.gyro.y;
    gySum += gyro.gyro.x * -1;
    gzSum += gyro.gyro.z * -1;
    delay(5);
  }
  accelOffsetX = axSum / numSamples;
  accelOffsetY = aySum / numSamples;
  accelOffsetZ = (azSum / numSamples) - 9.80665f;
  gyroOffsetX = gxSum / numSamples;
  gyroOffsetY = gySum / numSamples;
  gyroOffsetZ = gzSum / numSamples;
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Wire.begin();

  // Magnetometer setup
  if (magSensor.begin_I2C(LIS3MDL_I2CADDR_DEFAULT, &Wire)) {
    magSensor.setPerformanceMode(LIS3MDL_LOWPOWERMODE);
    magSensor.setOperationMode(LIS3MDL_CONTINUOUSMODE);
    magSensor.setDataRate(LIS3MDL_DATARATE_40_HZ);
    magSensor.setRange(LIS3MDL_RANGE_4_GAUSS);
    magSensor.setIntThreshold(500);
    magSensor.configInterrupt(false, false, false, true, false, false);
    Serial.println("LIS3MDL magnetometer initialized.");
  } else {
    Serial.println("LIS3MDL magnetometer NOT found!");
  }

  // IMU setup
  if (imuSensor.begin_I2C(LSM6DS_I2CADDR_DEFAULT, &Wire)) {
    imuSensor.setAccelRange(LSM6DS_ACCEL_RANGE_4_G);
    imuSensor.setGyroRange(LSM6DS_GYRO_RANGE_250_DPS);
    imuSensor.setAccelDataRate(LSM6DS_RATE_52_HZ);
    imuSensor.setGyroDataRate(LSM6DS_RATE_52_HZ);
    imuSensor.highPassFilter(false, LSM6DS_HPF_ODR_DIV_100);
    imuSensor.configInt1(true, false, false);
    imuSensor.configInt2(false, true, false);
    Serial.println("LSM6DS3TR-C IMU initialized.");
  } else {
    Serial.println("LSM6DS3TR-C IMU NOT found!");
  }

  // Calibrate offsets
  Serial.println("Calibrating IMU offsets...");
  calibrateIMUOffsets(20);
  Serial.println("Calibration complete.");
}

void loop() {
  sensors_event_t mag, accel, gyro, temp;
  magSensor.getEvent(&mag);
  imuSensor.getEvent(&accel, &gyro, &temp);

  // Raw readings (with axis swap/inversion as per your PCB orientation)
  float rawAccelX = accel.acceleration.y;
  float rawAccelY = accel.acceleration.x * -1;
  float rawAccelZ = accel.acceleration.z * -1;
  float rawGyroX = gyro.gyro.y;
  float rawGyroY = gyro.gyro.x * -1;
  float rawGyroZ = gyro.gyro.z * -1;
  float rawMagX = mag.magnetic.y;
  float rawMagY = mag.magnetic.x;
  float rawMagZ = mag.magnetic.z;

  // Offset corrected
  float offsetAccelX = rawAccelX - accelOffsetX;
  float offsetAccelY = rawAccelY - accelOffsetY;
  float offsetAccelZ = rawAccelZ - accelOffsetZ;
  float offsetGyroX = rawGyroX - gyroOffsetX;
  float offsetGyroY = rawGyroY - gyroOffsetY;
  float offsetGyroZ = rawGyroZ - gyroOffsetZ;

  // Filtered (EMA)
  filteredAccelX = FILTER_ALPHA * offsetAccelX + (1.0f - FILTER_ALPHA) * filteredAccelX;
  filteredAccelY = FILTER_ALPHA * offsetAccelY + (1.0f - FILTER_ALPHA) * filteredAccelY;
  filteredAccelZ = FILTER_ALPHA * offsetAccelZ + (1.0f - FILTER_ALPHA) * filteredAccelZ;
  filteredGyroX  = FILTER_ALPHA * offsetGyroX  + (1.0f - FILTER_ALPHA) * filteredGyroX;
  filteredGyroY  = FILTER_ALPHA * offsetGyroY  + (1.0f - FILTER_ALPHA) * filteredGyroY;
  filteredGyroZ  = FILTER_ALPHA * offsetGyroZ  + (1.0f - FILTER_ALPHA) * filteredGyroZ;
  filteredMagX   = FILTER_ALPHA * rawMagX      + (1.0f - FILTER_ALPHA) * filteredMagX;
  filteredMagY   = FILTER_ALPHA * rawMagY      + (1.0f - FILTER_ALPHA) * filteredMagY;
  filteredMagZ   = FILTER_ALPHA * rawMagZ      + (1.0f - FILTER_ALPHA) * filteredMagZ;

  // Print results
  Serial.println("---- Sensor Test ----");
  Serial.print("Raw Accel: X: "); Serial.print(rawAccelX, 2);
  Serial.print(" Y: "); Serial.print(rawAccelY, 2);
  Serial.print(" Z: "); Serial.println(rawAccelZ, 2);

  Serial.print("Offset Accel: X: "); Serial.print(offsetAccelX, 2);
  Serial.print(" Y: "); Serial.print(offsetAccelY, 2);
  Serial.print(" Z: "); Serial.println(offsetAccelZ, 2);

  Serial.print("Filtered Accel: X: "); Serial.print(filteredAccelX, 2);
  Serial.print(" Y: "); Serial.print(filteredAccelY, 2);
  Serial.print(" Z: "); Serial.println(filteredAccelZ, 2);

  Serial.print("Raw Gyro: X: "); Serial.print(rawGyroX, 2);
  Serial.print(" Y: "); Serial.print(rawGyroY, 2);
  Serial.print(" Z: "); Serial.println(rawGyroZ, 2);

  Serial.print("Offset Gyro: X: "); Serial.print(offsetGyroX, 2);
  Serial.print(" Y: "); Serial.print(offsetGyroY, 2);
  Serial.print(" Z: "); Serial.println(offsetGyroZ, 2);

  Serial.print("Filtered Gyro: X: "); Serial.print(filteredGyroX, 2);
  Serial.print(" Y: "); Serial.print(filteredGyroY, 2);
  Serial.print(" Z: "); Serial.println(filteredGyroZ, 2);

  Serial.print("Raw Mag: X: "); Serial.print(rawMagX, 2);
  Serial.print(" Y: "); Serial.print(rawMagY, 2);
  Serial.print(" Z: "); Serial.println(rawMagZ, 2);

  Serial.print("Filtered Mag: X: "); Serial.print(filteredMagX, 2);
  Serial.print(" Y: "); Serial.print(filteredMagY, 2);
  Serial.print(" Z: "); Serial.println(filteredMagZ, 2);

  Serial.println("---------------------");
  delay(100);
}
