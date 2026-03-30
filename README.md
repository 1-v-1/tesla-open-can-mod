# CanFeather – Tesla FSD CAN Bus Enabler

> **Why is this public?** Some sellers charge up to 500 € for a solution like this. In our opinion, that is massively overpriced. The board costs around 20 €, and even with labor factored in, a fair price is no more than 50 €. This project exists so nobody has to overpay.

## 📌 Prerequisites

**You must have an active FSD package on the vehicle** — either purchased or subscribed. This board enables the FSD functionality on the CAN bus level, but the vehicle still needs a valid FSD entitlement from Tesla.

If FSD subscriptions are not available in your region, you can work around this by:

1. Creating a Tesla account in a region where FSD subscriptions are offered (e.g. Canada).
2. Transferring the vehicle to that account.
3. Subscribing to FSD through that account.

This allows you to activate an FSD subscription from anywhere in the world.

## 🛠️ What It Does

This firmware runs on an Adafruit Feather with CAN bus support (RP2040 CAN with MCP2515, or M4 CAN Express with native ATSAME51 CAN). It intercepts specific CAN bus messages to enable and configure Full Self-Driving (FSD). Additionally, ASS (Actually Smart Summon) is no longer restricted by EU regulations.

🚗 Core Function
- Intercepts specific CAN bus messages
- Re-transmits them onto the vehicle bus


🧠 FSD Activation Logic
- Listens for Autopilot-related CAN frames
- Checks if "Traffic Light and Stop Sign Control" is enabled in the Autopilot settings Uses this setting as a trigger for Full Self-Driving (FSD)
- Adjusts the required bits in the CAN message to activate FSD

⚙️ Additional Behavior
- Reads the follow-distance stalk setting
- Maps it dynamically to a speed profile

⚙️ HW4 - FSD V14 Features
- Approaching Emergency Vehicle Detection

### Supported Hardware Variants

Select your hardware in RP2040CAN.ino via the #define HW directive:

| Define   | Target           | Listens on CAN IDs | Notes |
|----------|------------------|---------------------|-------|
| `LEGACY` | HW3 Retrofit     | 1006, 69            | Sets FSD enable bit and speed profile control via follow distance |
| `HW3`    | HW3 vehicles     | 1016, 1021          | Same functionality as legacy |
| `HW4`    | HW4 vehicles     | 1016, 1021          | Extended speed-profile range (5 levels) |

> **Note:** HW4 vehicles on firmware **2026.2.9.X** are on **FSD v14**. However, versions on the **2026.8.X** branch are still on **FSD v13**. If your vehicle is running FSD v13 (including the 2026.8.X branch or anything older than 2026.2.9), compile with `HW3` even if your vehicle has HW4 hardware.

### How to Determine Your Hardware Variant

- **Legacy** — Your vehicle has a **portrait-oriented center screen** and **HW3**. This applies to older (pre Palladium) Model S and Model X vehicles that originally came with or were retrofitted with HW3.
- **HW3** — Your vehicle has a **landscape-oriented center screen** and **HW3**. You can check your hardware version under **Controls → Software → Additional Vehicle Information** on the vehicle's touchscreen.
- **HW4** — Same as above, but the Additional Vehicle Information screen shows **HW4**.

### Key Behaviour

- **FSD enable bit** is set when **"Traffic Light and Stop Sign Control"** is enabled in the vehicle's Autopilot settings.
- **Speed profile** is derived from the scroll-wheel offset or follow-distance setting.
- **Nag suppression** — clears the hands-on-wheel nag bit.
- Debug output is printed over Serial at 115200 baud when `enablePrint` is `true`.

## Supported Boards

| Board | CAN Interface | Library | Status |
|-------|--------------|---------|--------|
| Adafruit Feather RP2040 CAN | MCP2515 over SPI | `mcp2515.h` (autowp) | Tested |
| Adafruit Feather M4 CAN Express (ATSAME51) | Native MCAN peripheral | `Adafruit_CAN` (`CANSAME5x`) | Compiles, needs on-vehicle testing |

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

**Important:** Cut the onboard 120 Ω termination resistor on the Feather CAN board (jumper labeled **TERM** on RP2040, **Trm** on M4). The vehicle's CAN bus already has its own termination, and adding a second resistor will cause communication errors.

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

#### 3. Install Required Libraries

Install via **Sketch → Include Library → Manage Libraries…**:

- **Feather RP2040 CAN:** MCP2515 by autowp
- **Feather M4 CAN Express:** Adafruit CAN

#### 4. Select Your Board and Vehicle

Near the top of `RP2040CAN.ino`, uncomment the line that matches your **board**:

