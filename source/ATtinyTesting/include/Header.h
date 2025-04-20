#pragma once

/*
 * All input and output pin definitions go here.
 */

// Pin definitions for SoftwareSerial for GPStar Audio
const uint8_t SOFT_TX_PIN = 0; // TX
const uint8_t SOFT_RX_PIN = 1; // RX

// Create a SoftwareSerial object
SoftwareSerial Serial3(SOFT_RX_PIN, SOFT_TX_PIN);

// Pin definition for the built-in activity LED
const uint8_t LED_PIN = 10;

// Pin definitions for I2C
const uint8_t SDA_PIN = 14; // PA3
const uint8_t SCL_PIN = 15; // PA4

// Pin definitions for serial communication
const uint8_t TX_PIN = 17; // PA1
const uint8_t RX_PIN = 18; // PA2

/*
 * Delay for fastled to update the addressable LEDs.
 * We have up to 126 addressable LEDs if using NeoPixel jewel in the N-Filter, a ring
 * for the Inner Cyclotron, and the optional "sparking" cyclotron cavity LEDs.
 * 0.0312 ms to update each LED, then a 0.05 ms resting period once all are updated.
 * So 4 ms should be okay. Let's bump it up to 5 just in case.
 * For cyclotrons with high density LEDs, increase this based on the cyclotron speed multiplier to simulate a faster spinning cyclotron.
 * This works by "skipping frames" in the animation, which can be done up until about 15 ms.
 * After 15ms it will become painfully obvious to most people that the animation is not smooth.
 */
#define FAST_LED_UPDATE_MS 5
uint8_t i_fast_led_delay = FAST_LED_UPDATE_MS;
millisDelay ms_fast_led;
