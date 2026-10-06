#ifndef DMXHANDLER_H
#define DMXHANDLER_H

#include <Arduino.h>
#include "esp_dmx.h"

#define TX_PIN 17
#define RX_PIN 16
#define RTS_PIN 4
#define DMX_PACKET_SIZE 512
#define DMX_TIMEOUT_TICK 1000


extern byte data[DMX_PACKET_SIZE];
extern bool dmxIsConnected;
extern unsigned long lastUpdate;
extern dmx_port_t dmxPort;
extern dmx_config_t config;
extern dmx_personality_t personalities[];

// Function declarations
void dmxHandler(void* pvParameters);
void processDMXChannels();

#endif // DMXHANDLER_H


// Oversized Analog Clock DMX Control Profile Overview

// Channel 1: Preset Clock Modes

    // 0-5: Stop Hands — Stops all movement of clock hands.

    // 6-10: Set time toggle — With this set, channels 3-4 will make the clock travel to the set time like a normal clock would. Speed is

    // 11-15: Set position toggle — With this set, channels 3-4 will make the clock travel to the set time via the shortest path.
    
    // 16-20: Spin Forward in Time — Hands continuously move forward, with the speed set via channel 2
    
    // 21-25: Spin Backward in Time — Hands continuously move backward, with 130 being the slowest and 249 the fastest speed.

    // 26-30: Real Minute Advance — Starts advancing the time by one min every min.

    // 31-35: Real Time Clock Mode — Synchronizes with real-world time.

    // 36-255: Reset to 12:00 — Resets the clock hands to the 12 o'clock position.

//Channel 2 : Clock Speed — Changes how quickly the clock will move to the new time, or in spinning mode

    // 0: Default Speed (5)

    // 1-255: Speed (1-100) 
    // 1: Fastest

    // 100: Medium..ist

    // 255: Slowest

// Channel 3-4: Clock Position (16-bit Control) — 5-minute intervals with 455 steps per interval

    // 0-454: 12:00

    // 455-909: 12:05

    // 910-1364: 12:10

    // 1365-1819: 12:15

    // 1820-2274: 12:20

    // 2275-2729: 12:25

    // 2730-3184: 12:30

    // 3185-3639: 12:35

    // 3640-4094: 12:40

    // 4095-4549: 12:45

    // 4550-5004: 12:50

    // 5005-5459: 12:55

    // ...

    // 32768-33222: 6:00

    // ...

    // 65080-65535: 11:55

// Channel 5-8: RGB LED Control

    // Channel 5: LED Intensity Master 0-255: Controls the overall brightness of all LEDs (0 for off, 255 for max brightness).

    // Channel 6 (Red): 0-255 Red Led Intensity

    // Channel 7 (Green): 0-255 Green Led Intensity

    // Channel 8 (Blue): 0-255 Blue Led Intensity


