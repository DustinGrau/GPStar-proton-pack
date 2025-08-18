/**
 *   GPStar Neutrona Wand - Ghostbusters Proton Pack & Neutrona Wand.
 *   Copyright (C) 2023-2025 Michael Rajotte <michael.rajotte@gpstartechnologies.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, see <https://www.gnu.org/licenses/>.
 *
 */

#pragma once

/**
 * NOTICE! Remember that the PCB for the Neutrona Wand is mounted upside down!
 * For proper orientation hold the device with the components facing downward.
 */

#include <Adafruit_LIS3MDL.h>
#include <Adafruit_LSM6DS3TRC.h>
#include <MadgwickAHRS.h>

// Forward function declarations.
void resetAllMotionData();
void sendTelemetryData(); // From Webhandler.h

/**
 * Magnetometer and IMU
 * Defines all device objects and variables.
 */
Adafruit_LIS3MDL magSensor;
Adafruit_LSM6DS3TRC imuSensor;
bool b_mag_found = false;
bool b_imu_found = false;
millisDelay ms_sensor_read_delay, ms_sensor_report_delay;
const uint16_t i_sensor_read_delay = 100; // Delay between sensor reads in milliseconds
const uint16_t i_sensor_report_delay = 200; // Delay between telemetry reporting in milliseconds.
const float HEADING_OFFSET_DEG = 0.0f; // Correct magnetometer orientation on the controller PCB.

// Create a global filter object
Madgwick filter;

/**
 * Constant: FILTER_ALPHA
 * Purpose: Controls the smoothing factor for exponential moving average filtering (0 < FILTER_ALPHA <= 1).
 *          Increasing this value makes it more responsive to changes, decreasing smooths out fluctuations.
 */
const float FILTER_ALPHA = 0.2f;

/**
 * Struct: MotionData
 * Purpose: Holds all motion sensor readings for magnetometer, accelerometer, gyroscope, and calculated heading.
 * Attributes:
 *   - magX, magY, magZ: Magnetometer readings (uTesla)
 *   - accelX, accelY, accelZ: Accelerometer readings (m/s^2)
 *   - gyroX, gyroY, gyroZ: Gyroscope readings (rads/s)
 *   - heading: Compass heading in degrees (0-360°), derived from magX and magY
 */
struct MotionData {
  float magX;
  float magY;
  float magZ;
  float heading;
  float accelX;
  float accelY;
  float accelZ;
  float gyroX;
  float gyroY;
  float gyroZ;
};

// Global objects to hold the latest raw or averaged sensor readings.
MotionData motionData, filteredMotionData;

/**
 * Struct: SpatialData
 * Purpose: Holds fused sensor readings for magnetometer, accelerometer, gyroscope, and calculated heading.
 * Attributes:
 *   - roll, pitch, yaw: Euler angles in degrees representing the orientation of the device.
 */
struct SpatialData {
  float roll;
  float pitch;
  float yaw;
};

// Global object to hold the fused sensor readings.
SpatialData spatialData;

/**
 * Function: resetMotionData
 * Purpose: Resets all fields of a MotionData object to zero.
 * Inputs:
 *   - MotionData &data: Reference to the MotionData object to reset.
 * Outputs: None (modifies the object in place)
 */
void resetMotionData(MotionData &data) {
  data.magX = 0.0f;
  data.magY = 0.0f;
  data.magZ = 0.0f;
  data.accelX = 0.0f;
  data.accelY = 0.0f;
  data.accelZ = 0.0f;
  data.gyroX = 0.0f;
  data.gyroY = 0.0f;
  data.gyroZ = 0.0f;
  data.heading = 0.0f;
}

/**
 * Function: resetSpatialData
 * Purpose: Resets all fields of a SpatialData object to zero.
 * Inputs:
 *   - SpatialData &data: Reference to the SpatialData object to reset.
 * Outputs: None (modifies the object in place)
 */
