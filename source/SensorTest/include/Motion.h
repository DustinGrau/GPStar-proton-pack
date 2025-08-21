
#define IMU_SCL 47
#define IMU_SDA 48

// Struct for motion sensor readings
struct MotionData {
  float magX = 0.0f;
  float magY = 0.0f;
  float magZ = 0.0f;
  float heading = 0.0f;
  float accelX = 0.0f;
  float accelY = 0.0f;
  float accelZ = 0.0f;
  float gyroX = 0.0f;
  float gyroY = 0.0f;
  float gyroZ = 0.0f;
};

// Struct for calibration offsets
struct MotionOffsets {
  float accelX = 0.0f;
  float accelY = 0.0f;
  float accelZ = 0.0f;
  float gyroX = 0.0f;
  float gyroY = 0.0f;
  float gyroZ = 0.0f;
};

const float FILTER_ALPHA = 0.5f;

// Sensor objects
Adafruit_LIS3MDL magSensor;
Adafruit_LSM6DS3TRC imuSensor;

// Global structs for sensor data
MotionData motionData;
MotionData filteredMotionData;
MotionOffsets motionOffsets;

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
    delay(20);
  }
  motionOffsets.accelX = axSum / numSamples;
  motionOffsets.accelY = aySum / numSamples;
  motionOffsets.accelZ = (azSum / numSamples) - 9.80665f;
  motionOffsets.gyroX = gxSum / numSamples;
  motionOffsets.gyroY = gySum / numSamples;
  motionOffsets.gyroZ = gzSum / numSamples;
}

// Format float with sign and padding
String formatSignedFloat(float value) {
  char buf[16];
  int whole = abs((int)value);
  const char* pad = (whole < 10) ? "  " : (whole < 100) ? " " : "";
  sprintf(buf, "%c%s%.2f", (value >= 0 ? '+' : '-'), pad, abs(value));
  return String(buf);
}

// Calculate heading from magnetometer X/Y
float calculateHeading(float magX, float magY) {
  float headingRad = atan2(-magY, -magX);
  float headingDeg = headingRad * (180.0f / PI);
  while (headingDeg < 0.0f) headingDeg += 360.0f;
  while (headingDeg >= 360.0f) headingDeg -= 360.0f;
  return headingDeg;
}
