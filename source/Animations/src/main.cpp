// Required for PlatformIO
#include <Arduino.h>

// 3rd-Party Libraries
#include <FastLED.h>
#include <millisDelay.h>

// Pin and LED configuration
#define PIXEL_PIN 33
#define WHITE_PIN 34
#define NUM_LEDS 12
#define COLOR_CHANGE_DELAY 500 // milliseconds
#define WHITE_TOGGLE_DELAY 1000 // milliseconds

CRGB pixels[NUM_LEDS];
millisDelay colorChangeDelay;
millisDelay whitePinDelay;

void setup() {
    FastLED.addLeds<WS2812, PIXEL_PIN, GRB>(pixels, NUM_LEDS);
    colorChangeDelay.start(COLOR_CHANGE_DELAY);
    pinMode(WHITE_PIN, OUTPUT);
    whitePinDelay.start(WHITE_TOGGLE_DELAY);
}

void updateColorAnimation() {
    static uint8_t hue = 0;

    if (colorChangeDelay.justFinished()) {
        colorChangeDelay.repeat(); // Restart the delay
        fill_solid(pixels, NUM_LEDS, CHSV(hue, 255, 255)); // Set LEDs to current hue
        FastLED.show();
        hue += 10; // Increment hue for next cycle
    }
}

void toggleWhitePin() {
    if (whitePinDelay.justFinished()) {
        whitePinDelay.repeat(); // Restart the delay
        static bool whitePinState = LOW;
        whitePinState = !whitePinState; // Toggle state
        digitalWrite(WHITE_PIN, whitePinState);
    }
}

void loop() {
    updateColorAnimation();
    toggleWhitePin();
}
