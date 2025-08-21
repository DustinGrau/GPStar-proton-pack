/**
 * SensorTest.cpp
 * Standalone sensor test for LSM6DS3TR-C IMU and LIS3MDL Magnetometer.
 * Prints raw, offset, and filtered sensor values to Serial for debugging.
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_LIS3MDL.h>
#include <Adafruit_LSM6DS3TRC.h>

#define IMU_SCL 47
#define IMU_SDA 48

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
  // Reduce CPU frequency to 160 MHz to save ~33% power compared to 240 MHz.
  // Alternatively set CPU to 80 MHz to save ~50% power compared to 240 MHz.
  // Do not set below 80 MHz as it will affect WiFi and other peripherals.
  setCpuFrequencyMhz(80);

  // This is required in order to make sure the board boots successfully.
  Serial.begin(115200);

  // Serial0 (UART0) is enabled by default; end() sets GPIO43 & GPIO44 to GPIO.
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

String formatSignedFloat(float value) {
  char buf[16];
  int whole = abs((int)value);
  // Determine padding: if whole < 10, pad 2 spaces; < 100, pad 1 space; else no pad
  const char* pad = (whole < 10) ? "  " : (whole < 100) ? " " : "";
  sprintf(buf, "%c%s%.2f", (value >= 0 ? '+' : '-'), pad, abs(value));
  return String(buf);
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
  Serial.print("Direct Accel:\tX: "); Serial.print(formatSignedFloat(rawAccelX));
  Serial.print("\tY: "); Serial.print(formatSignedFloat(rawAccelY));
  Serial.print("\tZ: "); Serial.println(formatSignedFloat(rawAccelZ));

  Serial.print("Offset Accel:\tX: "); Serial.print(formatSignedFloat(offsetAccelX));
  Serial.print("\tY: "); Serial.print(formatSignedFloat(offsetAccelY));
  Serial.print("\tZ: "); Serial.println(formatSignedFloat(offsetAccelZ));

  Serial.print("Filter Accel:\tX: "); Serial.print(formatSignedFloat(filteredAccelX));
  Serial.print("\tY: "); Serial.print(formatSignedFloat(filteredAccelY));
  Serial.print("\tZ: "); Serial.println(formatSignedFloat(filteredAccelZ));

  Serial.print(" Direct Gyro:\tX: "); Serial.print(formatSignedFloat(rawGyroX));
  Serial.print("\tY: "); Serial.print(formatSignedFloat(rawGyroY));
  Serial.print("\tZ: "); Serial.println(formatSignedFloat(rawGyroZ));

  Serial.print(" Offset Gyro:\tX: "); Serial.print(formatSignedFloat(offsetGyroX));
  Serial.print("\tY: "); Serial.print(formatSignedFloat(offsetGyroY));
  Serial.print("\tZ: "); Serial.println(formatSignedFloat(offsetGyroZ));

  Serial.print(" Filter Gyro:\tX: "); Serial.print(formatSignedFloat(filteredGyroX));
  Serial.print("\tY: "); Serial.print(formatSignedFloat(filteredGyroY));
  Serial.print("\tZ: "); Serial.println(formatSignedFloat(filteredGyroZ));

  Serial.print("  Direct Mag:\tX: "); Serial.print(formatSignedFloat(rawMagX));
  Serial.print("\tY: "); Serial.print(formatSignedFloat(rawMagY));
  Serial.print("\tZ: "); Serial.println(formatSignedFloat(rawMagZ));

  Serial.print("  Filter Mag:\tX: "); Serial.print(formatSignedFloat(filteredMagX));
  Serial.print("\tY: "); Serial.print(formatSignedFloat(filteredMagY));
  Serial.print("\tZ: "); Serial.println(formatSignedFloat(filteredMagZ));

  Serial.println("---------------------");
  delay(200);
}
