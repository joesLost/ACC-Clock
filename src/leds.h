#ifndef LEDS_H
#define LEDS_H

#include <Adafruit_NeoPixel.h>

// Configuration for NeoPixel LEDs
#define LED_PIN 5          // Data pin for NeoPixel LEDs connected to GPIO 5
#define NUM_LEDS 186        // Number of LEDs in the strip
#define BRIGHTNESS 255     // Maximum brightness

extern Adafruit_NeoPixel strip;

void initializeLEDs();
void setLEDColor(int red, int green, int blue);
void setLEDIntensity(int intensity);
void LEDTest();

#endif // LEDS_H