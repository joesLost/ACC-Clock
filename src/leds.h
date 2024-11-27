#ifndef LEDS_H
#define LEDS_H

#include <Adafruit_NeoPixel.h>

// Configuration for NeoPixel LEDs
#define LED_PIN 5          // Data pin for NeoPixel LEDs
#define NUM_LEDS 186        // Number of LEDs in the strip

extern Adafruit_NeoPixel strip;

// Function declarations
void initializeLEDs();
void setLEDColor(int intensity,int red, int green, int blue);
void LEDTest();

#endif // LEDS_H