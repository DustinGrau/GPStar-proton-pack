/**
 * SensorTest.cpp
 * Standalone sensor test for LSM6DS3TR-C IMU and LIS3MDL Magnetometer.
 * Uses struct-based storage for sensor values, offsets, and filtering.
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_LIS3MDL.h>
#include <Adafruit_LSM6DS3TRC.h>

#include "Motion.h"

void setup() {
  setCpuFrequencyMhz(80);
  Serial.begin(115200);
  Serial0.end();
  Wire1.begin(IMU_SDA, IMU_SCL, 400000UL);

  // Magnetometer setup
  if (magSensor.begin_I2C(LIS3MDL_I2CADDR_DEFAULT, &Wire1)) {
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
  if (imuSensor.begin_I2C(LSM6DS_I2CADDR_DEFAULT, &Wire1)) {
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

  // Update raw readings in struct (account for PCB orientation)
  motionData.accelX = accel.acceleration.y;
  motionData.accelY = accel.acceleration.x * -1;
  motionData.accelZ = accel.acceleration.z * -1;
  motionData.gyroX = gyro.gyro.y;
  motionData.gyroY = gyro.gyro.x * -1;
  motionData.gyroZ = gyro.gyro.z * -1;
  motionData.magX = mag.magnetic.y;
  motionData.magY = mag.magnetic.x;
  motionData.magZ = mag.magnetic.z;
  motionData.heading = calculateHeading(motionData.magX, motionData.magY);

  // Offset corrected
  MotionData offsetMotionData;
  offsetMotionData.accelX = motionData.accelX - motionOffsets.accelX;
  offsetMotionData.accelY = motionData.accelY - motionOffsets.accelY;
  offsetMotionData.accelZ = motionData.accelZ - motionOffsets.accelZ;
  offsetMotionData.gyroX = motionData.gyroX - motionOffsets.gyroX;
  offsetMotionData.gyroY = motionData.gyroY - motionOffsets.gyroY;
  offsetMotionData.gyroZ = motionData.gyroZ - motionOffsets.gyroZ;
  offsetMotionData.magX = motionData.magX;
  offsetMotionData.magY = motionData.magY;
  offsetMotionData.magZ = motionData.magZ;
  offsetMotionData.heading = motionData.heading;

  // Filtered (EMA)
  filteredMotionData.accelX = FILTER_ALPHA * offsetMotionData.accelX + (1.0f - FILTER_ALPHA) * filteredMotionData.accelX;
  filteredMotionData.accelY = FILTER_ALPHA * offsetMotionData.accelY + (1.0f - FILTER_ALPHA) * filteredMotionData.accelY;
  filteredMotionData.accelZ = FILTER_ALPHA * offsetMotionData.accelZ + (1.0f - FILTER_ALPHA) * filteredMotionData.accelZ;
  filteredMotionData.gyroX  = FILTER_ALPHA * offsetMotionData.gyroX  + (1.0f - FILTER_ALPHA) * filteredMotionData.gyroX;
  filteredMotionData.gyroY  = FILTER_ALPHA * offsetMotionData.gyroY  + (1.0f - FILTER_ALPHA) * filteredMotionData.gyroY;
  filteredMotionData.gyroZ  = FILTER_ALPHA * offsetMotionData.gyroZ  + (1.0f - FILTER_ALPHA) * filteredMotionData.gyroZ;
  filteredMotionData.magX   = FILTER_ALPHA * offsetMotionData.magX   + (1.0f - FILTER_ALPHA) * filteredMotionData.magX;
  filteredMotionData.magY   = FILTER_ALPHA * offsetMotionData.magY   + (1.0f - FILTER_ALPHA) * filteredMotionData.magY;
  filteredMotionData.magZ   = FILTER_ALPHA * offsetMotionData.magZ   + (1.0f - FILTER_ALPHA) * filteredMotionData.magZ;
  filteredMotionData.heading = calculateHeading(filteredMotionData.magX, filteredMotionData.magY);

  // Print results
  Serial.println("---- Sensor Test ----");
  Serial.print("Direct Accel:\tX: "); Serial.print(formatSignedFloat(motionData.accelX));
  Serial.print("\tY: "); Serial.print(formatSignedFloat(motionData.accelY));
  Serial.print("\tZ: "); Serial.println(formatSignedFloat(motionData.accelZ));

  Serial.print("Offset Accel:\tX: "); Serial.print(formatSignedFloat(offsetMotionData.accelX));
  Serial.print("\tY: "); Serial.print(formatSignedFloat(offsetMotionData.accelY));
  Serial.print("\tZ: "); Serial.println(formatSignedFloat(offsetMotionData.accelZ));

  Serial.print("Filter Accel:\tX: "); Serial.print(formatSignedFloat(filteredMotionData.accelX));
  Serial.print("\tY: "); Serial.print(formatSignedFloat(filteredMotionData.accelY));
  Serial.print("\tZ: "); Serial.println(formatSignedFloat(filteredMotionData.accelZ));

  Serial.print(" Direct Gyro:\tX: "); Serial.print(formatSignedFloat(motionData.gyroX));
  Serial.print("\tY: "); Serial.print(formatSignedFloat(motionData.gyroY));
  Serial.print("\tZ: "); Serial.println(formatSignedFloat(motionData.gyroZ));

  Serial.print(" Offset Gyro:\tX: "); Serial.print(formatSignedFloat(offsetMotionData.gyroX));
  Serial.print("\tY: "); Serial.print(formatSignedFloat(offsetMotionData.gyroY));
  Serial.print("\tZ: "); Serial.println(formatSignedFloat(offsetMotionData.gyroZ));

  Serial.print(" Filter Gyro:\tX: "); Serial.print(formatSignedFloat(filteredMotionData.gyroX));
  Serial.print("\tY: "); Serial.print(formatSignedFloat(filteredMotionData.gyroY));
  Serial.print("\tZ: "); Serial.println(formatSignedFloat(filteredMotionData.gyroZ));

  Serial.print("  Direct Mag:\tX: "); Serial.print(formatSignedFloat(motionData.magX));
  Serial.print("\tY: "); Serial.print(formatSignedFloat(motionData.magY));
  Serial.print("\tZ: "); Serial.println(formatSignedFloat(motionData.magZ));

  Serial.print("  Filter Mag:\tX: "); Serial.print(formatSignedFloat(filteredMotionData.magX));
  Serial.print("\tY: "); Serial.print(formatSignedFloat(filteredMotionData.magY));
  Serial.print("\tZ: "); Serial.println(formatSignedFloat(filteredMotionData.magZ));

  Serial.print("\tHeading: "); Serial.println(filteredMotionData.heading, 2);

  Serial.println("---------------------");
  delay(100);
}
