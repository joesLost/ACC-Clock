# 1 "/var/folders/3r/61tk807d3yjbcxrb1srmt7800000gn/T/tmp0vfwcgql"
#include <Arduino.h>
# 1 "/Users/joseph/Documents/PlatformIO/Projects/241027-190457-esp32dev/src/Motor_test.ino"
#include "esp_dmx.h"
#include <Adafruit_NeoPixel.h>


#define PUL_PIN_MIN 18
#define DIR_PIN 19
#define PUL_PIN_HR 22
#define TX_PIN 17
#define RX_PIN 16
#define RTS_PIN 4

#define PULS_PER_REV 8000
const float GEAR_RATIO_MIN = 40.0 / 20.0;
const float GEAR_RATIO_HR = 45.0 / 30.0;
const int HR_STEPS_PER_REV = PULS_PER_REV * GEAR_RATIO_HR;
const int MIN_STEPS_PER_REV = PULS_PER_REV * GEAR_RATIO_MIN;
volatile int CURRENT_HR_STEPS = HR_STEPS_PER_REV /2;
volatile int CURRENT_MIN_STEPS = MIN_STEPS_PER_REV /2;


#define LED_PIN 5
#define NUM_LEDS 93
#define BRIGHTNESS 255
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);


byte data[DMX_PACKET_SIZE];
bool dmxIsConnected = false;
unsigned long lastUpdate = millis();
dmx_port_t dmxPort = 1;
dmx_config_t config = DMX_CONFIG_DEFAULT;
dmx_personality_t personalities[] = {
  {6, "Default Personality"}
};

unsigned long lastLogTime = 0;
void setup();
void loop();
void logStackUsage();
void serialHandler();
void dmxHandler(void *pvParameters);
void processDMXChannels();
void advanceRealMinute();
void spinForward(int speed);
void stopHands();
void spinBackward(int speed);
void synchronizeRealTime();
void resetTo12();
void setHourHand(int hour);
void setMinuteHand(int minute);
void setLEDColor(int red, int green, int blue);
void setLEDIntensity(int intensity);
void pulseMotor(int pulPin, int speedMultiplier);
void spinMotor(bool isMinuteMotor, bool clockwise, int steps, int speedMultiplier);
void spinProportional(int minSteps, int hrSteps, bool clockwise, int maxSpeedMultiplier);
void spinProportionalDuration(int durationMs, bool clockwise, int maxSpeedMultiplier);
int getCurrentHour();
String getCurrentMin();
void checkTime();
void setTime(int hr, int min);
void correctTimePosition(int hr, int min);
void initToHome();
void moveToHome();
void spinTest();
void timeTest();
void LEDTest();
void motorAndLedTask(void *pvParameters);
#line 38 "/Users/joseph/Documents/PlatformIO/Projects/241027-190457-esp32dev/src/Motor_test.ino"
void setup() {
  Serial.begin(115200);
  Serial.println("Setup starting...");
  delay(1000);


  pinMode(PUL_PIN_MIN, OUTPUT);
  pinMode(PUL_PIN_HR, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);


  dmx_driver_install(dmxPort, &config, personalities, 1);
  dmx_set_pin(dmxPort, TX_PIN, RX_PIN, RTS_PIN);


  initToHome();


  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.show();
  Serial.println("Setup Complete");


  xTaskCreatePinnedToCore(
    dmxHandler,
    "DMX Task",
    10000,
    NULL,
    1,
    NULL,
    0);

  xTaskCreatePinnedToCore(
    motorAndLedTask,
    "Motor and LED Task",
    10000,
    NULL,
    1,
    NULL,
    1);
}

void loop() {

  if (millis() - lastLogTime > 60000) {
    logStackUsage();
    lastLogTime = millis();
  }
}

void logStackUsage() {
  UBaseType_t dmxTaskHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
  UBaseType_t motorAndLedTaskHighWaterMark = uxTaskGetStackHighWaterMark(NULL);

  Serial.print("DMX Task Stack High Water Mark: ");
  Serial.println(dmxTaskHighWaterMark);

  Serial.print("Motor and LED Task Stack High Water Mark: ");
  Serial.println(motorAndLedTaskHighWaterMark);
}

void serialHandler() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    Serial.print("Received command: ");
    Serial.println(command);

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

