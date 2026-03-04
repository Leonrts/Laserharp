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
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = DAC_SPI_FREQ_HZ,
        .mode = 0, // SPI mode 0
        .spics_io_num = -1, // We control CS manually? Or per device.
        // If we use per device, we add 3 devices.
        // MCP4922 requires CS to toggle for each 16-bit word?
        // Or can we send 32 bits? No, it's 16 bit.
        // We have to send 2 words per chip (Ch A and Ch B).
        // It's better to let the driver handle CS.
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
    uint16_t data_a = MCP4922_CMD_A | (val_a & 0x0FFF);
    uint16_t data_b = MCP4922_CMD_B | (val_b & 0x0FFF);

    // Swap bytes: 0x7000 -> 0x0070 (MCP4922 expects MSB first, but ESP32 is LE)
    data_a = (data_a >> 8) | (data_a << 8);
    data_b = (data_b >> 8) | (data_b << 8);

    static spi_transaction_t t = {
        .flags = SPI_TRANS_USE_TXDATA,
        .length = 16,
    };

    t.tx_data[0] = (uint8_t)(data_a & 0xFF);
    t.tx_data[1] = (uint8_t)((data_a >> 8) & 0xFF);

    ESP_ERROR_CHECK(spi_device_polling_transmit(handle, &t));

    // Send Channel B (reuse transaction struct)
    t.tx_data[0] = (uint8_t)(data_b & 0xFF);
    t.tx_data[1] = (uint8_t)((data_b >> 8) & 0xFF);
    ESP_ERROR_CHECK(spi_device_polling_transmit(handle, &t));
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
