# System Design & Architecture: ESP32-S3 Laser Harp

## 1. Overview
This system controls a frameless laser harp using an ESP32-S3. It drives an ILDA-compliant laser projector (via 3x MCP4922 DACs) and detects hand interruptions using a photodiode sensor.

## 2. Hardware Architecture

### 2.1 Power Distribution
*   **Input**: 5V DC (via USB-C connector).
*   **Digital Logic**: 3.3V via AMS1117-3.3 LDO (Max 800mA).
*   **Analog Laser Control**: +/- 12V via B0512LS-1WR3 DC-DC Converter (Isolated) -> LC Filters -> TL074 OpAmps.
*   **DAC Supply**: 5V (Direct from input, filtered) to maximize dynamic range.

### 2.2 Pinout Assignment (ESP32-S3)

| GPIO | Function | Description |
|---|---|---|
| **SPI Bus (FSPI)** | | Fast SPI for DACs |
| GPIO 12 | SPI_CLK | Shared Clock for all DACs |
| GPIO 11 | SPI_MOSI | Shared Data for all DACs |
| GPIO 10 | CS_XY | Chip Select for DAC U1 (X/Y Axis) |
| GPIO 9 | CS_RG | Chip Select for DAC U2 (Red/Green) |
| GPIO 14 | CS_BI | Chip Select for DAC U3 (Blue/Intensity) |
| GPIO 13 | LDAC | Global Latch for synchronous update |
| **Sensor Interface** | | |
| GPIO 4 | SENSOR_IN | Input from Comparator (Interrupt on rising/falling edge) |
| **User Interface** | | |
| GPIO 0 | BOOT_BTN | Boot button / Factory Reset |
| GPIO 2 | STATUS_LED | Onboard LED for status indication |
| GPIO 43 | UART_TX | Debug Output |
| GPIO 44 | UART_RX | Debug Input |

### 2.3 Component Map

| RefDes | Component | Function |
|---|---|---|
| U1 | MCP4922 | DAC for X (Ch A) and Y (Ch B) Axis |
| U2 | MCP4922 | DAC for Red (Ch A) and Green (Ch B) |
| U3 | MCP4922 | DAC for Blue (Ch A) and Intensity (Ch B) |
| U4 | ESP32-S3-WROOM-1 | Main Controller Module |
| U5 | TL074 | OpAmp: Diff Drivers for X and Y |
| U6 | TL074 | OpAmp: Drivers for R, G, B, I |
| U7 | OPA358 | TIA (Transimpedance Amplifier) for Sensor |
| U8 | LM393 | Comparator for Sensor Digitization |
| U9 | 74HCT125 | Level Shifter (3.3V -> 5V) for SPI signals |
| U10 | B0512LS-1WR3 | 5V to +/-12V DC-DC Converter |
| U11 | AMS1117-3.3 | 3.3V LDO Regulator |

### 2.4 Signal Chain
1.  **ESP32** sends 12-bit data via SPI to **MCP4922** (U1-U3).
2.  **MCP4922** outputs 0-4.096V (using internal or external Vref).
3.  **TL074** (U5) converts X/Y to differential +/- 5V (10Vpp) for ILDA ISP-DB25.
4.  **TL074** (U6) buffers/scales R/G/B/I to 0-5V for ILDA.
5.  **BPW34** (Sensor) receives light -> **OPA358** (TIA) converts to voltage -> **LM393** triggers at threshold -> **ESP32** GPIO 4 Interrupt.

## 3. Software Architecture

### 3.1 Task Allocation (Dual Core)

*   **Core 0 (Comms & UI)**
    *   **WiFi Task**: Manages Station/AP mode, Captive Portal (DNS), HTTP Server.
    *   **BLE Task**: Handles Bluetooth MIDI service (GATT Server).
    *   **System Task**: Monitoring, Logging, OTA Updates.
    *   *Priority*: Low/Medium.

*   **Core 1 (Real-time Control)**
    *   **Harp Engine Task**: The main loop.
        *   Calculates vector points for the laser path (Harp strings).
        *   Writes to DACs via DMA-SPI.
        *   Handles geometric corrections (Keystone).
    *   **Sensor ISR**: High-priority interrupt.
        *   Captures the exact time/index of the scan when light is detected.
        *   Notifies the Harp Engine to trigger a MIDI note.
    *   *Priority*: Real-time / High.

### 3.2 Data Structures
*   `ilda_point_t`: Struct containing {x, y, r, g, b, i}.
*   `string_config_t`: Position and note mapping for each virtual string.
*   `frame_buffer`: DMA-accessible buffer for SPI transactions.

### 3.3 Safety Mechanisms
*   **Watchdog**: If Core 1 hangs, the laser is blanked immediately.
*   **Static Beam Protection**: If the scan area is too small (beam stationary), the intensity is cut to prevent eye damage (Software implementation, secondary to hardware interlock).
