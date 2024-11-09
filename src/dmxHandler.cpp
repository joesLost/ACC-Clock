#include "dmxHandler.h"
#include "motors.h"
#include "leds.h"
#include "globals.h" 

byte data[DMX_PACKET_SIZE]; 
bool dmxIsConnected = false;
unsigned long lastUpdate = 0;
dmx_port_t dmxPort = 1;
dmx_config_t config = DMX_CONFIG_DEFAULT;
dmx_personality_t personalities[] = {
  {6, "Default Personality"}
};

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
          // Log each DMX value to the serial monitor
          for (int i = 0; i < 9; i++) {
            Serial.printf("DMX Channel %d: %d\n", i, data[i]);
          }
          lastUpdate = now;
        }
        
        // Process DMX channels
        processDMXChannels();
      } else {
        Serial.println("A DMX error occurred.");
      }
    }
    vTaskDelay(10 / portTICK_PERIOD_MS); // Small delay to prevent watchdog timer reset
  }
}

void processDMXChannels() {
  MotorCommand cmd;

  // Channel 1: Preset Clock Modes
  switch (data[1]) {
    case 0:
      cmd.type = STOP_HANDS;
      xQueueSend(motorCommandQueue, &cmd, portMAX_DELAY);
      break;
    case 2 ... 5:
      // Real Minute Advance
      cmd.type = MIN_ADVANCE;
      xQueueSend(motorCommandQueue, &cmd, portMAX_DELAY);
      break;
      break;
    case 6 ... 124:
      // Spin Forward in Time
      cmd.type = SPIN_CONTINUOUS;
      cmd.speed = map(data[1], 6, 124, 100, 1);
      cmd.direction = true;
      cmd.proportional = true;
      xQueueSend(motorCommandQueue, &cmd, portMAX_DELAY);
      break;
    case 125 ... 129:
      // Stop Hands
      cmd.type = STOP_HANDS;
      xQueueSend(motorCommandQueue, &cmd, portMAX_DELAY);
      break;
    case 130 ... 249:
      // Spin Backward in Time
      cmd.type = SPIN_CONTINUOUS;
      cmd.speed = map(data[1], 130, 249, 1, 100);
      cmd.direction = false;
      cmd.proportional = true;
      xQueueSend(motorCommandQueue, &cmd, portMAX_DELAY);
      break;
    case 250 ... 254:
      // Real Time Clock Mode
      // Not yet working, need RTC functionality
      //synchronizeRealTime();
      break;
    case 255:
      // Reset to 12:00
      cmd.type = MOVE_TO_HOME;
      xQueueSend(motorCommandQueue, &cmd, portMAX_DELAY);
      break;
  }

  // Channel 2: setTime Speed (1-100) changes how quickly the clock will move to the new time
  int setTimeSpeed = (data[2] == 0) ? 15 : map(data[2], 0, 255, 1, 100);

  // Channel 3-4: Time position (16-bit control, 5-minute intervals with 455 steps per interval)
  if (data[3] != 0 || data[4] != 0) {
    // Combine Channel 3 (high byte) and Channel 4 (low byte) to form the 16-bit value
    int combinedValue = (data[3] << 8) | data[4];

    // Calculate the position within the 12-hour clock (each 5-minute interval corresponds to 455 values)
    int intervalIndex = combinedValue / 455;

    // Calculate hour and minute
    int hour = intervalIndex / 12;
    hour = (hour == 0) ? 12 : hour; // Convert 0 to 12
    int minute = (intervalIndex % 12) * 5; // Convert to 5-minute increments (0, 5, 10, ..., 55)

    cmd.type = SET_TIME;
    cmd.hour = hour;
    cmd.minute = minute;
    xQueueSend(motorCommandQueue, &cmd, portMAX_DELAY);
  }

  // Channel 5: LED Intensity Master
  int ledIntensity = data[5];
  setLEDIntensity(ledIntensity);

  // Channel 6-8: RGB LED Control
  int redIntensity = data[6];
  int greenIntensity = data[7];
  int blueIntensity = data[8];
  setLEDColor(redIntensity, greenIntensity, blueIntensity);
}
