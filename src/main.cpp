/*
    PlatformIO entry point.
    This file is the PlatformIO equivalent of RP2040CAN.ino.
    Logic is in the shared headers under include/.
*/

#include <Arduino.h>
#include "app.h"

#ifdef DRIVER_MCP2515
#include <SPI.h>
#include "drivers/mcp2515_driver.h"
#elif defined(DRIVER_SAME51)
#include "drivers/same51_driver.h"
#else
#error "Define DRIVER_MCP2515 or DRIVER_SAME51 in build_flags"
#endif

void setup() {
#ifdef DRIVER_MCP2515
    appSetup(std::make_unique<MCP2515Driver>(PIN_CAN_CS), "MCP25625 ready @ 500k");
#elif defined(DRIVER_SAME51)
    appSetup(std::make_unique<SAME51Driver>(), "SAME51 CAN ready @ 500k");
#endif
}

void loop() { appLoop(); }
