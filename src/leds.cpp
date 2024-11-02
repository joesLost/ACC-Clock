#include "leds.h"

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

void initializeLEDs() {
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.show();
}

void setLEDColor(int red, int green, int blue) {
  // Set the color of the LEDs only if the color is different from the current one
  static int prevRed = -1, prevGreen = -1, prevBlue = -1;

  // Only proceed if there's a change
  if (red != prevRed || green != prevGreen || blue != prevBlue) {
    for (int i = 0; i < NUM_LEDS; i++) {
      strip.setPixelColor(i, strip.Color(red, green, blue));
    }
    strip.show(); // Commit the changes to the LED strip

    // Store the current values for the next check
    prevRed = red;
    prevGreen = green;
    prevBlue = blue;
  }
}


void setLEDIntensity(int intensity) {
  // Set the overall intensity (brightness) of the LEDs
  strip.setBrightness(intensity);
  strip.show();
}

void LEDTest() {
  // Flash RGB quickly
  for (int i = 0; i < 3; i++) {
    setLEDColor(255, 0, 0); // Red
    delay(300);

    setLEDColor(0, 255, 0); // Green
    delay(300);

    setLEDColor(0, 0, 255); // Blue
    delay(300);
  }

  // All LEDs on max brightness for 5 seconds
  setLEDColor(255, 255, 255); // White
  setLEDIntensity(255);
  delay(5000);

  // Cycle through RGB at 2 seconds each
  setLEDColor(255, 0, 0); // Red
  delay(2000);

  setLEDColor(0, 255, 0); // Green
  delay(2000);

  setLEDColor(0, 0, 255); // Blue
  delay(2000);

  // Rainbow effect for 5 seconds
  unsigned long start = millis();
  while (millis() - start < 5000) {
    for (int i = 0; i < NUM_LEDS; i++) {
      int pixelHue = (i * 65536L / NUM_LEDS) + (millis() - start) * 256;
      strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
    }
    strip.show();
    delay(20);
  }
}