void resetSpatialData(SpatialData &data) {
  data.pitch = 0.0f;
  data.yaw = 0.0f;
  data.roll = 0.0f;
}

/**
 * Struct: MotionOffsets
 * Purpose: Holds baseline offsets for accelerometer and gyroscope to correct sensor drift.
 * Members:
 *   - accelX, accelY, accelZ: Accelerometer offsets (m/s^2)
 *   - gyroX, gyroY, gyroZ: Gyroscope offsets (rads/s)
 */
struct MotionOffsets {
  float accelX;
  float accelY;
  float accelZ;
  float gyroX;
  float gyroY;
  float gyroZ;
};

// Global object to hold the latest IMU offsets.
MotionOffsets motionOffsets = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};

/**
 * Function: initializeMotionDevices
 * Purpose: Initializes the I2C bus and configures the Magnetometer and IMU devices.
 */
void initializeMotionDevices() {
#ifdef ENABLE_MOTION_SENSORS
  Wire1.begin(IMU_SDA, IMU_SCL, 400000UL);

  // Initialize the LIS3MDL magnetometer.
  if(magSensor.begin_I2C(LIS3MDL_I2CADDR_DEFAULT, &Wire1)) {
    b_mag_found = true; // Indicate that the magnetometer was found.
    debugln(F("LIS3MDL found at default address"));
    magSensor.setPerformanceMode(LIS3MDL_MEDIUMMODE); // Set performance mode to medium (balanced power/accuracy)
    magSensor.setOperationMode(LIS3MDL_CONTINUOUSMODE); // Set operation mode to continuous measurements
    magSensor.setDataRate(LIS3MDL_DATARATE_155_HZ); // Set data rate to 155Hz (or LIS3MDL_DATARATE_300_HZ)
    magSensor.setRange(LIS3MDL_RANGE_8_GAUSS); // Set range to 8 Gauss (mid sensitivity, mid max field)
    magSensor.setIntThreshold(500); // Set interrupt threshold to 500
    magSensor.configInterrupt(false, false, true, true, false, true); // Configure interrupts
  }

  // Initialize the LSM6DS3TR-C IMU.
  if(imuSensor.begin_I2C(LSM6DS_I2CADDR_DEFAULT, &Wire1)) {
    b_imu_found = true; // Indicate that the IMU was found.
    debugln(F("LSM6DS3TR-C found at default address"));
    imuSensor.setAccelRange(LSM6DS_ACCEL_RANGE_4_G); // Set accelerometer range to 4G (high sensitivity, low max acceleration)
    imuSensor.setGyroRange(LSM6DS_GYRO_RANGE_250_DPS); // Set gyroscope range to 250DPS (high sensitivity, low max rotation)
    imuSensor.setAccelDataRate(LSM6DS_RATE_208_HZ); // Set accelerometer data rate to 208Hz
    imuSensor.setGyroDataRate(LSM6DS_RATE_208_HZ); // Set gyroscope data rate to 208Hz
    imuSensor.configInt1(false, false, true); // Enable accelerometer data ready interrupt
    imuSensor.configInt2(false, true, false); // Enable gyroscope data ready interrupt
  }

  // Set the sample frequency for the Madgwick filter (converting our sensor delay interval from milliseconds to Hz).
  float f_sample_freq = (1000.0f / i_sensor_read_delay);
  filter.begin(f_sample_freq);
#endif

  // Reset all motion data.
  resetAllMotionData();
}

/**
 * Function: calibrateIMUOffsets
 * Purpose: Samples the IMU while stationary to determine and set baseline offsets for accelerometer and gyroscope.
 * Inputs:
 *   - uint8_t numSamples: Number of samples to average for calibration.
 * Outputs: None (updates global motionOffsets struct)
 * Side Effects: Updates motionOffsets for more accurate stationary readings.
 * Note: Samples are collected as fast as possible, no delay.
 */
