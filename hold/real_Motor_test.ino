#include "esp_dmx.h"
#include <Adafruit_NeoPixel.h>

// Configuration for the stepper motors controlling min and HR hands
#define PUL_PIN_MIN 18  // Pulse pin connected to PUL+ on DM542 for min hand motor
#define DIR_PIN 19  // Direction pin connected to DIR+ on both DM542
#define PUL_PIN_HR 22    // Pulse pin connected to PUL+ on DM542 for Hour hand motor
#define TX_PIN 17 // DMX transmitter pin
#define RX_PIN 16 // DMX receiver pin
#define RTS_PIN 4 // DMX rts pin

#define PULS_PER_REV 8000  // The number of pulses to make one full revolution
const float GEAR_RATIO_MIN = 40.0 / 20.0; // Shaft gear / Motor Gear
const float GEAR_RATIO_HR = 45.0 / 30.0; // Shaft gear / Motor Gear
const int HR_STEPS_PER_REV = PULS_PER_REV * GEAR_RATIO_HR; // The number of steps to make one full revolution on the hour hand
const int MIN_STEPS_PER_REV = PULS_PER_REV * GEAR_RATIO_MIN; // The number of steps to make one full revolution on the minute hand
volatile int  CURRENT_HR_STEPS = HR_STEPS_PER_REV /2; // Tracks the hour hand position in steps. Initial position is 6 o'clock
volatile int CURRENT_MIN_STEPS = MIN_STEPS_PER_REV /2; // Tracks the minute hand position in steps. Initial position is 30 minutes

// Configuration for NeoPixel LEDs
#define LED_PIN 5          // Data pin for NeoPixel LEDs connected to GPIO 5
#define NUM_LEDS 93        // Number of LEDs in the strip
#define BRIGHTNESS 255     // Maximum brightness
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Configuration for DMX
byte data[DMX_PACKET_SIZE];
bool dmxIsConnected = false;
unsigned long lastUpdate = millis();
dmx_port_t dmxPort = 1;
dmx_config_t config = DMX_CONFIG_DEFAULT;
dmx_personality_t personalities[] = {
  {6, "Default Personality"} // 6 channels
};

void setup() {
  Serial.begin(115200);
  Serial.println("Setup starting...");
  delay(1000);

  // Set up pins for hand motora
  pinMode(PUL_PIN_MIN, OUTPUT);
  pinMode(PUL_PIN_HR, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);

  //Initialize DMX
  dmx_driver_install(dmxPort, &config, personalities, 1);
  dmx_set_pin(dmxPort, TX_PIN, RX_PIN, RTS_PIN);

  // Move to home position initially
  initToHome();

  // Initialize LEDs
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.show();
  Serial.println("Setup Complete");

}

void loop() {
  //dmxHandler();
  //serialHandler();
  LEDTest();
  spinTest();
}

void serialHandler() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n'); // Read input until newline
    command.trim(); // Remove any leading/trailing whitespace
    Serial.print("Received command: ");
    Serial.println(command); // Log the received command
    // Call functions based on the command received
    if (command == "home") {
      moveToHome();
    } else if (command == "LEDTest") {
      LEDTest();
    } else if (command == "spinTest") {
      spinTest();
    } else if (command == "checkTime") {
      checkTime();
    }else if (command == "correctTimePosition") {
      int firstSpaceIndex = command.indexOf(' ');
      int secondSpaceIndex = command.indexOf(' ', firstSpaceIndex + 1);

      if (firstSpaceIndex != -1 && secondSpaceIndex != -1) {
        String hourStr = command.substring(firstSpaceIndex + 1, secondSpaceIndex);
        String minuteStr = command.substring(secondSpaceIndex + 1);

        int hour = hourStr.toInt();
        int minute = minuteStr.toInt();

        if (hour >= 1 && hour <= 12 && minute >= 0 && minute < 60) {
          correctTimePosition(hour, minute);
        } else {
          Serial.println("Invalid time. Please enter hour (1-12) and minute (0-59).");
        }
      } else {
        Serial.println("Invalid command format. Use: correctTimePosition hour minute");
      }
    } else if (command.startsWith("setTime")) {
      // Example: setTime 10 30 to set the clock to 10:30
      int firstSpaceIndex = command.indexOf(' ');
      int secondSpaceIndex = command.indexOf(' ', firstSpaceIndex + 1);

      if (firstSpaceIndex != -1 && secondSpaceIndex != -1) {
        String hourStr = command.substring(firstSpaceIndex + 1, secondSpaceIndex);
        String minuteStr = command.substring(secondSpaceIndex + 1);

        int hour = hourStr.toInt();
        int minute = minuteStr.toInt();

        if (hour >= 1 && hour <= 12 && minute >= 0 && minute < 60) {
          setTime(hour, minute);
        } else {
          Serial.println("Invalid time. Please enter hour (1-12) and minute (0-59).");
        }
      } else {
        Serial.println("Invalid command format. Use: setTime hour minute");
      } 
    } else {
      Serial.println("Unknown command.");
    }
  }
}

