# Tesla Open Can Mod

[Community Discord](https://discord.gg/ZTQKAUTd2F)

An open-source general-purpose CAN bus modification tool for Tesla vehicles. While FSD enablement was the starting point, the goal is to expose and control everything accessible via CAN — as a fully open project.

Some sellers charge up to 500 € for a solution like this. The hardware costs around 20 €, and even with labor factored in, a fair price is no more than 50 €. This project exists so nobody has to overpay.
> [!WARNING] Ban  
>**Any attempt to bypass the purchase or subscription requirement for Full Self-Driving (FSD) will result in a permanent ban from Tesla services.**
>
>**FSD is a premium feature and must be properly purchased or subscribed to.**


> [!WARNING]
> **This project is for testing and educational purposes only.** Sending incorrect CAN bus messages to your vehicle can cause unexpected behavior, disable safety-critical systems, or permanently damage electronic components. The CAN bus controls everything from braking and steering to airbags — a malformed message can have serious consequences. If you don't fully understand what you're doing, **do not install this on your car**.

> [!CAUTION]
> **Use this project at your own risk.** This project is intended for testing purposes only and for use on **private property**. Modifying CAN bus messages on a vehicle can lead to unexpected or dangerous behavior.
>
> The authors accept no responsibility for any damage to your vehicle, injury, or legal consequences resulting from the use of this software. This project may void your vehicle warranty and **may not comply with road safety regulations in your jurisdiction**.
>
> For any use beyond private testing, you are responsible for complying with all applicable local laws and regulations. Always keep your hands on the wheel and stay attentive while driving.

## Prerequisites

**You must have an active FSD package on the vehicle** — either purchased or subscribed. This board enables the FSD functionality on the CAN bus level, but the vehicle still needs a valid FSD entitlement from Tesla.

If FSD subscriptions are not available in your region, you can work around this by:

1. Creating a Tesla account in a region where FSD subscriptions are offered (e.g. Canada).
2. Transferring the vehicle to that account.
3. Subscribing to FSD through that account.

This allows you to activate an FSD subscription from anywhere in the world.

## What It Does

This firmware runs on an Adafruit Feather with CAN bus support (RP2040 CAN with MCP2515, M4 CAN Express with native ATSAME51 CAN, or any ESP32 board with a built-in TWAI peripheral). It intercepts specific CAN bus messages to enable and configure Full Self-Driving (FSD). Additionally, ASS (Actually Smart Summon) is no longer restricted by EU regulations.

Core Function
- Intercepts specific CAN bus messages
- Re-transmits them onto the vehicle bus


FSD Activation Logic
- Listens for Autopilot-related CAN frames
- Checks if "Traffic Light and Stop Sign Control" is enabled in the Autopilot settings Uses this setting as a trigger for Full Self-Driving (FSD)
- Adjusts the required bits in the CAN message to activate FSD

Additional Behavior
- Reads the follow-distance stalk setting
- Maps it dynamically to a speed profile

HW4 - FSD V14 Features
- Approaching Emergency Vehicle Detection

### Supported Hardware Variants

Select your vehicle hardware variant in `RP2040CAN.ino` via the `#define` directive. Arduino IDE and PlatformIO both use the same vehicle and feature defines from that file.

| Define   | Target           | Listens on CAN IDs | Notes |
|----------|------------------|---------------------|-------|
| `LEGACY` | HW3 Retrofit     | 1006, 69            | Sets FSD enable bit and speed profile control via follow distance |
| `HW3`    | HW3 vehicles     | 1016, 1021          | Same functionality as legacy |
| `HW4`    | HW4 vehicles     | 1016, 1021          | Extended speed-profile range (5 levels) |

> [!NOTE]
> HW4 vehicles on firmware **2026.2.9.X** are on **FSD v14**. However, versions on the **2026.8.X** branch are still on **FSD v13**. If your vehicle is running FSD v13 (including the 2026.8.X branch or anything older than 2026.2.9), compile with `HW3` even if your vehicle has HW4 hardware.

### How to Determine Your Hardware Variant

- **Legacy** — Your vehicle has a **portrait-oriented center screen** and **HW3**. This applies to older (pre Palladium) Model S and Model X vehicles that originally came with or were retrofitted with HW3.
- **HW3** — Your vehicle has a **landscape-oriented center screen** and **HW3**. You can check your hardware version under **Controls → Software → Additional Vehicle Information** on the vehicle's touchscreen.
- **HW4** — Same as above, but the Additional Vehicle Information screen shows **HW4**.

### Key Behaviour

- **FSD enable bit** is set when **"Traffic Light and Stop Sign Control"** is enabled in the vehicle's Autopilot settings.
- **Speed profile** is derived from the scroll-wheel offset or follow-distance setting.
- **Nag suppression** — clears the hands-on-wheel nag bit.
- Debug output is printed over Serial at 115200 baud when `enablePrint` is `true`.

### CAN Message Details

The table below shows exactly which CAN messages each hardware variant monitors and what modifications are made.

#### Legacy (HW3 Retrofit)

| CAN ID | Name | R/W | Mux | Bit | Value | Signal | Description |
|---|---|---|---|---|---|---|---|
| 69 | STW_ACTN_RQ | R | — | 13–15 | (0–7) | | read stalk |
| 1006 | — | R+W | 0 | 38 | (0/1) | | read FSD |
| 1006 | — | R+W | 0 | 46 | 1 | | enable FSD |
| 1006 | — | R+W | 0 | 49–50 | (0–2) | | inject profile |
| 1006 | — | R+W | 1 | 19 | 0 | | suppress nag |

#### HW3

| CAN ID | Name | R/W | Mux | Bit | Value | Signal | Description |
|---|---|---|---|---|---|---|---|
| 1016 | UI_driverAssistControl | R | — | 45–47 | (0–7) | UI_accFollowDistanceSetting | read distance |
| 1021 | UI_autopilotControl | R+W | 0 | 38 | (0/1) | UI_fsdStopsControlEnabled | read FSD |
| 1021 | UI_autopilotControl | R+W | 0 | 25–30 | (offset) | | read offset |
| 1021 | UI_autopilotControl | R+W | 0 | 46 | 1 | | enable FSD |
| 1021 | UI_autopilotControl | R+W | 0 | 49–50 | (0–2) | | inject profile |
| 1021 | UI_autopilotControl | R+W | 1 | 19 | 0 | UI_applyEceR79 | suppress nag |
| 1021 | UI_autopilotControl | R+W | 2 | 6–7, 8–13 | (offset) | | inject offset |

#### HW4

| CAN ID | Name | R/W | Mux | Bit | Value | Signal | Description |
|---|---|---|---|---|---|---|---|
| 921 | DAS_status | R+W | — | 13 | 1 | DAS_suppressSpeedWarning | suppress chime |
| 921 | DAS_status | R+W | — | 56–63 | (checksum) | DAS_statusChecksum | update checksum |
| 1016 | UI_driverAssistControl | R | — | 45–47 | (0–7) | UI_accFollowDistanceSetting | read distance |
| 1021 | UI_autopilotControl | R+W | 0 | 38 | (0/1) | UI_fsdStopsControlEnabled | read FSD |
| 1021 | UI_autopilotControl | R+W | 0 | 46 | 1 | | enable FSD |
| 1021 | UI_autopilotControl | R+W | 0 | 59 | 1 | | enable detection |
| 1021 | UI_autopilotControl | R+W | 0 | 60 | 1 | | enable V14 |
| 1021 | UI_autopilotControl | R+W | 1 | 19 | 0 | UI_applyEceR79 | suppress nag |
| 1021 | UI_autopilotControl | R+W | 1 | 47 | 1 | UI_hardCoreSummon | enable summon |
| 1021 | UI_autopilotControl | R+W | 2 | 60–62 | (0–4) | | inject profile |

> Signal names sourced from [tesla-can-explorer](https://github.com/mikegapinski/tesla-can-explorer) by [@mikegapinski](https://x.com/mikegapinski).

## Supported Boards

| Board                                                                   | CAN Interface              | Library                      | Status                             |
|-------------------------------------------------------------------------|----------------------------|------------------------------|------------------------------------|
| Adafruit Feather RP2040 CAN                                             | MCP2515 over SPI           | `mcp2515.h` (autowp)         | Tested                             |
| Adafruit Feather M4 CAN Express (ATSAME51)                              | Native MCAN peripheral     | `Adafruit_CAN` (`CANSAME5x`) | Tested                             |
| ESP32 with CAN transceiver (e.g. ESP32-DevKitC + SN65HVD230)            | Native TWAI peripheral     | ESP-IDF `driver/twai.h`      | Compiles, needs on-vehicle testing |
| [Atomic CAN Base](https://docs.m5stack.com/en/atom/Atomic%20CAN%20Base) | CA-IS3050G over ESP32 TWAI | ESP32 TWAI                   | Tested                             | 

## Hardware Requirements

- One of the supported boards listed above
- CAN bus connection to the vehicle (500 kbit/s)

**Feather RP2040 CAN** — the board must expose these pins (defined by the earlephilhower board variant):
  - `PIN_CAN_CS` — SPI chip-select for the MCP2515
  - `PIN_CAN_INTERRUPT` — interrupt pin (unused; polling mode)
  - `PIN_CAN_STANDBY` — CAN transceiver standby control
  - `PIN_CAN_RESET` — MCP2515 hardware reset

**Feather M4 CAN Express** — uses the native ATSAME51 CAN peripheral; requires:
  - `PIN_CAN_STANDBY` — CAN transceiver standby control
  - `PIN_CAN_BOOSTEN` — 3V→5V boost converter enable for CAN signal levels

**ESP32 with CAN transceiver** — uses the native TWAI peripheral; requires:
  - An external CAN transceiver module (e.g. SN65HVD230, TJA1050, or MCP2551)
  - `TWAI_TX_PIN` — GPIO connected to the transceiver TX pin (default `GPIO_NUM_5`)
  - `TWAI_RX_PIN` — GPIO connected to the transceiver RX pin (default `GPIO_NUM_4`)

> [!IMPORTANT]
> Cut the onboard 120 Ω termination resistor on the Feather CAN board (jumper labeled **TERM** on RP2040, **Trm** on M4). If using an ESP32 with an external transceiver that has a termination resistor, remove or disable it as well. The vehicle's CAN bus already has its own termination, and adding a second resistor will cause communication errors.

## Installation

### Option A: Arduino IDE — Flash Only

*Recommended if you just want to flash the firmware onto your board. No command-line tools required.*

#### 1. Install the Arduino IDE

Download from [https://www.arduino.cc/en/software](https://www.arduino.cc/en/software).

#### 2. Add the Board Package

**For Feather RP2040 CAN:**
1. Open **File → Preferences**.
2. In **Additional Board Manager URLs**, add:
   ```
   https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
   ```
3. Go to **Tools → Board → Boards Manager**, search for **Raspberry PI Pico/RP2040**, and install it.
4. Select **Adafruit Feather RP2040 CAN** as the Board.

**For Feather M4 CAN Express:**
1. In **Additional Board Manager URLs**, add:
   ```
   https://adafruit.github.io/arduino-board-index/package_adafruit_index.json
   ```
2. Install **Adafruit SAMD Boards** from the Boards Manager.
3. Select **Feather M4 CAN (SAME51)** as the Board.
4. Install the **Adafruit CAN** library via the Library Manager.

**For ESP32 boards:**
1. In **Additional Board Manager URLs**, add:
   ```
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```
2. Install **esp32 by Espressif Systems** from the Boards Manager.
3. Select your ESP32 board (e.g. **ESP32 Dev Module**).

#### 3. Install Required Libraries

Install via **Sketch → Include Library → Manage Libraries…**:

- **Feather RP2040 CAN:** MCP2515 by autowp
- **Feather M4 CAN Express:** Adafruit CAN
- **ESP32:** No additional libraries needed — the TWAI driver is built into the ESP32 Arduino core.

#### 4. Select Your Board and Vehicle

Near the top of `RP2040CAN.ino`, uncomment the line that matches your **board**:

```cpp
#define DRIVER_MCP2515   // Adafruit Feather RP2040 CAN (MCP2515 over SPI)
//#define DRIVER_SAME51  // Adafruit Feather M4 CAN Express (native ATSAME51 CAN)
//#define DRIVER_TWAI    // ESP32 boards with built-in TWAI (CAN) peripheral
```

Then uncomment the line that matches your **vehicle**:

```cpp
#define LEGACY // HW4, HW3, or LEGACY
//#define HW3
//#define HW4
```

#### 5. Upload

1. Connect the Feather via USB.
2. Select the correct board and port under **Tools**.
3. Click **Upload**.

### Option B: PlatformIO — Development, Testing & Flash

*For developers who want to run unit tests, build for multiple boards, or integrate with CI. Can also flash firmware to the board.*

#### Prerequisites (Windows)

| Tool | Purpose | Install |
|------|---------|---------|
| [Python 3](https://www.python.org/downloads/) | PlatformIO runtime | `winget install Python.Python.3.14` |
| [PlatformIO CLI](https://docs.platformio.org/en/latest/core/installation.html) | Build system & test runner | `pip install platformio` |
| [MinGW-w64 GCC](https://winlibs.com/) | Native test compiler | `winget install BrechtSanders.WinLibs.POSIX.UCRT` |

> [!TIP]
> After installing MinGW-w64, restart your terminal so `gcc` and `g++` are on PATH. GCC is only needed for `pio test -e native` (host-side unit tests) — cross-compiling to the Feather boards uses PlatformIO's built-in ARM toolchain.

#### Build

1. Select your board, vehicle, and optional features near the top of `RP2040CAN.ino`:
   ```cpp
   #define DRIVER_MCP2515  // Change to DRIVER_SAME51 or DRIVER_TWAI for other boards
   #define LEGACY          // Change to HW4, HW3, or LEGACY
   #define EMERGENCY_VEHICLE_DETECTION  // Optional
   ```
   PlatformIO reads the active vehicle and optional feature defines from `RP2040CAN.ino`.
   The `-e` environment still selects the board, so it must match the uncommented driver define in the sketch. If they do not match, the build stops with a clear error.

2. Build for your board:
   ```bash
   # Adafruit Feather RP2040 CAN
   pio run -e feather_rp2040_can

   # Adafruit Feather M4 CAN Express (ATSAME51)
   pio run -e feather_m4_can

   # ESP32 with TWAI (CAN) peripheral
   pio run -e esp32_twai

   # M5Stack Atomic CAN Base
   pio run -e m5stack-atomic-can-base
   ```

#### Flash

Connect the board via USB, then upload:

```bash
# Adafruit Feather RP2040 CAN
pio run -e feather_rp2040_can --target upload

# Adafruit Feather M4 CAN Express (ATSAME51)
pio run -e feather_m4_can --target upload

# ESP32
pio run -e esp32_twai --target upload

# M5Stack Atomic CAN Base
pio run -e m5stack-atomic-can-base --target upload
```

> [!TIP]
> For Feather boards, if the board is not detected, double-press the **Reset** button to enter the UF2 bootloader, then retry the upload command. For ESP32 boards, hold the **BOOT** button during upload if auto-reset does not work.

#### Run Tests

Unit tests run on your host machine — no hardware required:

```bash
pio test -e native
```

### Wiring

The recommended connection point is the [**X179 connector**](https://service.tesla.com/docs/Model3/ElectricalReference/prog-233/connector/x179/):

| Pin | Signal |
|-----|--------|
| 13  | CAN-H  |
| 14  | CAN-L  |

Connect the Feather's CAN-H and CAN-L lines to pins 13 and 14 on the X179 connector.

The recommended connection point for **legacy Model 3 (2020 and earlier)** is the [**X652 connector**](https://service.tesla.com/docs/Model3/ElectricalReference/prog-187/connector/x652/) if the vehicle is not equipped with the X179 port (varies depending on production date):

| Pin | Signal |
|-----|--------|
| 1  | CAN-H  |
| 2  | CAN-L  |

Connect the Feather's CAN-H and CAN-L lines to pins 1 and 2 on the X652 connector.

## Speed Profiles

The speed profile controls how aggressively the vehicle drives under FSD. It is configured differently depending on the hardware variant:


### Legacy, HW3 & HW4 Profiles



| Distance | Profile (HW3) | Profile (HW4) |
| :--- | :--- | :--- |
| 2 | Hurry | Max |
| 3 | Normal | Hurry |
| 4 | Chill | Normal |
| 5 | — | Chill |
| 6 | — | Sloth |

## Serial Monitor

Open the Serial Monitor at **115200 baud** to see live debug output showing FSD state and the active speed profile. Disable logging by setting `enablePrint = false`.

## Board Porting Notes

The project uses an abstract `CanDriver` interface so that all vehicle logic (handlers, bit manipulation, speed profiles) is shared across boards. Only the driver implementation changes.

**What changes per board:**
- **RP2040 CAN:** `mcp2515.h` (autowp) — SPI-based, struct read/write, needs `PIN_CAN_CS`
- **M4 CAN Express:** `Adafruit_CAN` (`CANSAME5x`) — native MCAN peripheral, packet-stream API, needs `PIN_CAN_BOOSTEN`
- **ESP32 TWAI:** ESP-IDF `driver/twai.h` — native TWAI peripheral, FreeRTOS queue-based RX, needs an external CAN transceiver and two GPIO pins

**What stays identical:**
- All handler structs and bit manipulation logic
- Vehicle-specific behavior (FSD enable, nag suppression, speed profiles)
- Serial debug output

## Development & Testing

The project uses [PlatformIO](https://platformio.org/) with the [Unity](https://github.com/ThrowTheSwitch/Unity) test framework.

### Project Structure

```
include/
  can_frame_types.h       # Portable CanFrame struct
  can_driver.h            # Abstract CanDriver interface
  can_helpers.h           # setBit, readMuxID, isFSDSelectedInUI, setSpeedProfileV12V13
  handlers.h              # CarManagerBase, LegacyHandler, HW3Handler, HW4Handler
  app.h                   # Shared setup/loop logic for all entry points
  drivers/
    mcp2515_driver.h      # MCP2515 driver (Feather RP2040 CAN)
    same51_driver.h       # CANSAME5x driver (Feather M4 CAN Express)
    twai_driver.h         # ESP32 TWAI driver
    mock_driver.h         # Mock driver for unit tests
src/
  main.cpp                # PlatformIO entry point
test/
  test_native_helpers/    # Tests for bit manipulation helpers
  test_native_legacy/     # LegacyHandler tests
  test_native_hw3/        # HW3Handler tests
  test_native_hw4/        # HW4Handler tests
  test_native_twai/       # TWAI filter computation tests
RP2040CAN.ino             # Arduino IDE entry point (uses same headers)
```

### Running Tests

```bash
pio test -e native
```

Tests run on your host machine — no hardware required. They cover all handler logic including FSD activation, nag suppression, speed profile mapping, and bit manipulation correctness.



## Third-Party Libraries

This project depends on the following open-source libraries. Their full license texts are in [THIRD_PARTY_LICENSES](THIRD_PARTY_LICENSES).

| Library | License | Copyright |
|---------|---------|-----------|
| [autowp/arduino-mcp2515](https://github.com/autowp/arduino-mcp2515) | MIT | (c) 2013 Seeed Technology Inc., (c) 2016 Dmitry |
| [adafruit/Adafruit_CAN](https://github.com/adafruit/Adafruit_CAN) | MIT | (c) 2017 Sandeep Mistry |
| [espressif/esp-idf](https://github.com/espressif/esp-idf) (TWAI driver) | Apache 2.0 | (c) 2015-2025 Espressif Systems (Shanghai) CO LTD |

## License

This project is licensed under the **GNU General Public License v3.0** — see the [GPL-3.0 License](https://www.gnu.org/licenses/gpl-3.0.html) for details.