void calibrateIMUOffsets(uint8_t numSamples = 20) {
#ifdef ENABLE_MOTION_SENSORS
  float axSum = 0.0f, aySum = 0.0f, azSum = 0.0f;
  float gxSum = 0.0f, gySum = 0.0f, gzSum = 0.0f;

  for (uint8_t i = 0; i < numSamples; i++) {
    sensors_event_t accel, gyro, temp;
    imuSensor.getEvent(&accel, &gyro, &temp);

    axSum += accel.acceleration.y * -1; // Invert Y for the component's installation.
    aySum += accel.acceleration.x; // X reads as-is because it points East already.
    azSum += accel.acceleration.z * -1; // Invert Z because we install upside down.
    gxSum += gyro.gyro.y * -1; // Invert Y for the component's installation.
    gySum += gyro.gyro.x; // X reads as-is because it points East already.
    gzSum += gyro.gyro.z * -1; // Invert Z because we install upside down.
  }

  // Calculate average offsets
  motionOffsets.accelX = axSum / numSamples;
  motionOffsets.accelY = aySum / numSamples;
  motionOffsets.accelZ = (azSum / numSamples) - 9.80665f; // Subtract gravity for Z axis (9.81 m/s^2)
  motionOffsets.gyroX = gxSum / numSamples;
  motionOffsets.gyroY = gySum / numSamples;
  motionOffsets.gyroZ = gzSum / numSamples;

  // Print the filtered sensor data to the debug console.
  #if defined(DEBUG_TELEMETRY_DATA)
    debug("\t\tOffset Accel X: ");
    debug(motionOffsets.accelX);
    debug(" \tY: ");
    debug(motionOffsets.accelY);
    debug(" \tZ: ");
    debug(motionOffsets.accelZ);
    debugln(" m/s^2 ");
    debug("\t\tOffset Gyro  X: ");
    debug(motionOffsets.gyroX);
    debug(" \tY: ");
    debug(motionOffsets.gyroY);
    debug(" \tZ: ");
    debug(motionOffsets.gyroZ);
    debugln(" rads/s ");
    debugln();
  #endif
#endif
}

/**
 * Function: resetAllMotionData
 * Purpose: Resets both global motionData and filteredMotionData objects to zero.
 */
void resetAllMotionData() {
  resetMotionData(motionData);
  resetMotionData(filteredMotionData);
  resetSpatialData(spatialData);
  calibrateIMUOffsets();
}

/**
 * Function: calculateHeading
 * Purpose: Computes the compass heading (in degrees) from magnetometer X and Y values, applying a device-specific offset and optional inversion for mounting.
 * Inputs:
 *   - float magX: Magnetometer X-axis reading
 *   - float magY: Magnetometer Y-axis reading
 * Outputs:
 *   - float: Compass heading in degrees (0-360°)
 */
float calculateHeading(float magX, float magY) {
  float headingRad = atan2(magY, magX); // Get heading in radians from atan2 of Y and X.
  float headingDeg = headingRad * (180.0f / PI); // Convert radians to degrees (180/pi).

  // Apply offset for physical chip orientation
  //headingDeg += HEADING_OFFSET_DEG;

  // Normalize to 0-360°
  while (headingDeg < 0.0f) {
    headingDeg += 360.0f;
  }
  while (headingDeg >= 360.0f) {
    headingDeg -= 360.0f;
  }

  return headingDeg;
}

/**
 * Function: updateFilteredMotionData
 * Purpose: Applies exponential moving average filtering to raw motionData and updates filteredMotionData.
 * Inputs: None (uses global motionData and filteredMotionData)
 * Outputs: None (updates filteredMotionData)
 */
