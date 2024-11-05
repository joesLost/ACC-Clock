#include <Arduino.h>
#include "dmxHandler.h"
#include "motors.h"
#include "leds.h"
// Function declarations
void logStackUsage(void* pvParameters);
void serialHandler(void* pvParameters);

QueueHandle_t motorCommandQueue;


void setup() {
  Serial.begin(115200);
  Serial.println("Setup starting...");
  delay(1000);

  // Initialize LEDs
  initializeLEDs();
  Serial.println("Setup Complete");

  motorCommandQueue = xQueueCreate(10, sizeof(MotorCommand));


  dmx_driver_install(dmxPort, &config, personalities, 1);
  dmx_set_pin(dmxPort, TX_PIN, RX_PIN, RTS_PIN);

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
    motorControlTask, // Function to implement the task
    "Motor Control Task", // Name of the task
    10000,      // Stack size in words
    NULL,       // Task input parameter
    1,          // Priority of the task
    NULL,       // Task handle
    1);         // Core where the task should run
  xTaskCreate(
    logStackUsage, // Function to implement the task
    "Log Stack Usage", // Name of the task
    10000,      // Stack size in words
    NULL,       // Task input parameter
    1,          // Priority of the task
    NULL);      // Task handle
}

void logStackUsage(void *pvParameters) {
  while (true) {
      // Get the number of tasks currently running
      UBaseType_t taskCount = uxTaskGetNumberOfTasks();

      // Allocate a buffer to store task information
      // Each task information takes about 40 characters
      char taskList[taskCount * 40];

      // Get task information
      vTaskList(taskList);

      Serial.println("Task Name\tState\tPrio\tStack\tTask#");
      Serial.println("-------------------------------------------------");
      Serial.print(taskList);

      vTaskDelay(60000 / portTICK_PERIOD_MS); // Delay for 60,000 milliseconds (1 minute)
  }
}