void dmxHandler(void *pvParameters) {
  while (true) {
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


        processDMXChannels();
      } else {
        Serial.println("A DMX error occurred.");
      }
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void processDMXChannels() {

  switch (data[0]) {
    case 0:

      break;
    case 1 ... 5:

      advanceRealMinute();
      break;
    case 6 ... 124:

      spinForward(map(data[0], 6, 124, 124, 6));
      break;
    case 125 ... 129:

      stopHands();
      break;
    case 130 ... 249:

      spinBackward(map(data[0], 130, 249, 130, 249));
      break;
    case 250 ... 254:

      synchronizeRealTime();
      break;
    case 255:

      resetTo12();
      break;
  }


  if (data[1] != 0) {
    int hour = map(data[1], 1, 255, 0, 12);
    setHourHand(hour);
  }


  if (data[2] != 0) {
    int minute = map(data[2], 1, 255, 0, 60);
    setMinuteHand(minute);
  }


  int redIntensity = data[3];
  int greenIntensity = data[4];
  int blueIntensity = data[5];
  setLEDColor(redIntensity, greenIntensity, blueIntensity);


  int ledIntensity = data[6];
  setLEDIntensity(ledIntensity);
}



void setLEDColor(int red, int green, int blue) {

  strip.setPixelColor(0, strip.Color(red, green, blue));
  strip.show();
}

void setLEDIntensity(int intensity) {

  strip.setBrightness(intensity);
  strip.show();
}

void pulseMotor(int pulPin, int speedMultiplier) {
  int delayTime = max(1000 / speedMultiplier, 10);
  digitalWrite(pulPin, HIGH);
  delayMicroseconds(20);
  digitalWrite(pulPin, LOW);
  delayMicroseconds(delayTime);
}

void spinMotor(bool isMinuteMotor, bool clockwise, int steps, int speedMultiplier) {
  int pulPin = isMinuteMotor ? PUL_PIN_MIN : PUL_PIN_HR;

  digitalWrite(DIR_PIN, clockwise ? LOW : HIGH);
  Serial.println(steps);
  for (int i = 0; i < steps; i++) {
    pulseMotor(pulPin, speedMultiplier);


    if (isMinuteMotor) {
      if (clockwise) {
        CURRENT_MIN_STEPS = (CURRENT_MIN_STEPS + 1) % MIN_STEPS_PER_REV;
            } else {
        CURRENT_MIN_STEPS = (CURRENT_MIN_STEPS - 1 + MIN_STEPS_PER_REV) % MIN_STEPS_PER_REV;
            }
    } else {
      if (clockwise) {
        CURRENT_HR_STEPS = ( CURRENT_HR_STEPS + 1) % HR_STEPS_PER_REV;
      } else {
        CURRENT_HR_STEPS = ( CURRENT_HR_STEPS - 1 + HR_STEPS_PER_REV) % HR_STEPS_PER_REV;
      }
    }
  }
}

void spinProportional(int minSteps, int hrSteps, bool clockwise, int maxSpeedMultiplier) {
  int totalSteps = max(minSteps, hrSteps);
  int rampUpSteps = totalSteps / 15;
  int rampDownSteps = totalSteps / 15;

  int currentSpeed = 1;
  int minCounter = 0;
  int hrCounter = 0;

  digitalWrite(DIR_PIN, clockwise ? LOW : HIGH);

  for (int step = 0; step < totalSteps; step++) {

    if (step < rampUpSteps) {
      currentSpeed = map(step, 0, rampUpSteps, 1, maxSpeedMultiplier);
    } else if (step > totalSteps - rampDownSteps) {
      currentSpeed = map(step, totalSteps - rampDownSteps, totalSteps, maxSpeedMultiplier, 1);
    } else {
      currentSpeed = maxSpeedMultiplier;
    }


    if (minCounter < minSteps) {
      pulseMotor(PUL_PIN_MIN, currentSpeed);
      minCounter++;
      if (clockwise) {
        CURRENT_MIN_STEPS = (CURRENT_MIN_STEPS + 1) % MIN_STEPS_PER_REV;
      } else {
        CURRENT_MIN_STEPS = (CURRENT_MIN_STEPS - 1 + MIN_STEPS_PER_REV) % MIN_STEPS_PER_REV;
      }
    }


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
  int rampUpSteps = 1000;
  int rampDownSteps = 1000;
  int totalSteps = durationMs * maxSpeedMultiplier;
  int minCounter = 0;

  for (int step = 0; step < totalSteps; step++) {

    if (step < rampUpSteps) {
      currentSpeed = map(step, 0, rampUpSteps, 1, maxSpeedMultiplier);
    } else if (step > totalSteps - rampDownSteps) {
      currentSpeed = map(step, totalSteps - rampDownSteps, totalSteps, maxSpeedMultiplier, 1);
    } else {
      currentSpeed = maxSpeedMultiplier;
    }


    pulseMotor(PUL_PIN_MIN, currentSpeed);


    minCounter++;
    if (minCounter % 12 == 0) {
      pulseMotor(PUL_PIN_HR, currentSpeed);
    }
  }
}

int getCurrentHour() {
  int hour = (CURRENT_HR_STEPS % HR_STEPS_PER_REV) / (HR_STEPS_PER_REV / 12);
  return (hour == 0 ? 12 : hour);
}
String getCurrentMin() {
  int minute = (CURRENT_MIN_STEPS % MIN_STEPS_PER_REV) / (MIN_STEPS_PER_REV / 60);
  return String(minute < 10 ? "0" : "") + String(minute);
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


  int targetHrSteps = map(hr, 0, 12, 0, HR_STEPS_PER_REV);
  int targetMinSteps = map(min, 0, 60, 0, MIN_STEPS_PER_REV);


  int hrSteps = (targetHrSteps - CURRENT_HR_STEPS + HR_STEPS_PER_REV) % HR_STEPS_PER_REV;
  int minSteps = (targetMinSteps - CURRENT_MIN_STEPS + MIN_STEPS_PER_REV) % MIN_STEPS_PER_REV;


  spinProportional(minSteps, hrSteps, false, 15);

  Serial.println("Steps to target: ");
  Serial.print("Hour: ");
  Serial.print(targetHrSteps);
  Serial.print(" Min: ");
  Serial.println(targetMinSteps);


  CURRENT_HR_STEPS = targetHrSteps;
  CURRENT_MIN_STEPS = targetMinSteps;
}

void correctTimePosition(int hr, int min) {

  int hrSteps = map(hr, 0, 12, 0, HR_STEPS_PER_REV);
  int minSteps = map(min, 0, 59, 0, MIN_STEPS_PER_REV);

  CURRENT_HR_STEPS = hrSteps;
  CURRENT_MIN_STEPS = minSteps;
  moveToHome();
}

void initToHome() {
  Serial.println("Moving to home position...");

  spinMotor(true, true, MIN_STEPS_PER_REV / 2 , 3);
  spinMotor(false, true, HR_STEPS_PER_REV / 2, 3);
  CURRENT_HR_STEPS = 0;
  CURRENT_MIN_STEPS = 0;
}

void moveToHome() {
  Serial.println("Moving to home position...");

  int hrStepsToHome = (0 - CURRENT_HR_STEPS + HR_STEPS_PER_REV) % HR_STEPS_PER_REV;
  int minStepsToHome = (0 - CURRENT_MIN_STEPS + MIN_STEPS_PER_REV) % MIN_STEPS_PER_REV;

  spinMotor(false, true, abs(hrStepsToHome), 3);
  spinMotor(true, true, abs(minStepsToHome), 3);

  CURRENT_HR_STEPS = 0;
  CURRENT_MIN_STEPS = 0;
}

void spinTest() {
  for(int i = 10; i <=100; i+=10){
    spinProportionalDuration(5000, true, i);
    delay(1000);
  }
}

void timeTest() {
  for (int h = 1; h<= 12; h++){
    for(int m = 4; m <=59; m+=5){
      setTime(h,m);
      delay(1000);
    }
  }
}

void LEDTest() {

  for (int i = 0; i < 3; i++) {
    strip.fill(strip.Color(255, 0, 0), 0, NUM_LEDS);
    strip.show();
    delay(300);

    strip.fill(strip.Color(0, 255, 0), 0, NUM_LEDS);
    strip.show();
    delay(300);

    strip.fill(strip.Color(0, 0, 255), 0, NUM_LEDS);
    strip.show();
    delay(300);
  }


  strip.fill(strip.Color(255, 255, 255), 0, NUM_LEDS);
  strip.setBrightness(255);
  strip.show();
  delay(5000);


  strip.fill(strip.Color(255, 0, 0), 0, NUM_LEDS);
  strip.show();
  delay(2000);

  strip.fill(strip.Color(0, 255, 0), 0, NUM_LEDS);
  strip.show();
  delay(2000);

  strip.fill(strip.Color(0, 0, 255), 0, NUM_LEDS);
  strip.show();
  delay(2000);


  unsigned long start = millis();
  while (millis() - start < 5000) {
    for (int i = 0; i < NUM_LEDS; i++) {
      strip.setPixelColor(i, strip.ColorHSV((millis() / 10 + i * 10) % 65536));
    }
    strip.show();
    delay(50);
  }


  strip.fill(strip.Color(0, 0, 0), 0, NUM_LEDS);
  strip.show();
}

void motorAndLedTask(void *pvParameters) {
  while (true) {
    serialHandler();
    LEDTest();
    spinTest();
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}