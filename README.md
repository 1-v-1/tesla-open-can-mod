# CanFeather – Tesla FSD CAN Bus Enabler

> **Why is this public?** Some sellers — such as Michal Gapinski — charge up to 500 € for a solution like this. In our opinion, that is massively overpriced. The board costs around 20 €, and even with labor factored in, a fair price is no more than 50 €. This project exists so nobody has to overpay.

## What It Does

This firmware runs on an **Adafruit CAN Bus FeatherWing** (MCP25625/MCP2515-based) and intercepts specific CAN bus messages on a Tesla vehicle to enable and configure **Full Self-Driving (FSD)** functionality.

It listens for autopilot-related CAN frames on the vehicle's CAN bus and checks whether **"Traffic Light and Stop Sign Control"** is enabled in the vehicle's Autopilot settings. When this setting is turned on, the chip treats it as the trigger to enable **Full Self-Driving** by modifying the relevant bits in the CAN frame and re-transmitting it onto the bus. It also reads the follow-distance stalk setting and maps it to a speed profile.

### Supported Hardware Variants

The firmware supports three Tesla hardware generations, selected at compile time via the `#define HW` directive:

| Define   | Target           | Listens on CAN IDs | Notes |
|----------|------------------|---------------------|-------|
| `LEGACY` | HW3 Retrofit     | 1006                | Sets FSD enable bit and speed profile |
| `HW3`    | HW3 vehicles     | 1016, 1021          | Adds speed-offset control via follow-distance |
| `HW4`    | HW4 vehicles     | 1016, 1021          | Extended speed-profile range (5 levels) |

### Key Behaviour

- **FSD enable bit** is set when **"Traffic Light and Stop Sign Control"** is enabled in the vehicle's Autopilot settings.
- **Speed profile** is derived from the scroll-wheel offset or follow-distance setting.
- **Nag suppression** — clears the hands-on-wheel nag bit.
- Debug output is printed over Serial at 115200 baud when `enablePrint` is `true`.

## Hardware Requirements

- Adafruit Feather M4 CAN (or compatible board with MCP25625/MCP2515)
- The board must expose these pins (defined at the top of the sketch):
  - `PIN_CAN_CS` — SPI chip-select for the MCP2515
  - `PIN_CAN_INTERRUPT` — interrupt pin (unused; polling mode)
  - `PIN_CAN_STANDBY` — CAN transceiver standby control
  - `PIN_CAN_RESET` — MCP2515 hardware reset
- CAN bus connection to the vehicle (500 kbit/s)

## Installation

### 1. Install the Arduino IDE

Download from [https://www.arduino.cc/en/software](https://www.arduino.cc/en/software).

### 2. Add the Adafruit Board Package

1. Open **File → Preferences**.
2. In **Additional Board Manager URLs**, add:
   ```
   https://adafruit.github.io/arduino-board-index/package_adafruit_index.json
   ```
3. Go to **Tools → Board → Boards Manager**, search for **Adafruit SAMD** (or the appropriate family for your Feather), and install it.

### 3. Install Required Libraries

Install the following library via **Sketch → Include Library → Manage Libraries…** or the Arduino Library Manager:

- **MCP2515** by autowp — CAN controller driver (`mcp2515.h`)

### 4. Select Your Hardware Target

Near the top of `CanFeather.ino`, change the `HW` define to match your vehicle:

```cpp
#define HW HW3  // Change to LEGACY, HW3, or HW4
```

### 5. Upload

1. Connect the Feather via USB.
2. Select the correct board and port under **Tools**.
3. Click **Upload**.

### 6. Wiring

The recommended connection point is the [**X179 connector**](https://service.tesla.com/docs/Model3/ElectricalReference/prog-233/connector/x179/):

| Pin | Signal |
|-----|--------|
| 13  | CAN-H  |
| 14  | CAN-L  |

Connect the Feather's CAN-H and CAN-L lines to pins 13 and 14 on the X179 connector.

**Important:** Cut the onboard 120 Ω termination resistor on the Feather CAN board. The vehicle's CAN bus already has its own termination, and adding a second resistor will cause communication errors.

## Serial Monitor

Open the Serial Monitor at **115200 baud** to see live debug output showing FSD state and the active speed profile. Disable logging by setting `enablePrint = false`.
