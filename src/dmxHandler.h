#ifndef DMXHANDLER_H
#define DMXHANDLER_H

#include <Arduino.h>
#include "esp_dmx.h"


void dmxHandler(void *pvParameters);
void processDMXChannels();

#endif // DMXHANDLER_H