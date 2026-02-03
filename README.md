# ESP32-S3 Laser Harp Controller

This project implements a frameless Laser Harp controller using the ESP32-S3. It drives an ILDA-compliant laser projector and detects hand interruptions using a photodiode, generating MIDI notes over Bluetooth LE.

## Features
*   **Core 1**: High-speed (30 kpps) vector graphics engine using DMA-SPI for DACs (MCP4922).
*   **Core 0**: WiFi Captive Portal for configuration and BLE MIDI Server.
*   **Input**: Interrupt-based photodiode detection with low latency.
*   **Simulation Mode**: Debug output without hardware.

## Hardware Design
The `hardware/` directory contains KiCad 7.0 design files.
*   `laser_harp.kicad_sch`: The schematic (generated).
*   `laser_harp.kicad_pcb`: The PCB board file (component placement only).
*   `BOM.csv`: Bill of Materials.

### Critical Components
*   **MCU**: ESP32-S3-WROOM-1 (N16R8)
*   **DACs**: 3x MCP4922 (12-bit, Dual Channel)
*   **OpAmps**: TL074 (Output), OPA358 (Sensor TIA)
*   **Power**: B0512LS-1WR3 (5V -> +/-12V Isolated)

### Wiring
Refer to `docs/DESIGN.md` for the detailed pinout and architecture description.

## Firmware Build Instructions

### Prerequisites
*   ESP-IDF v5.0 or later.
*   Python 3.

### Build & Flash
1.  Set the target:
    ```bash
    idf.py set-target esp32s3
    ```
2.  Configure (Optional):
    ```bash
    idf.py menuconfig
    ```
    *   Navigate to **Laser Harp Configuration** to enable Simulation Mode or change WiFi settings.
3.  Build:
    ```bash
    idf.py build
    ```
4.  Flash:
    ```bash
    idf.py -p /dev/ttyUSB0 flash monitor
    ```

## PCB Manufacturing Instructions
**IMPORTANT**: The provided `.kicad_pcb` file contains the correct footprint placement but **NO ROUTING**.

1.  Open `hardware/laser_harp.kicad_pro` in KiCad 7.0+.
2.  Open the PCB Editor (`.kicad_pcb`).
3.  **Route the Board**: You must manually draw the copper tracks connecting the components. Follow the "Ratsnest" (thin lines indicating connections).
    *   **Analog Path**: Keep lines from DACs to OpAmps short.
    *   **Grounding**: Use a solid Ground Plane on the Bottom Layer.
    *   **Power**: Use thick traces (0.5mm+) for 5V and +/-12V.
4.  **DRC**: Run Design Rules Check to ensure no errors.
5.  **Export**: File -> Fabrication Outputs -> Gerbers.
6.  Upload the Gerber zip to a manufacturer (e.g., JLCPCB, PCBWay).

## Safety Warning
**LASER SAFETY**: This device controls Class 3B or Class 4 lasers.
*   Always ensure the **Interlock** loop (DB25 Pins 4 & 17) is handled correctly.
*   Never look directly into the beam.
*   The firmware includes a basic watchdog, but hardware failsafes (Scan Fail Monitor) are recommended for high-power systems.
