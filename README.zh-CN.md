# Tesla 开放 CAN 改装

[社区 Discord](https://discord.gg/ZTQKAUTd2F)

一个用于特斯拉车辆的开源通用CAN总线改装工具。虽然FSD（完全自动驾驶）启用是起点，但目标是暴露和控制CAN可访问的所有功能——作为一个完全开放的项目。

一些卖家对这样的解决方案收费高达500欧元。硬件成本约为20欧元，即使算上人工费，合理价格也不超过50欧元。这个项目的存在是为了让任何人都不必多付钱。

> [!警告] 封禁  
>**任何试图绕过Full Self-Driving（FSD）购买或订阅要求的尝试都将导致特斯拉服务永久封禁。**
>
>**FSD是一个高级功能，必须正确购买或订阅。**

> [!警告]
> **本项目仅用于测试和教育目的。** 向车辆发送错误的CAN总线消息可能导致意外行为、禁用安全关键系统或永久损坏电子组件。CAN总线控制一切，从制动到转向再到气囊——格式错误的消息可能会产生严重后果。如果您不完全了解自己在做什么，**请勿将其安装到您的汽车上**。

> [!注意]
> **使用本项目需自担风险。** 本项目仅适用于**私人场所**的测试目的。修改车辆的CAN总线消息可能导致意外或危险行为。
>
> 作者对使用本软件造成的车辆损坏、人身伤害或法律后果不承担任何责任。本项目可能使您的车辆保修失效，并且**可能不符合您所在司法管辖区的道路安全法规**。
>
> 对于私人测试以外的任何使用，您有责任遵守所有适用的当地法律法规。驾驶时始终保持双手在方向盘上并保持警觉。

## 前提条件

**您必须在车辆上拥有有效的FSD套餐**——无论是购买的还是订阅的。该开发板在CAN总线层面启用FSD功能，但车辆仍然需要来自特斯拉的有效FSD授权。

如果您的所在地区没有FSD订阅服务，您可以通过以下方式解决：

1. 在有FSD订阅服务的地区（如加拿大）创建一个特斯拉账户。
2. 将车辆转移到该账户。
3. 通过该账户订阅FSD。

这使您可以在世界任何地方激活FSD订阅。

## 功能说明

该固件在支持CAN总线的Adafruit Feather上运行（RP2040 CAN带MCP2515、M4 CAN Express带原生ATSAME51 CAN，或任何带内置TWAI外设的ESP32开发板）。它拦截特定的CAN总线消息以启用和配置Full Self-Driving（FSD）。此外，ASS（智能召唤）不再受欧盟法规限制。

核心功能
- 拦截特定的CAN总线消息
- 将它们重新传输到车辆总线上

FSD激活逻辑
- 监听与Autopilot相关的CAN帧
- 检查Autopilot设置中是否启用了"交通信号灯和停车标志控制"
- 使用此设置作为Full Self-Driving（FSD）的触发器
- 调整CAN消息中所需的位以激活FSD

附加功能
- 读取跟随距离拨杆设置
- 将其动态映射到速度配置文件

HW4 - FSD V14功能
- 接近紧急车辆检测

### 支持的硬件变体

在`RP2040CAN.ino`中通过`#define`指令选择您的车辆硬件变体。Arduino IDE和PlatformIO都使用该文件中相同的车辆和功能定义。

| 定义     | 目标              | 监听CAN ID | 说明 |
|----------|-------------------|------------|-------|
| `LEGACY` | HW3改装           | 1006, 69   | 设置FSD启用位并通过跟随距离控制速度配置文件 |
| `HW3`    | HW3车辆           | 1016, 1021 | 与传统版本相同的功能 |
| `HW4`    | HW4车辆           | 1016, 1021 | 扩展速度配置文件范围（5级）|

> [!注意]
> 运行固件**2026.2.9.X**的HW4车辆使用的是**FSD v14**。然而，**2026.8.X**分支上的版本仍然使用FSD v13。如果您的车辆运行的是FSD v13（包括2026.8.X分支或任何早于2026.2.9的版本），即使您的车辆是HW4硬件，也请使用`HW3`编译。

### 如何确定您的硬件变体

- **Legacy** — 您的车辆有**纵向居中屏幕**和**HW3**。这适用于较旧的（原装或改装了HW3的）Model S和Model X车辆（Palladium之前）。
- **HW3** — 您的车辆有**横向居中屏幕**和**HW3**。您可以在车辆触摸屏上**控制→软件→其他车辆信息**中检查您的硬件版本。
- **HW4** — 与上述相同，但其他车辆信息屏幕显示**HW4**。

### 关键行为

- 当在车辆的Autopilot设置中启用**"交通信号灯和停车标志控制"**时，设置**FSD启用位**。
- **速度配置文件**源自滚轮偏移量或跟随距离设置。
- **提醒抑制** — 清除手扶方向盘提醒位。
- 当`enablePrint`为`true`时，调试输出以115200波特率打印到串口。

### CAN消息详情

下表显示了每个硬件变体监控的确切CAN消息以及所做的修改。

#### Legacy（HW3改装）

| CAN ID | 名称 | 读/写 | 多路复用 | 位 | 值 | 信号 | 说明 |
|---|---|---|---|---|---|---|---|
| 69 | STW_ACTN_RQ | 读 | — | 13–15 | (0–7) | | 读取拨杆 |
| 1006 | — | 读+写 | 0 | 38 | (0/1) | | 读取FSD |
| 1006 | — | 读+写 | 0 | 46 | 1 | | 启用FSD |
| 1006 | — | 读+写 | 0 | 49–50 | (0–2) | | 注入配置文件 |
| 1006 | — | 读+写 | 1 | 19 | 0 | | 抑制提醒 |

#### HW3

| CAN ID | 名称 | 读/写 | 多路复用 | 位 | 值 | 信号 | 说明 |
|---|---|---|---|---|---|---|---|
| 1016 | UI_driverAssistControl | 读 | — | 45–47 | (0–7) | UI_accFollowDistanceSetting | 读取距离 |
| 1021 | UI_autopilotControl | 读+写 | 0 | 38 | (0/1) | UI_fsdStopsControlEnabled | 读取FSD |
| 1021 | UI_autopilotControl | 读+写 | 0 | 25–30 | (偏移量) | | 读取偏移量 |
| 1021 | UI_autopilotControl | 读+写 | 0 | 46 | 1 | | 启用FSD |
| 1021 | UI_autopilotControl | 读+写 | 0 | 49–50 | (0–2) | | 注入配置文件 |
| 1021 | UI_autopilotControl | 读+写 | 1 | 19 | 0 | UI_applyEceR79 | 抑制提醒 |
| 1021 | UI_autopilotControl | 读+写 | 2 | 6–7, 8–13 | (偏移量) | | 注入偏移量 |

#### HW4

| CAN ID | 名称 | 读/写 | 多路复用 | 位 | 值 | 信号 | 说明 |
|---|---|---|---|---|---|---|---|
| 921 | DAS_status | 读+写 | — | 13 | 1 | DAS_suppressSpeedWarning | 抑制提示音 |
| 921 | DAS_status | 读+写 | — | 56–63 | (校验和) | DAS_statusChecksum | 更新校验和 |
| 1016 | UI_driverAssistControl | 读 | — | 45–47 | (0–7) | UI_accFollowDistanceSetting | 读取距离 |
| 1021 | UI_autopilotControl | 读+写 | 0 | 38 | (0/1) | UI_fsdStopsControlEnabled | 读取FSD |
| 1021 | UI_autopilotControl | 读+写 | 0 | 46 | 1 | | 启用FSD |
| 1021 | UI_autopilotControl | 读+写 | 0 | 59 | 1 | | 启用检测 |
| 1021 | UI_autopilotControl | 读+写 | 0 | 60 | 1 | | 启用V14 |
| 1021 | UI_autopilotControl | 读+写 | 1 | 19 | 0 | UI_applyEceR79 | 抑制提醒 |
| 1021 | UI_autopilotControl | 读+写 | 1 | 47 | 1 | UI_hardCoreSummon | 启用召唤 |
| 1021 | UI_autopilotControl | 读+写 | 2 | 60–62 | (0–4) | | 注入配置文件 |

> 信号名称来源于[tesla-can-explorer](https://github.com/mikegapinski/tesla-can-explorer)，由[@mikegapinski](https://x.com/mikegapinski)提供。

## 支持的开发板

| 开发板                                                                   | CAN接口              | 库                      | 状态                             |
|-------------------------------------------------------------------------|----------------------|-------------------------|----------------------------------|
| Adafruit Feather RP2040 CAN                                             | MCP2515 over SPI      | `mcp2515.h` (autowp)    | 已测试                           |
| Adafruit Feather M4 CAN Express (ATSAME51)                              | 原生MCAN外设          | `Adafruit_CAN` (`CANSAME5x`) | 已编译，需实车测试 |
| 带CAN收发器的ESP32（如ESP32-DevKitC + SN65HVD230）                       | 原生TWAI外设          | ESP-IDF `driver/twai.h` | 已编译，需实车测试 |
| [Atomic CAN Base](https://docs.m5stack.com/en/atom/Atomic%20CAN%20Base) | CA-IS3050G over ESP32 TWAI | ESP32 TWAI              | 已测试                           |

## 硬件要求

- 上述支持开发板之一
- 连接到车辆的CAN总线（500 kbit/s）

**Feather RP2040 CAN** — 开发板必须暴露以下引脚（由earlephilhower开发板变体定义）：
- `PIN_CAN_CS` — MCP2515的SPI片选
- `PIN_CAN_INTERRUPT` — 中断引脚（未使用；轮询模式）
- `PIN_CAN_STANDBY` — CAN收发器待机控制
- `PIN_CAN_RESET` — MCP2515硬件复位

**Feather M4 CAN Express** — 使用原生ATSAME51 CAN外设；需要：
- `PIN_CAN_STANDBY` — CAN收发器待机控制
- `PIN_CAN_BOOSTEN` — 3V→5V升压转换器启用，用于CAN信号电平

**带CAN收发器的ESP32** — 使用原生TWAI外设；需要：
- 外部CAN收发器模块（如SN65HVD230、TJA1050或MCP2551）
- `TWAI_TX_PIN` — 连接到收发器TX引脚的GPIO（默认`GPIO_NUM_5`）
- `TWAI_RX_PIN` — 连接到收发器RX引脚的GPIO（默认`GPIO_NUM_4`）

> [!重要]
> 切开Feather CAN板上的板载120Ω终端电阻（RP2040上标有**TERM**的跳线，M4上标有**Trm**）。如果使用带终端电阻的外部收发器的ESP32，也要移除或禁用它。车辆的CAN总线已有自己的终端，添加第二个电阻会导致通信错误。

## 安装

### 选项A：Arduino IDE — 仅烧录

*如果您只想将固件烧录到开发板上，建议使用此选项。无需命令行工具。*

#### 1. 安装Arduino IDE

从[https://www.arduino.cc/en/software](https://www.arduino.cc/en/software)下载。

#### 2. 添加开发板包

**对于Feather RP2040 CAN：**
1. 打开**文件→首选项**。
2. 在**附加开发板管理器URL**中，添加：
   ```
   https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
   ```
3. 转到**工具→开发板→开发板管理器**，搜索**Raspberry PI Pico/RP2040**并安装。
4. 选择**Adafruit Feather RP2040 CAN**作为开发板。

**对于Feather M4 CAN Express：**
1. 在**附加开发板管理器URL**中，添加：
   ```
   https://adafruit.github.io/arduino-board-index/package_adafruit_index.json
   ```
2. 从开发板管理器安装**Adafruit SAMD Boards**。
3. 选择**Feather M4 CAN (SAME51)**作为开发板。
4. 通过库管理器安装**Adafruit CAN**库。

**对于ESP32开发板：**
1. 在**附加开发板管理器URL**中，添加：
   ```
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```
2. 从开发板管理器安装**esp32 by Espressif Systems**。
3. 选择您的ESP32开发板（如**ESP32 Dev Module**）。

#### 3. 安装所需库

通过**草图→包含库→管理库…**安装：

- **Feather RP2040 CAN：** MCP2515 by autowp
- **Feather M4 CAN Express：** Adafruit CAN
- **ESP32：** 无需额外库——TWAI驱动已内置于ESP32 Arduino核心中。

#### 4. 选择您的开发板和车辆

在`RP2040CAN.ino`顶部附近，取消注释与您的**开发板**匹配的行：

```cpp
#define DRIVER_MCP2515   // Adafruit Feather RP2040 CAN (MCP2515 over SPI)
//#define DRIVER_SAME51  // Adafruit Feather M4 CAN Express (native ATSAME51 CAN)
//#define DRIVER_TWAI    // ESP32 boards with built-in TWAI (CAN) peripheral
```

然后取消注释与您的**车辆**匹配的行：

```cpp
#define LEGACY // HW4, HW3, or LEGACY
//#define HW3
//#define HW4
```

#### 5. 上传

1. 通过USB连接Feather。
2. 在**工具**下选择正确的开发板和端口。
3. 点击**上传**。

### 选项B：PlatformIO — 开发、测试和烧录

*适用于想要运行单元测试、构建多个开发板或与CI集成的开发人员。也可以将固件烧录到开发板上。*

#### 前置条件（Windows）

| 工具 | 用途 | 安装 |
|------|------|------|
| [Python 3](https://www.python.org/downloads/) | PlatformIO运行时 | `winget install Python.Python.3.14` |
| [PlatformIO CLI](https://docs.platformio.org/en/latest/core/installation.html) | 构建系统和测试运行器 | `pip install platformio` |
| [MinGW-w64 GCC](https://winlibs.com/) | 本地测试编译器 | `winget install BrechtSanders.WinLibs.POSIX.UCRT` |

> [!提示]
> 安装MinGW-w64后，重启终端使`gcc`和`g++`在PATH中。GCC仅用于`pio test -e native`（主机端单元测试）——交叉编译到Feather开发板使用PlatformIO内置的ARM工具链。

#### 构建

1. 在`RP2040CAN.ino`顶部附近选择您的开发板、车辆和可选功能：
   ```cpp
   #define DRIVER_MCP2515  // 更改為 DRIVER_SAME51 或 DRIVER_TWAI 以使用其他開發板
   #define LEGACY          // 更改為 HW4, HW3, 或 LEGACY
   #define EMERGENCY_VEHICLE_DETECTION  // 可选
   ```
   PlatformIO从`RP2040CAN.ino`读取活动的车辆和可选功能定义。
   `-e`环境仍然选择开发板，因此它必须与草图中未注释的驱动定义匹配。如果它们不匹配，构建将停止并显示清晰的错误。

2. 为您的开发板构建：
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

#### 烧录

通过USB连接开发板，然后上传：

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

> [!提示]
> 对于Feather开发板，如果未检测到开发板，请双击**复位**按钮进入UF2引导加载程序，然后重试上传命令。对于ESP32开发板，如果在上传时自动复位不起作用，请按住**BOOT**按钮。

#### 运行测试

单元测试在您的主机上运行——无需硬件：

```bash
pio test -e native
```

### 接线

推荐的连接点是[**X179连接器**](https://service.tesla.com/docs/Model3/ElectricalReference/prog-233/connector/x179/)：

| 引脚 | 信号 |
|------|------|
| 13  | CAN-H |
| 14  | CAN-L |

将Feather的CAN-H和CAN-L线连接到X179连接器的引脚13和14。

对于**传统Model 3（2020年及更早）**的推荐连接点（如果车辆未配备X179端口，因生产日期而异）是[**X652连接器**](https://service.tesla.com/docs/Model3/ElectricalReference/prog-187/connector/x652/)：

| 引脚 | 信号 |
|------|------|
| 1  | CAN-H |
| 2  | CAN-L |

将Feather的CAN-H和CAN-L线连接到X652连接器的引脚1和2。

## 速度配置文件

速度配置文件控制FSD下车辆的驾驶激进程度。不同硬件变体的配置方式不同：

### Legacy、HW3和HW4配置文件

| 距离 | 配置文件 (HW3) | 配置文件 (HW4) |
| :--- | :--- | :--- |
| 2 | 匆忙 | 最高 |
| 3 | 正常 | 匆忙 |
| 4 | 轻松 | 正常 |
| 5 | — | 轻松 |
| 6 | — | 缓慢 |

## 串口监视器

打开串口监视器以**115200波特**查看显示FSD状态和活动速度配置文件的实时调试输出。将`enablePrint`设置为`false`可禁用日志。

## 开发板移植说明

该项目使用抽象的`CanDriver`接口，以便所有车辆逻辑（处理程序、位操作、速度配置文件）在各开发板之间共享。只有驱动实现发生变化。

**每个开发板的变化：**
- **RP2040 CAN：** `mcp2515.h` (autowp) — 基于SPI，结构体读写，需要`PIN_CAN_CS`
- **M4 CAN Express：** `Adafruit_CAN` (`CANSAME5x`) — 原生MCAN外设，数据包流API，需要`PIN_CAN_BOOSTEN`
- **ESP32 TWAI：** ESP-IDF `driver/twai.h` — 原生TWAI外设，基于FreeRTOS队列的RX，需要外部CAN收发器和两个GPIO引脚

**保持不变的部分：**
- 所有处理程序结构和位操作逻辑
- 车辆特定行为（FSD启用、提醒抑制、速度配置文件）
- 串口调试输出

## 开发与测试

该项目使用[PlatformIO](https://platformio.org/)和[Unity](https://github.com/ThrowTheSwitch/Unity)测试框架。

### 项目结构

```
include/
  can_frame_types.h       # 可移植的CanFrame结构体
  can_driver.h            # 抽象的CanDriver接口
  can_helpers.h           # setBit、readMuxID、isFSDSelectedInUI、setSpeedProfileV12V13
  handlers.h              # CarManagerBase、LegacyHandler、HW3Handler、HW4Handler
  app.h                   # 所有入口点的共享设置/循环逻辑
  drivers/
    mcp2515_driver.h      # MCP2515驱动（Feather RP2040 CAN）
    same51_driver.h       # CANSAME5x驱动（Feather M4 CAN Express）
    twai_driver.h         # ESP32 TWAI驱动
    mock_driver.h         # 用于单元测试的模拟驱动
src/
  main.cpp                # PlatformIO入口点
test/
  test_native_helpers/    # 位操作辅助函数测试
  test_native_legacy/     # LegacyHandler测试
  test_native_hw3/        # HW3Handler测试
  test_native_hw4/        # HW4Handler测试
  test_native_twai/       # TWAI过滤器计算测试
RP2040CAN.ino             # Arduino IDE入口点（使用相同的头文件）
```

### 运行测试

```bash
pio test -e native
```

测试在您的主机上运行——无需硬件。它们涵盖所有处理程序逻辑，包括FSD激活、提醒抑制、速度配置文件映射和位操作正确性。

## 第三方库

本项目依赖以下开源库。它们的完整许可文本见[THIRD_PARTY_LICENSES](THIRD_PARTY_LICENSES)。

| 库 | 许可 | 版权 |
|---------|---------|-----------|
| [autowp/arduino-mcp2515](https://github.com/autowp/arduino-mcp2515) | MIT | (c) 2013 Seeed Technology Inc., (c) 2016 Dmitry |
| [adafruit/Adafruit_CAN](https://github.com/adafruit/Adafruit_CAN) | MIT | (c) 2017 Sandeep Mistry |
| [espressif/esp-idf](https://github.com/espressif/esp-idf) (TWAI驱动) | Apache 2.0 | (c) 2015-2025 Espressif Systems (Shanghai) CO LTD |

## 许可

本项目根据**GNU通用公共许可证v3.0**许可——详见[GPL-3.0许可证](https://www.gnu.org/licenses/gpl-3.0.html)。
