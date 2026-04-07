#include "laser_engine.h"
#include "sys_config.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include <string.h>
#include <stdio.h>

static spi_device_handle_t spi_handle_xy;
static spi_device_handle_t spi_handle_rg;
static spi_device_handle_t spi_handle_bi;

#define MCP4922_CMD_A 0x7000 // Ch A, Buf, 1x, Active
#define MCP4922_CMD_B 0xF000 // Ch B, Buf, 1x, Active

void dac_init(void) {
#if CONFIG_LASER_SIMULATION_MODE
    printf("LASER_ENGINE: Simulation Mode - SPI Init Skipped\n");
    return;
#endif

    // 1. Init Bus
    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_SPI_MISO,
        .mosi_io_num = PIN_SPI_MOSI,
        .sclk_io_num = PIN_SPI_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 32
    };
    ESP_ERROR_CHECK(spi_bus_initialize(DAC_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));

    // 2. Add Devices
    // We use the SPI driver's per-device hardware CS control.
    // MCP4922 requires CS to toggle for each 16-bit word to latch data.
    // By adding 3 devices with their respective CS pins, the driver
    // handles the hardware CS signals efficiently.
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = DAC_SPI_FREQ_HZ,
        .mode = 0, // SPI mode 0
        .queue_size = 1,
    };

    // U1: XY
    devcfg.spics_io_num = PIN_CS_XY;
    ESP_ERROR_CHECK(spi_bus_add_device(DAC_SPI_HOST, &devcfg, &spi_handle_xy));

    // U2: RG
    devcfg.spics_io_num = PIN_CS_RG;
    ESP_ERROR_CHECK(spi_bus_add_device(DAC_SPI_HOST, &devcfg, &spi_handle_rg));

    // U3: BI
    devcfg.spics_io_num = PIN_CS_BI;
    ESP_ERROR_CHECK(spi_bus_add_device(DAC_SPI_HOST, &devcfg, &spi_handle_bi));

    // 3. Init LDAC
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << PIN_LDAC);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
    gpio_set_level(PIN_LDAC, 1);
}

// Internal helper to send to one DAC
static void send_dac(spi_device_handle_t handle, uint16_t val_a, uint16_t val_b) {
    // We use SPI_TRANS_USE_TXDATA to avoid DMA overhead for these small 16-bit transfers.
    // MCP4922 expects MSB first. We manually arrange bytes in tx_data to avoid bit-swapping.
    spi_transaction_t t = {
        .flags = SPI_TRANS_USE_TXDATA,
        .length = 16,
    };

    // Lock the bus for this device to perform two consecutive transactions
    // (Channel A and Channel B) without interruption.
    ESP_ERROR_CHECK(spi_device_acquire_bus(handle, portMAX_DELAY));

    // Send Channel A
    // Command format: [A/B, BUF, GA, SHDN, D11, D10, D9, D8] [D7, D6, D5, D4, D3, D2, D1, D0]
    t.tx_data[0] = (MCP4922_CMD_A >> 8) | ((val_a >> 8) & 0x0F);
    t.tx_data[1] = val_a & 0xFF;
    ESP_ERROR_CHECK(spi_device_polling_transmit(handle, &t));

    // Send Channel B
    t.tx_data[0] = (MCP4922_CMD_B >> 8) | ((val_b >> 8) & 0x0F);
    t.tx_data[1] = val_b & 0xFF;
    ESP_ERROR_CHECK(spi_device_polling_transmit(handle, &t));

    spi_device_release_bus(handle);
}

void dac_output_point(ilda_point_t p) {
#if CONFIG_LASER_SIMULATION_MODE
    printf("LASER: X=%4d Y=%4d R=%3d G=%3d B=%3d I=%3d\n", p.x, p.y, p.r, p.g, p.b, p.i);
    return;
#endif

    // Scale 8-bit color to 12-bit DAC
    uint16_t r = ((uint16_t)p.r << 4);
    uint16_t g = ((uint16_t)p.g << 4);
    uint16_t b = ((uint16_t)p.b << 4);
    uint16_t i = ((uint16_t)p.i << 4);

    send_dac(spi_handle_xy, p.x, p.y);
    send_dac(spi_handle_rg, r, g);
    send_dac(spi_handle_bi, b, i);

    // Latch
    gpio_set_level(PIN_LDAC, 0);
    // Short delay? ESP32 is fast.
    // MCP4922 requires LDAC low pulse width min 100ns.
    // GPIO toggle takes > 100ns usually (APB bus).
    // Let's add a NOP just in case.
    asm("nop"); asm("nop");
    gpio_set_level(PIN_LDAC, 1);
}
