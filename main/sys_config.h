#ifndef SYS_CONFIG_H
#define SYS_CONFIG_H

#include <sdkconfig.h>

// --- PINOUT DEFINITIONS ---

// SPI Bus (FSPI Host)
#define PIN_SPI_MOSI    11
#define PIN_SPI_CLK     12
#define PIN_SPI_MISO    -1 // Not used for DACs

// DAC Chip Selects
#define PIN_CS_XY       10 // U1
#define PIN_CS_RG       9  // U2
#define PIN_CS_BI       14 // U3

// DAC Latch (Common)
#define PIN_LDAC        13

// Sensor Input (Comparator)
#define PIN_SENSOR_IN   4

// User Interface
#define PIN_BOOT_BTN    0
#define PIN_STATUS_LED  2
#define PIN_UART_TX     43
#define PIN_UART_RX     44

// --- HARDWARE CONSTANTS ---

#define DAC_SPI_HOST    SPI2_HOST // FSPI
#define DAC_SPI_FREQ_HZ 20000000  // 20 MHz (MCP4922 max is 20MHz)

// --- TASK CONFIGURATION ---
#define TASK_CORE_COMMS 0
#define TASK_CORE_LASER 1

#endif // SYS_CONFIG_H
