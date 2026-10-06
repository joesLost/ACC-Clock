#include "dmxHandler.h"
#include "motors.h"
#include "leds.h"
#include "globals.h" 

byte data[DMX_PACKET_SIZE]; 
bool dmxIsConnected = false;
//Hardcoded DMX Start Address
int dmxAddress = 500;
unsigned long lastUpdate = 0;
dmx_port_t dmxPort = 1;
dmx_config_t config = DMX_CONFIG_DEFAULT;
dmx_personality_t personalities[] = {
  {8, "Default Personality"}
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

        //Useful for debugging, prints out dmx channel values
        // if (now - lastUpdate > 1000) {
        //   Serial.printf("Start code is 0x%02X and slot 1 is 0x%02X\n", data[0], data[1]);
        //   // Log each DMX value to the serial monitor
        //   for (int i = 1 + dmxAddress; i < 9 + dmxAddress; i++) {
        //     Serial.printf("DMX Channel %d: %d\n", i, data[i]);
        //   }
        //   lastUpdate = now;
        // }
        
        // Process DMX channels
        processDMXChannels();
      } else {
        Serial.println("A DMX error occurred.");
      }
    } else {
      Serial.println("No DMX packet received.");
    }
    taskYIELD(); //Feed the watchdog
  }
}

void processDMXChannels() {
  MotorCommand cmd = {};
  static MotorCommand lastCmd = {};
  static bool hasLastCmd = false;
  
  // Channel 2: setTime Speed (1-100) changes how quickly the clock will move to the new time
  int setTimeSpeed = map(data[2 + dmxAddress], 0, 255, 0, 100);
  cmd.speed = setTimeSpeed;
  
  // Channel 3-4: Time position (16-bit control, 5-minute intervals with 455 steps per interval)
  if (data[3 + dmxAddress] != 0 || data[4 + dmxAddress] != 0) {
    // Combine Channel 3 (high byte) and Channel 4 (low byte) to form the 16-bit value
    int combinedValue = (data[3 + dmxAddress] << 8) | data[4 + dmxAddress];
    if(combinedValue > 0){
      // Calculate the position within the 12-hour clock (each 5-minute interval corresponds to 455 values)
      int intervalIndex = combinedValue / 455;
      int hour = intervalIndex / 12;
      hour = (hour == 0) ? 12 : hour; // Convert 0 to 12
      int minute = (intervalIndex % 12) * 5; // Convert to 5-minute increments (0, 5, 10, ..., 55)
      cmd.hour = hour;
      cmd.minute = minute;
    }
  }
  
  // Channel 1: Preset Clock Modes
  switch (data[1 + dmxAddress]) {
    case 0 ... 5:
      cmd.type = STOP_HANDS;
            if (!hasLastCmd || !isSameMotorCommand(cmd, lastCmd)) {
        xQueueSend(motorCommandQueue, &cmd, portMAX_DELAY);
        lastCmd = cmd;
        hasLastCmd = true;
      }
      break;
    case 6 ... 10:
      cmd.type = SET_TIME;
      cmd.speed = setTimeSpeed == 0 ? 5 : setTimeSpeed;
      if(not (cmd.hour == getCurrentHour() && cmd.minute == getCurrentMin())){
        if (!hasLastCmd || !isSameMotorCommand(cmd, lastCmd)) {
          xQueueSend(motorCommandQueue, &cmd, portMAX_DELAY);
          lastCmd = cmd;
          hasLastCmd = true;
        }
      }
      break;
    case 11 ... 15:
      //Set position toggle
      cmd.type = SET_POSITION;
      cmd.speed = setTimeSpeed == 0 ? 5 : setTimeSpeed;
      if(not (cmd.hour == getCurrentHour() && cmd.minute == getCurrentMin())){
        if (!hasLastCmd || !isSameMotorCommand(cmd, lastCmd)) {
          xQueueSend(motorCommandQueue, &cmd, portMAX_DELAY);
          lastCmd = cmd;
          hasLastCmd = true;
        }
      }
      break;

    case 16 ... 20:
      // Spin Forward in Time
      cmd.type = SPIN_CONTINUOUS;
      cmd.direction = true;
      cmd.proportional = true;
      if (!hasLastCmd || !isSameMotorCommand(cmd, lastCmd)) {
        xQueueSend(motorCommandQueue, &cmd, portMAX_DELAY);
        lastCmd = cmd;
        hasLastCmd = true;
      }
      break;
    case 21 ... 25:
      // Spin Backward in Time
      cmd.type = SPIN_CONTINUOUS;
      cmd.direction = false;
      cmd.proportional = true;
      if (!hasLastCmd || !isSameMotorCommand(cmd, lastCmd)) {
        xQueueSend(motorCommandQueue, &cmd, portMAX_DELAY);
        lastCmd = cmd;
        hasLastCmd = true;
      }
      break;
    case 26 ... 30:
      // Real Minute Advance
      cmd.type = MIN_ADVANCE;
      if (!hasLastCmd || !isSameMotorCommand(cmd, lastCmd)) {
        xQueueSend(motorCommandQueue, &cmd, portMAX_DELAY);
        lastCmd = cmd;
        hasLastCmd = true;
      }
      break;
    case 31 ... 35:
      // Real Time Clock Mode
      cmd.type = RTC_MODE;
      if (!hasLastCmd || !isSameMotorCommand(cmd, lastCmd)) {
        xQueueSend(motorCommandQueue, &cmd, portMAX_DELAY);
        lastCmd = cmd;
        hasLastCmd = true;
      }
      break;
    case 36 ... 255:
      // Reset to 12:00
      cmd.type = MOVE_TO_HOME;
      if(not (getCurrentHour() == 12 && getCurrentMin() == 0)){
        if (!hasLastCmd || !isSameMotorCommand(cmd, lastCmd)) {
          xQueueSend(motorCommandQueue, &cmd, portMAX_DELAY);
          lastCmd = cmd;
          hasLastCmd = true;
        }
      }
      break;
  }


  // Channel 5-8: LED Intensity, RGB Control
  int Intensity = data[5 + dmxAddress];
  int red = data[6 + dmxAddress];
  int green = data[7 + dmxAddress];
  int blue = data[8 + dmxAddress];
  setLEDColor(Intensity, red, green, blue);
}

bool isSameMotorCommand(const MotorCommand &a, const MotorCommand &b) {
  if (a.type != b.type) return false;

  switch (a.type) {
    case SET_TIME:
    case SET_POSITION:
      return a.hour == b.hour && a.minute == b.minute;

    case SPIN_CONTINUOUS:
      return a.speed == b.speed &&
             a.direction == b.direction &&
             a.proportional == b.proportional;

    case MIN_ADVANCE:
    case RTC_MODE:
      return true;

    default:
      return true;
  }
}