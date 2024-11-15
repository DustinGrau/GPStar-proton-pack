#include <FastLED.h>

#define LED_PIN 21     // GPIO pin connected to the data line of the WS2812 LED
#define NUM_LEDS 1     // Number of LEDs in the strip
CRGB leds[NUM_LEDS];   // Create an array to hold LED data

void setup() {
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);  // Initialize WS2812 LED
  Serial.begin(115200);  // Start Serial communication at 115200 baud
} 

void loop() {
  leds[0] = CRGB::Green; // Set LED to green
  FastLED.show();        // Update the LED to display the color
  Serial.println("Hello world"); // Print message when LED is lit
  delay(1000);           // Wait for 1 second

  leds[0] = CRGB::Black; // Turn off the LED
  FastLED.show();        // Update the LED to turn off
  delay(1000);           // Wait for 1 second
}