void dmxHandler() {
  dmx_packet_t packet;
  if (dmx_receive(dmxPort, &packet, DMX_TIMEOUT_TICK)) {
    unsigned long now = millis();

    if (!packet.err) {
      if (!dmxIsConnected) {
        Serial.println("DMX is connected!");
        dmxIsConnected = true;
      }

      dmx_read(dmxPort, data, packet.size);

      if (now - lastUpdate > 1000) {
        Serial.printf("Start code is 0x%02X and slot 1 is 0x%02X\n", data[0], data[1]);
        lastUpdate = now;
      }
    } else {
      Serial.println("A DMX error occurred.");
    }
  }
}

void pulseMotor(int pulPin, int speedMultiplier) {
  int delayTime = max(1000 / speedMultiplier, 10); // Reduce delay time for higher speed, ensure a minimum delay of 20 µs
  digitalWrite(pulPin, HIGH);
  delayMicroseconds(20); // Ensure minimum pulse width for detection
  digitalWrite(pulPin, LOW);
  delayMicroseconds(delayTime);
}

void spinMotor(bool isMinuteMotor, bool clockwise, int steps, int speedMultiplier) {
  int pulPin = isMinuteMotor ? PUL_PIN_MIN : PUL_PIN_HR;

  digitalWrite(DIR_PIN, clockwise ? LOW : HIGH);
  Serial.println(steps);
  for (int i = 0; i < steps; i++) {
    pulseMotor(pulPin, speedMultiplier);

    // Update the current position
    if (isMinuteMotor) {
      if (clockwise) {
        CURRENT_MIN_STEPS = (CURRENT_MIN_STEPS + 1) % MIN_STEPS_PER_REV;
            } else {
        CURRENT_MIN_STEPS = (CURRENT_MIN_STEPS - 1 + MIN_STEPS_PER_REV) % MIN_STEPS_PER_REV;
            }
    } else {
      if (clockwise) {
        CURRENT_HR_STEPS = (  CURRENT_HR_STEPS + 1) % HR_STEPS_PER_REV;
      } else {
        CURRENT_HR_STEPS = (  CURRENT_HR_STEPS - 1 + HR_STEPS_PER_REV) % HR_STEPS_PER_REV;
      }
    }
  }
}

