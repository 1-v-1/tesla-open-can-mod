/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

// ── Board selection ──────────────────────────────────────────────
// Uncomment ONE of the following lines to match your board:
#define DRIVER_MCP2515   // Adafruit Feather RP2040 CAN (MCP2515 over SPI)
//#define DRIVER_SAME51  // Adafruit Feather M4 CAN Express (native ATSAME51 CAN)
//#define DRIVER_TWAI    // ESP32 boards with built-in TWAI (CAN) peripheral

// ── Vehicle hardware selection ───────────────────────────────────
// Uncomment ONE of the following lines to match your vehicle:
#define LEGACY // HW4, HW3, or LEGACY
//#define HW3
//#define HW4

#include "include/app.h"

#if defined(DRIVER_MCP2515)
#include <SPI.h>
#include <mcp2515.h>
#include "include/drivers/mcp2515_driver.h"
#elif defined(DRIVER_SAME51)
#include "include/drivers/same51_driver.h"
#elif defined(DRIVER_TWAI)
#include "include/drivers/twai_driver.h"
#define TWAI_TX_PIN GPIO_NUM_5
#define TWAI_RX_PIN GPIO_NUM_4
#else
#error "Uncomment DRIVER_MCP2515, DRIVER_SAME51, or DRIVER_TWAI at the top of this file"
#endif

void setup() {
#if defined(DRIVER_MCP2515)
    appSetup<MCP2515Driver>(std::make_unique<MCP2515Driver>(PIN_CAN_CS), "MCP25625 ready @ 500k");
#elif defined(DRIVER_SAME51)
    appSetup<SAME51Driver>(std::make_unique<SAME51Driver>(), "SAME51 CAN ready @ 500k");
#elif defined(DRIVER_TWAI)
    appSetup<TWAIDriver>(std::make_unique<TWAIDriver>(TWAI_TX_PIN, TWAI_RX_PIN), "ESP32 TWAI ready @ 500k");
#endif
}

__attribute__((optimize("O3"))) void loop() {
#if defined(DRIVER_MCP2515)
    appLoop<MCP2515Driver>();
#elif defined(DRIVER_SAME51)
    appLoop<SAME51Driver>();
#elif defined(DRIVER_TWAI)
    appLoop<TWAIDriver>();
#endif
}
