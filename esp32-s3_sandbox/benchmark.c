#include <stdio.h>
#include <time.h>
#include "mock_esp_idf.h"

volatile uint32_t g_dummy_accum = 0;

// Define helper to simulate old behavior
static void send_dac_old(void *handle, uint16_t val_a, uint16_t val_b) {
    spi_transaction_t t;
    uint16_t data;

    // Send Channel A
    memset(&t, 0, sizeof(t));
    data = MCP4922_CMD_A | (val_a & 0x0FFF);
    data = (data >> 8) | (data << 8);
    t.length = 16;
    t.tx_buffer = &data;
    spi_device_polling_transmit(handle, &t);

    // Send Channel B
    data = MCP4922_CMD_B | (val_b & 0x0FFF);
    data = (data >> 8) | (data << 8);
    t.tx_buffer = &data;
    spi_device_polling_transmit(handle, &t);
}

// Define helper to simulate new optimized behavior (Stack-based designated init)
static void send_dac_new(void *handle, uint16_t val_a, uint16_t val_b) {
    uint16_t data_a = MCP4922_CMD_A | (val_a & 0x0FFF);
    uint16_t data_b = MCP4922_CMD_B | (val_b & 0x0FFF);

    data_a = (data_a >> 8) | (data_a << 8);
    data_b = (data_b >> 8) | (data_b << 8);

    spi_transaction_t t = {
        .flags = SPI_TRANS_USE_TXDATA,
        .length = 16,
        .tx_data = {
            (uint8_t)(data_a & 0xFF),
            (uint8_t)((data_a >> 8) & 0xFF),
            0, 0
        }
    };
    spi_device_polling_transmit(handle, &t);

    t.tx_data[0] = (uint8_t)(data_b & 0xFF);
    t.tx_data[1] = (uint8_t)((data_b >> 8) & 0xFF);

    spi_device_polling_transmit(handle, &t);
}

// Define helper to simulate Static approach
static void send_dac_static(void *handle, uint16_t val_a, uint16_t val_b) {
    static spi_transaction_t t = {
        .flags = SPI_TRANS_USE_TXDATA,
        .length = 16,
    };
    // Note: static initialization happens once.
    // If we need to re-zero/reset other fields, we must be careful.
    // But we only change tx_data.

    uint16_t data_a = MCP4922_CMD_A | (val_a & 0x0FFF);
    uint16_t data_b = MCP4922_CMD_B | (val_b & 0x0FFF);

    data_a = (data_a >> 8) | (data_a << 8);
    data_b = (data_b >> 8) | (data_b << 8);

    t.tx_data[0] = (uint8_t)(data_a & 0xFF);
    t.tx_data[1] = (uint8_t)((data_a >> 8) & 0xFF);
    spi_device_polling_transmit(handle, &t);

    t.tx_data[0] = (uint8_t)(data_b & 0xFF);
    t.tx_data[1] = (uint8_t)((data_b >> 8) & 0xFF);
    spi_device_polling_transmit(handle, &t);
}

int main() {
    const int ITERATIONS = 10000000;
    clock_t start, end;
    double time_old, time_new, time_static;
    void *handle = NULL;

    printf("Benchmarking send_dac optimization...\n");
    printf("Iterations: %d\n", ITERATIONS);

    // Benchmark Old
    g_dummy_accum = 0;
    start = clock();
    for (int i = 0; i < ITERATIONS; i++) {
        send_dac_old(handle, i % 4096, (i + 1) % 4096);
    }
    end = clock();
    time_old = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Old Implementation Time: %f seconds\n", time_old);
    printf("Accumulator Old: %u\n", g_dummy_accum);

    // Benchmark New
    g_dummy_accum = 0;
    start = clock();
    for (int i = 0; i < ITERATIONS; i++) {
        send_dac_new(handle, i % 4096, (i + 1) % 4096);
    }
    end = clock();
    time_new = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("New Implementation Time: %f seconds\n", time_new);
    printf("Accumulator New: %u\n", g_dummy_accum);

    // Benchmark Static
    g_dummy_accum = 0;
    start = clock();
    for (int i = 0; i < ITERATIONS; i++) {
        send_dac_static(handle, i % 4096, (i + 1) % 4096);
    }
    end = clock();
    time_static = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Static Implementation Time: %f seconds\n", time_static);
    printf("Accumulator Static: %u\n", g_dummy_accum);

    printf("Speedup (New vs Old): %.2fx\n", time_old / time_new);
    printf("Speedup (Static vs Old): %.2fx\n", time_old / time_static);

    return 0;
}
