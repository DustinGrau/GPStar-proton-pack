// Required for PlatformIO
#include <Arduino.h>

// Pin definition for the built-in activity LED
const uint8_t LED_PIN = 10;

void setup() {
  // Set the LED pin as an output
  pinMode(LED_PIN, OUTPUT);

  // Turn the LED off initially (active low)
  digitalWrite(LED_PIN, HIGH);
}

void loop() {
  // Turn the LED on (active low)
  digitalWrite(LED_PIN, LOW);
  delay(500); // Wait for 500 milliseconds

  // Turn the LED off
  digitalWrite(LED_PIN, HIGH);
  delay(500); // Wait for 500 milliseconds
}
