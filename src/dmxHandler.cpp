#include "dmxHandler.h"
#include "motors.h"
#include "leds.h"


extern byte data[DMX_PACKET_SIZE];
extern bool dmxIsConnected;
extern unsigned long lastUpdate;
extern dmx_port_t dmxPort;

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
  // Channel 1: Preset Clock Modes
  switch (data[0]) {
    case 0:
      // No Action (idle state)
      break;
    case 1 ... 5:
      // Real Minute Advance
      // Not yet working, need RTC functionality
      // advanceRealMinute();
      break;
    case 6 ... 124:
      // Spin Forward in Time
      spinContinuous(map(data[0], 6, 124, 100, 1), true);
      break;
    case 125 ... 129:
      // Stop Hands
      //stopHands();
      break;
    case 130 ... 249:
      // Spin Backward in Time
      spinContinuous(map(data[0], 130, 249, 1, 100), false);
      break;
    case 250 ... 254:
      // Real Time Clock Mode
      // Not yet working, need RTC functionality
      //synchronizeRealTime();
      break;
    case 255:
      // Reset to 12:00
      moveToHome();
      break;
  }

  // Channel 2-3: Time position (16-bit control, 5-minute intervals with 455 steps per interval)
  if (data[1] != 0 || data[2] != 0) {
    // Combine Channel 2 (high byte) and Channel 3 (low byte) to form the 16-bit value
    int combinedValue = (data[1] << 8) | data[2];

    // Calculate the position within the 12-hour clock (each 5-minute interval corresponds to 455 values)
    int intervalIndex = combinedValue / 455;

    // Calculate hour and minute
    int hour = intervalIndex / 12;         // Index of the hour (0 to 11)
    int minute = (intervalIndex % 12) * 5; // Convert to 5-minute increments (0, 5, 10, ..., 55)

    // Set the time using setTime function
    setTime(hour, minute);
  }

  // Channel 4-6: RGB LED Control
  int redIntensity = data[3];
  int greenIntensity = data[4];
  int blueIntensity = data[5];
  setLEDColor(redIntensity, greenIntensity, blueIntensity);

  // Channel 7: LED Intensity Master
  int ledIntensity = data[6];
  setLEDIntensity(ledIntensity);
}