void spinProportional(int minSteps, int hrSteps, bool clockwise, int maxSpeedMultiplier) {
  int totalSteps = max(minSteps, hrSteps);
  int rampUpSteps = totalSteps / 15;  // Number of steps to ramp up speed
  int rampDownSteps = totalSteps / 15; // Number of steps to ramp down speed

  int currentSpeed = 1;
  int minCounter = 0;
  int hrCounter = 0;

  digitalWrite(DIR_PIN, clockwise ? LOW : HIGH);

  for (int step = 0; step < totalSteps; step++) {
    // Adjust speed during acceleration and deceleration phases
    if (step < rampUpSteps) {
      currentSpeed = map(step, 0, rampUpSteps, 1, maxSpeedMultiplier);
    } else if (step > totalSteps - rampDownSteps) {
      currentSpeed = map(step, totalSteps - rampDownSteps, totalSteps, maxSpeedMultiplier, 1);
    } else {
      currentSpeed = maxSpeedMultiplier;
    }

    // Spin min hand
    if (minCounter < minSteps) {
      pulseMotor(PUL_PIN_MIN, currentSpeed);
      minCounter++;
      if (clockwise) {
        CURRENT_MIN_STEPS = (CURRENT_MIN_STEPS + 1) % MIN_STEPS_PER_REV;
      } else {
        CURRENT_MIN_STEPS = (CURRENT_MIN_STEPS - 1 + MIN_STEPS_PER_REV) % MIN_STEPS_PER_REV;
      }
    }

    // Spin HR hand every 12 steps of the min hand
    if (hrCounter < hrSteps && minCounter % 12 == 0) {
      pulseMotor(PUL_PIN_HR, currentSpeed);
      hrCounter++;
      if (clockwise) {
        CURRENT_HR_STEPS = (CURRENT_HR_STEPS + 1) % HR_STEPS_PER_REV;
      } else {
        CURRENT_HR_STEPS = (CURRENT_HR_STEPS - 1 + HR_STEPS_PER_REV) % HR_STEPS_PER_REV;
      }
    }
  }
}

void spinProportionalDuration(int durationMs, bool clockwise, int maxSpeedMultiplier) {
  unsigned long endTime = millis() + durationMs;
  digitalWrite(DIR_PIN, clockwise ? LOW : HIGH);

  int currentSpeed = 1;
  int rampUpSteps = 1000;  // Number of steps to ramp up speed
  int rampDownSteps = 1000; // Number of steps to ramp down speed
  int totalSteps = durationMs * maxSpeedMultiplier; // Estimated total steps based on duration and speed
  int minCounter = 0;

  for (int step = 0; step < totalSteps; step++) {
    // Adjust speed during acceleration and deceleration phases
    if (step < rampUpSteps) {
      currentSpeed = map(step, 0, rampUpSteps, 1, maxSpeedMultiplier);
    } else if (step > totalSteps - rampDownSteps) {
      currentSpeed = map(step, totalSteps - rampDownSteps, totalSteps, maxSpeedMultiplier, 1);
    } else {
      currentSpeed = maxSpeedMultiplier;
    }

    // Spin min hand
    pulseMotor(PUL_PIN_MIN, currentSpeed);

    // Spin HR hand every 12 steps of the min hand
    minCounter++;
    if (minCounter % 12 == 0) {
      pulseMotor(PUL_PIN_HR, currentSpeed);
    }
  }
}

int getCurrentHour() {
  int hour =  (CURRENT_HR_STEPS % HR_STEPS_PER_REV) / (HR_STEPS_PER_REV / 12);
  return (hour == 0 ? 12 : hour); //1-12 valid values for hours, replace 0 with 12
}
String getCurrentMin() {
  int minute = (CURRENT_MIN_STEPS % MIN_STEPS_PER_REV) / (MIN_STEPS_PER_REV / 60);
  return String(minute < 10 ? "0" : "") + String(minute); //kinda hacky way to add leading 0
}

void checkTime() {
  Serial.print("Current Time: ");
  Serial.print(getCurrentHour());
  Serial.print(":");
  Serial.println(getCurrentMin());
  Serial.println("Current Steps: ");
  Serial.print("Hour: ");
  Serial.print(CURRENT_HR_STEPS);
  Serial.print(" Min: ");
  Serial.println(CURRENT_MIN_STEPS);
}

void setTime(int hr, int min) {
  Serial.print("Prior time: ");
  Serial.print(getCurrentHour());
  Serial.print(":");
  Serial.print(getCurrentMin());
  Serial.print(" Setting time to ");
  Serial.print(hr);
  Serial.print(":");
  Serial.println(min);

  // Calculate the target positions in steps
  int targetHrSteps = map(hr, 0, 12, 0, HR_STEPS_PER_REV);
  int targetMinSteps = map(min, 0, 60, 0, MIN_STEPS_PER_REV);

  // Calculate the steps needed to move to the target positions
  int hrSteps = (targetHrSteps -  CURRENT_HR_STEPS + HR_STEPS_PER_REV) % HR_STEPS_PER_REV;
  int minSteps = (targetMinSteps - CURRENT_MIN_STEPS + MIN_STEPS_PER_REV) % MIN_STEPS_PER_REV;

  // Move the motors
  spinProportional(minSteps, hrSteps, false, 15);

  Serial.println("Steps to target: ");
  Serial.print("Hour: ");
  Serial.print(targetHrSteps);
  Serial.print(" Min: ");
  Serial.println(targetMinSteps);

  // Update the current positions
  CURRENT_HR_STEPS = targetHrSteps;
  CURRENT_MIN_STEPS = targetMinSteps;
}

