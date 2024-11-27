#include "leds.h"

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_RGB + NEO_KHZ800);

void initializeLEDs() {
  strip.begin();
  strip.setBrightness(255);
  strip.show();
}

void setLEDColor(int intensity, int red, int green, int blue) {
  static int prevInt = 0,prevRed = 0, prevGreen = 0, prevBlue = 0;

  // Only proceed if there's a change
  if (intensity != prevInt || red != prevRed || green != prevGreen || blue != prevBlue) {
    for (int i = 0; i < NUM_LEDS; i++) {
      strip.setPixelColor(i, strip.Color(red, green, blue));
    }
    // Store the current values for the next check
    prevRed = red;
    prevGreen = green;
    prevBlue = blue;
    prevInt = intensity;
  }
  strip.setBrightness(intensity);
  strip.show();
}

void LEDTest() {
  Serial.println("Starting LED test");

  Serial.println("Quick GRB flash");

  // Flash RGB quickly
  for (int i = 0; i < 3; i++) {
    setLEDColor(255, 255, 0, 0); // Red
    delay(300);

    setLEDColor(255, 0, 255, 0); // Green
    delay(300);

    setLEDColor(255, 0, 0, 255); // Blue
    delay(300);
  }
  Serial.println("Max White 5s"); // All LEDs on max brightness for 5 seconds
  setLEDColor(255, 255, 255, 255); // White
  delay(5000);

  Serial.println("RGB cycle 2s each"); // Cycle through RGB at 2 seconds each
  setLEDColor(255, 255, 0, 0); // Red
  delay(2000);

  setLEDColor(255, 0, 255, 0); // Green
  delay(2000);

  setLEDColor(255, 0, 0, 255); // Blue
  delay(2000);

  Serial.println("Rainbow effect 5s"); // Rainbow effect for 5 seconds
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