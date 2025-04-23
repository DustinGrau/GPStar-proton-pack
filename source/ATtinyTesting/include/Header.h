#pragma once

/*
 * All input and output pin definitions go here.
 */

// Pin definitions for SoftwareSerial for GPStar Audio
const uint8_t SOFT_TX_PIN = 0; // TX
const uint8_t SOFT_RX_PIN = 1; // RX

// Create a SoftwareSerial object for audio communication
SoftwareSerial AudioSerial(SOFT_RX_PIN, SOFT_TX_PIN);

// Pin definition for the built-in activity LED
const uint8_t LED_PIN = 10;

// Pin definitions for I2C
const uint8_t SDA_PIN = 14; // PA3
const uint8_t SCL_PIN = 15; // PA4

// Pin definitions for serial communication
const uint8_t TX_PIN = 6; // PB3
const uint8_t RX_PIN = 7; // PB2
