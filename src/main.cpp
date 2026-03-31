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
#elif defined(DRIVER_TWAI)
#include "drivers/twai_driver.h"
#ifndef TWAI_TX_PIN
#define TWAI_TX_PIN GPIO_NUM_5
#endif
#ifndef TWAI_RX_PIN
#define TWAI_RX_PIN GPIO_NUM_4
#endif
#else
#error "Define DRIVER_MCP2515, DRIVER_SAME51, or DRIVER_TWAI in build_flags"
#endif

void setup()
{
#ifdef DRIVER_MCP2515
    appSetup<MCP2515Driver>(std::make_unique<MCP2515Driver>(PIN_CAN_CS), "MCP25625 ready @ 500k");
#elif defined(DRIVER_SAME51)
    appSetup<SAME51Driver>(std::make_unique<SAME51Driver>(), "SAME51 CAN ready @ 500k");
#elif defined(DRIVER_TWAI)
    appSetup<TWAIDriver>(std::make_unique<TWAIDriver>(TWAI_TX_PIN, TWAI_RX_PIN), "ESP32 TWAI ready @ 500k");
#endif
}

void loop()
{
#ifdef DRIVER_MCP2515
    appLoop<MCP2515Driver>();
#elif defined(DRIVER_SAME51)
    appLoop<SAME51Driver>();
#elif defined(DRIVER_TWAI)
    appLoop<TWAIDriver>();
#endif
}
