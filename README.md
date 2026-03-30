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

This firmware runs on an Adafruit CAN Bus FeatherWing (MCP25625/MCP2515-based). It intercepts specific CAN bus messages to enable and configure Full Self-Driving (FSD).Additionally, ASS (Actually Smart Summon) is no longer restricted by EU regulations.

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
   https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
   ```
3. Go to **Tools → Board → Boards Manager**, search for **Raspberry PI Pico/RP2040** (or the appropriate family for your Feather), and install it.
4. Select **Adafruit Feather RP2040 CAN** as the Board.

### 3. Install Required Libraries

Install the following library via **Sketch → Include Library → Manage Libraries…** or the Arduino Library Manager:

- **MCP2515** by autowp — CAN controller driver (`mcp2515.h`)

### 4. Select Your Hardware Target

Near the top of `RP2040CAN.ino`, change the `HW` define to match your vehicle:

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


The recommended connection point for **legacy Model 3 (2020 and earlier)** is the [**X652 connector**](https://service.tesla.com/docs/Model3/ElectricalReference/prog-187/connector/x652/) if the vehicle is not equipped with the X179 port (varies depending on production date):
| Pin | Signal |
|-----|--------|
| 1  | CAN-H  |
|  2  | CAN-L  |

Connect the Feather's CAN-H and CAN-L lines to pins 1 and 2 on the X652 connector.




**Important:** Cut the onboard 120 Ω termination resistor on the Feather CAN board. The vehicle's CAN bus already has its own termination, and adding a second resistor will cause communication errors.

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
