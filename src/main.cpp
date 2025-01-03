#include <Arduino.h>
#include "dmxHandler.h"
#include "motors.h"
#include "leds.h"
#include "globals.h"

void serialHandler(void* pvParameters);

QueueHandle_t motorCommandQueue;

void setup() {
  Serial.begin(115200);
  Serial.println("Setup starting...");
  delay(1000); //Needed but I forgot why...

  pinMode(PUL_PIN_HR, OUTPUT);
  pinMode(PUL_PIN_MIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);

  // Initialize LEDs
  initializeLEDs();
  initToHome();

  motorCommandQueue = xQueueCreate(10, sizeof(MotorCommand));
  dmx_driver_install(dmxPort, &config, personalities, 1);
  dmx_set_pin(dmxPort, TX_PIN, RX_PIN, RTS_PIN);

  // Create tasks
  xTaskCreatePinnedToCore(
    dmxHandler, // Function to implement the task
    "DMX Task", // Name of the task
    10000,      // Stack size in words
    NULL,       // Task input parameter
    10,          // Priority of the task
    NULL,       // Task handle
    0);         // Core where the task should run

  xTaskCreatePinnedToCore(
    motorControlTask, // Function to implement the task
    "Motor Control Task", // Name of the task
    10000,      // Stack size in words
    NULL,       // Task input parameter
    20,         // Priority of the task
    NULL,       // Task handle
    1           // Core where the task should run
  );         

  //Enable Serial Handler for debugging only, There is some bug inside that will trigger the watchdog 🤷🏻‍♂️
  // xTaskCreate(
  //   serialHandler, // Function to implement the task
  //   "Serial Handler", // Name of the task
  //   10000,      // Stack size in words
  //   NULL,       // Task input parameter
  //   1,          // Priority of the task
  //   NULL
  // ); 
  Serial.println("Setup Complete");
}

void loop() {
 // Silence is golden
}

void serialHandler(void *pvParameters) {
  while (true) {
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
      } else if (command.startsWith("correctTimePos")) {
        int firstSpaceIndex = command.indexOf(' ');
        int secondSpaceIndex = command.indexOf(' ', firstSpaceIndex + 1);

        if (firstSpaceIndex != -1 && secondSpaceIndex != -1) {
          String hourStr = command.substring(firstSpaceIndex + 1, secondSpaceIndex);
          String minuteStr = command.substring(secondSpaceIndex + 1);

          int hour = hourStr.toInt();
          int minute = minuteStr.toInt();

          if (hour >= 1 && hour <= 12 && minute >= 0 && minute < 60) {
            correctTimePos(hour, minute);
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
    taskYIELD(); //Feed the watchdog
  }
}