void updateFilteredMotionData() {
  filteredMotionData.magX    = FILTER_ALPHA * motionData.magX    + (1.0f - FILTER_ALPHA) * filteredMotionData.magX;
  filteredMotionData.magY    = FILTER_ALPHA * motionData.magY    + (1.0f - FILTER_ALPHA) * filteredMotionData.magY;
  filteredMotionData.magZ    = FILTER_ALPHA * motionData.magZ    + (1.0f - FILTER_ALPHA) * filteredMotionData.magZ;
  filteredMotionData.accelX  = FILTER_ALPHA * motionData.accelX  + (1.0f - FILTER_ALPHA) * filteredMotionData.accelX;
  filteredMotionData.accelY  = FILTER_ALPHA * motionData.accelY  + (1.0f - FILTER_ALPHA) * filteredMotionData.accelY;
  filteredMotionData.accelZ  = FILTER_ALPHA * motionData.accelZ  + (1.0f - FILTER_ALPHA) * filteredMotionData.accelZ;
  filteredMotionData.gyroX   = FILTER_ALPHA * motionData.gyroX   + (1.0f - FILTER_ALPHA) * filteredMotionData.gyroX;
  filteredMotionData.gyroY   = FILTER_ALPHA * motionData.gyroY   + (1.0f - FILTER_ALPHA) * filteredMotionData.gyroY;
  filteredMotionData.gyroZ   = FILTER_ALPHA * motionData.gyroZ   + (1.0f - FILTER_ALPHA) * filteredMotionData.gyroZ;
}

/**
 * Function: updateOrientation
 * Purpose: Updates the orientation using sensor fusion (Madgwick filter).
 * Inputs: None (uses filteredMotionData)
 * Outputs: None (updates global orientation variables)
 */
void updateOrientation() {
#ifdef ENABLE_MOTION_SENSORS
  /**
   * Madgwick expects gyroscope in deg/s, accelerometer in g, magnetometer in uT.
   * It also assumes gravity-positive z-axis and right-handed coordinate system.
   * It will use all 9 DoF values to calculate roll (X), pitch (Y), and yaw (Z).
   */

  // Convert gyroscope from rad/s to deg/s by multiplying by (180.0f / PI).
  float gx = motionData.gyroX * (180.0f / PI);
  float gy = motionData.gyroY * (180.0f / PI);
  float gz = motionData.gyroZ * (180.0f / PI);

  // Convert accelerometer from m/s^2 to g.
  float ax = motionData.accelX / 9.80665f;
  float ay = motionData.accelY / 9.80665f;
  float az = motionData.accelZ / 9.80665f;

  // Magnetometer already in micro-Teslas.
  float mx = motionData.magX;
  float my = motionData.magY;
  float mz = motionData.magZ;

  // Update the filter, using the calculated sample frequency in Hz.
  filter.update(gx, gy, gz, ax, ay, az, mx, my, mz);

  // Get Euler angles (degrees) for position in NED space.
  spatialData.roll = filter.getRoll();
  spatialData.pitch = filter.getPitch();
  spatialData.yaw = filter.getYaw();
#endif
}

/**
 * Function: readMotionSensors
 * Purpose: Reads the motion sensors and prints the data to the debug console (if enabled).
 */
