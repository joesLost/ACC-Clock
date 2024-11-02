#include <Arduino.h>
#include "leds.h"
#include "dmxHandler.h"
#include "motors.h"

unsigned long lastLogTime = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("Setup starting...");
  delay(1000);

  // Initialize LEDs
  initializeLEDs();
  Serial.println("Setup Complete");

  // Create tasks
  xTaskCreatePinnedToCore(
    dmxHandler, // Function to implement the task
    "DMX Task", // Name of the task
    10000,      // Stack size in words
    NULL,       // Task input parameter
    1,          // Priority of the task
    NULL,       // Task handle
    0);         // Core where the task should run

  xTaskCreatePinnedToCore(
    motorAndLedTask, // Function to implement the task
    "Motor and LED Task", // Name of the task
    10000,      // Stack size in words
    NULL,       // Task input parameter
    1,          // Priority of the task
    NULL,       // Task handle
    1);         // Core where the task should run
}

void loop() {
  // Empty loop as tasks are running on separate cores
  if (millis() - lastLogTime > 60000) { // Check if a minute has passed
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

void motorAndLedTask(void *pvParameters) {
  while (true) {
    serialHandler();
    LEDTest();
    spinTest();
    vTaskDelay(10 / portTICK_PERIOD_MS); // Small delay to prevent watchdog timer reset
  }
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
    } else if (command == "correctTimePosition") {
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