```cpp
#define DRIVER_MCP2515   // Adafruit Feather RP2040 CAN (MCP2515 over SPI)
//#define DRIVER_SAME51  // Adafruit Feather M4 CAN Express (native ATSAME51 CAN)
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

> After installing MinGW-w64, restart your terminal so `gcc` and `g++` are on PATH. GCC is only needed for `pio test -e native` (host-side unit tests) — cross-compiling to the Feather boards uses PlatformIO's built-in ARM toolchain.

#### Build

1. Select your vehicle by editing the define near the top of `src/main.cpp`:
   ```cpp
   #define LEGACY  // Change to HW4, HW3, or LEGACY
   ```
   The board is selected automatically by the PlatformIO environment (`-e feather_rp2040_can` or `-e feather_m4_can`).

2. Build for your board:
   ```bash
   # Adafruit Feather RP2040 CAN
   pio run -e feather_rp2040_can

   # Adafruit Feather M4 CAN Express (ATSAME51)
   pio run -e feather_m4_can
   ```

#### Flash

Connect the Feather via USB, then upload:

```bash
# Adafruit Feather RP2040 CAN
pio run -e feather_rp2040_can --target upload

# Adafruit Feather M4 CAN Express (ATSAME51)
pio run -e feather_m4_can --target upload
```

> **Tip:** If the board is not detected, double-press the **Reset** button to enter the UF2 bootloader, then retry the upload command.

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
| 2 | ⚡ Hurry | 🔥 Max |
| 3 | 🟢 Normal | ⚡ Hurry |
| 4 | ❄️ Chill | 🟢 Normal |
| 5 | — | ❄️ Chill |
| 6 | — | 🐢 Sloth |

## Serial Monitor

Open the Serial Monitor at **115200 baud** to see live debug output showing FSD state and the active speed profile. Disable logging by setting `enablePrint = false`.

## Feather M4 CAN Express — Porting Notes

The Adafruit Feather M4 CAN Express (ATSAME51) has a native CAN controller built into the MCU — there is no MCP2515 on this board. The CAN driver abstraction layer handles the differences transparently.

**What changes per board:**
- **Library:** RP2040 uses `mcp2515.h` (autowp); M4 uses `Adafruit_CAN` (`CANSAME5x` class)
- **CAN API:** RP2040 uses struct-based read/write (`can_frame`); M4 uses a packet-stream API (`parsePacket`/`beginPacket`/`write`/`endPacket`)
- **Pin setup:** RP2040 needs `PIN_CAN_CS` (SPI chip-select); M4 needs `PIN_CAN_BOOSTEN` (3V→5V boost enable). Only `PIN_CAN_STANDBY` and `PIN_LED` exist on both boards.

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
  drivers/
    mcp2515_driver.h      # MCP2515 driver (Feather RP2040 CAN)
    same51_driver.h       # CANSAME5x driver (Feather M4 CAN Express)
    mock_driver.h         # Mock driver for unit tests
src/
  main.cpp                # PlatformIO entry point
test/
  test_native_helpers/    # Tests for bit manipulation helpers
  test_native_legacy/     # LegacyHandler tests
  test_native_hw3/        # HW3Handler tests
  test_native_hw4/        # HW4Handler tests
RP2040CAN.ino             # Arduino IDE entry point (uses same headers)
```

### Running Tests

```bash
pio test -e native
```

Tests run on your host machine — no hardware required. They cover all handler logic including FSD activation, nag suppression, speed profile mapping, and bit manipulation correctness.

## Disclaimer

**Use this project at your own risk.** Modifying CAN bus messages on a vehicle can lead to unexpected or dangerous behavior. The authors accept no responsibility for any damage to your vehicle, injury, or legal consequences resulting from the use of this software. This project may void your vehicle warranty and may not comply with road safety regulations in your jurisdiction. Always keep your hands on the wheel and stay attentive while driving.

## License

This project is licensed under the **GNU General Public License v3.0** — see the [GPL-3.0 License](https://www.gnu.org/licenses/gpl-3.0.html) for details.

### Third-Party Licenses

This project uses the [MCP2515 library](https://github.com/autowp/arduino-mcp2515) by autowp, which is licensed under the MIT License:

> Copyright (c) 2013 Seeed Technology Inc.  
> Copyright (c) 2016 Dmitry
>
> Permission is hereby granted, free of charge, to any person obtaining a copy
> of this software and associated documentation files (the "Software"), to deal
> in the Software without restriction, including without limitation the rights
> to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
> copies of the Software, and to permit persons to whom the Software is
> furnished to do so, subject to the following conditions:
>
> The above copyright notice and this permission notice shall be included in all
> copies or substantial portions of the Software.
>
> THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
> IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
> FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
> AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
> LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
> OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
> SOFTWARE.