void readMotionSensors() {
#ifdef ENABLE_MOTION_SENSORS
  if(b_imu_found && b_mag_found) {
    // Read the IMU/MAG values every N milliseconds.
    if(!ms_sensor_read_delay.isRunning()) {
      ms_sensor_read_delay.start(i_sensor_read_delay);
    }
    else if(ms_sensor_read_delay.justFinished()) {
      // Poll the sensors.
      sensors_event_t mag, accel, gyro, temp;
      magSensor.getEvent(&mag);
      imuSensor.getEvent(&accel, &gyro, &temp);

      /**
       * Update the raw IMU data in a global object, accounting for the orientation of the magnetometer and IMU sensors relative
       * to the mounted position of the PCB in the wand. In our case, the PCB is mounted upside down, so we need to consider the
       * orientation of the components as looking at the BACK of the PCB with the USB port facing forward (up/north) and the two
       * terminal blocks are on the RIGHT (long edge) of the board. Note that the X/Y orientation of the sensors is based on the
       * robotic coordinate system and mounted face-up so we'll need to adjust for 3D spatial orientation.
       *
       *     |---|
       * |-----------|_
       * |    USB    ||
       * | .G/A      ||  Gyro/Accel Sensor
       * |           |-
       * |           |_
       * |         . ||
       * |        M  ||  Magnetometer
       * |           ||
       * |-----------|-
       *
       * In this orientation both sensors are mounted such that their Y+ is away from the USB port (down), X+ is to the right,
       * and Z+ is toward you (as you look down). However, this does not align with NED (North-East-Down) conventions.
       * 
       * We will use the “Aerospace NED Frame” (North–East–Down convention) for positive values on each axis:
       *  +X = Forward (-Backward)
       *  +Y = Right (-Left)
       *  +Z = Down (toward the Earth at +9.81 m/s^2) remaining "gravity positive" for NED orientation.
       */

      // Update the magnetometer data (swapping the X and Y axes).
      motionData.magX = mag.magnetic.y * -1; // Invert Y for the component's installation.
      motionData.magY = mag.magnetic.x; // X reads as-is because it points East already.
      motionData.magZ = mag.magnetic.z * -1; // Invert Z because we install upside down.

      // Update heading value based on the raw magnetometer X and Y only.
      motionData.heading = calculateHeading(motionData.magX, motionData.magY);

      // Update the acceleration and gyroscope values (swapping the X and Y axes).
      motionData.accelX = accel.acceleration.y * -1; // Invert Y for the component's installation.
      motionData.accelY = accel.acceleration.x; // X reads as-is because it points East already.
      motionData.accelZ = accel.acceleration.z * -1; // Invert Z because we install upside down.
      motionData.gyroX = gyro.gyro.y * -1; // Invert Y for the component's installation.
      motionData.gyroY = gyro.gyro.x; // X reads as-is because it points East already.
      motionData.gyroZ = gyro.gyro.z * -1; // Invert Z because we install upside down.

      // Apply offsets to IMU readings.
      motionData.accelX -= motionOffsets.accelX;
      motionData.accelY -= motionOffsets.accelY;
      motionData.accelZ -= motionOffsets.accelZ;
      motionData.gyroX -= motionOffsets.gyroX;
      motionData.gyroY -= motionOffsets.gyroY;
      motionData.gyroZ -= motionOffsets.gyroZ;

      // Apply smoothing filter to sensor data.
      updateFilteredMotionData();

      // Update heading value based on the moving average magnetometer X and Y only.
      filteredMotionData.heading = calculateHeading(motionData.magX, motionData.magY);

      // Print the filtered sensor data to the debug console.
    #if defined(DEBUG_TELEMETRY_DATA)
      debug("\t\tFiltered Mag   X: ");
      debug(filteredMotionData.magX);
      debug(" \tY: ");
      debug(filteredMotionData.magY);
      debug(" \tZ: ");
      debug(filteredMotionData.magZ);
      debugln(" uTesla ");
      debug("\t\tFiltered Accel X: ");
      debug(filteredMotionData.accelX);
      debug(" \tY: ");
      debug(filteredMotionData.accelY);
      debug(" \tZ: ");
      debug(filteredMotionData.accelZ);
      debugln(" m/s^2 ");
      debug("\t\tFiltered Gyro  X: ");
      debug(filteredMotionData.gyroX);
      debug(" \tY: ");
      debug(filteredMotionData.gyroY);
      debug(" \tZ: ");
      debug(filteredMotionData.gyroZ);
      debugln(" rads/s ");
      debug("\t\tFiltered Heading: ");
      debug(filteredMotionData.heading);
      debugln(" deg ");
      debugln();
    #endif

      // Update the orientation using the filtered data.
      updateOrientation();
    }

    // Report the averaged IMU/MAG values every N milliseconds.
    if(!ms_sensor_report_delay.isRunning()) {
      ms_sensor_report_delay.start(i_sensor_report_delay);
    }
    else if(ms_sensor_report_delay.justFinished()) {
      // Send telemetry data to connected clients via server-side events.
      sendTelemetryData();
    }
  }
#endif
}