void correctTimePosition(int hr, int min) {
  // Calculate the target positions in steps
  int hrSteps = map(hr, 0, 12, 0, HR_STEPS_PER_REV);
  int minSteps = map(min, 0, 59, 0, MIN_STEPS_PER_REV);

  CURRENT_HR_STEPS = hrSteps;
  CURRENT_MIN_STEPS = minSteps;
  moveToHome();
}

void initToHome() {
  Serial.println("Moving to home position...");
  // Assume starting at 6 o'clock (straight down), move to 12 o'clock
  spinMotor(true, true, MIN_STEPS_PER_REV / 2 , 3);  // Minute hand: 6 HRs clockwise
  spinMotor(false, true, HR_STEPS_PER_REV / 2, 3); // Hour hand: 6 HRs clockwise
  CURRENT_HR_STEPS = 0;
  CURRENT_MIN_STEPS = 0;
}

void moveToHome() {
  Serial.println("Moving to home position...");
  // Calculate the steps needed to move to the home position (12 o'clock)
  int hrStepsToHome = (0 - CURRENT_HR_STEPS + HR_STEPS_PER_REV) % HR_STEPS_PER_REV;
  int minStepsToHome = (0 - CURRENT_MIN_STEPS + MIN_STEPS_PER_REV) % MIN_STEPS_PER_REV;

  spinMotor(false, true, abs(hrStepsToHome), 3);
  spinMotor(true, true, abs(minStepsToHome), 3);
  // Update the current positions
  CURRENT_HR_STEPS = 0;
  CURRENT_MIN_STEPS = 0;
}

void spinTest() {
  for(int i = 10; i <=100; i+=10){ //0-59
    spinProportionalDuration(5000, true, i);
    delay(1000);
  }
}

void timeTest() {
  for (int h = 1; h<= 12; h++){ //1-12
    for(int m = 4; m <=59; m+=5){ //0-59
      setTime(h,m);
      delay(1000);
    }
  }
}

void LEDTest() {
  // Flash RGB quickly
  for (int i = 0; i < 3; i++) {
    strip.fill(strip.Color(255, 0, 0), 0, NUM_LEDS); // Red
    strip.show();
    delay(300);

    strip.fill(strip.Color(0, 255, 0), 0, NUM_LEDS); // Green
    strip.show();
    delay(300);

    strip.fill(strip.Color(0, 0, 255), 0, NUM_LEDS); // Blue
    strip.show();
    delay(300);
  }

  // All LEDs on max brightness for 5 seconds
  strip.fill(strip.Color(255, 255, 255), 0, NUM_LEDS); // White
  strip.setBrightness(255);
  strip.show();
  delay(5000);

  // Cycle through RGB at 2 seconds each
  strip.fill(strip.Color(255, 0, 0), 0, NUM_LEDS); // Red
  strip.show();
  delay(2000);

  strip.fill(strip.Color(0, 255, 0), 0, NUM_LEDS); // Green
  strip.show();
  delay(2000);

  strip.fill(strip.Color(0, 0, 255), 0, NUM_LEDS); // Blue
  strip.show();
  delay(2000);

  // Rainbow effect for 5 seconds
  unsigned long start = millis();
  while (millis() - start < 5000) {
    for (int i = 0; i < NUM_LEDS; i++) {
      strip.setPixelColor(i, strip.ColorHSV((millis() / 10 + i * 10) % 65536));
    }
    strip.show();
    delay(50);
  }

  // Turn off LEDs
  strip.fill(strip.Color(0, 0, 0), 0, NUM_LEDS); // Black (off)
  strip.show();
}