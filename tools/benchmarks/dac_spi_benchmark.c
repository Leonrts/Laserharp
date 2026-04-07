#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

#define SPI_TRANS_USE_TXDATA 0x01
#define MCP4922_CMD_A 0x7000
#define MCP4922_CMD_B 0xF000

typedef struct {
    uint32_t flags;
    size_t length;
    uint8_t tx_data[4];
    void *user;
} spi_transaction_t;

// Dummy handle for function signature
typedef void* spi_device_handle_t;

// Mock ESP_ERROR_CHECK to do nothing
#define ESP_ERROR_CHECK(x) (void)(x)

// Mock spi_device_polling_transmit to do nothing (we're benchmarking struct init)
int spi_device_polling_transmit(spi_device_handle_t handle, spi_transaction_t *t) {
    // Avoid optimizing out
    volatile uint8_t tmp = t->tx_data[0];
    (void)tmp;
    return 0;
}

// ==========================================
// UNOPTIMIZED (Baseline)
// ==========================================
void send_dac_unoptimized(spi_device_handle_t handle, uint16_t val_a, uint16_t val_b) {
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
        },
    };

    ESP_ERROR_CHECK(spi_device_polling_transmit(handle, &t));

    t.tx_data[0] = (uint8_t)(data_b & 0xFF);
    t.tx_data[1] = (uint8_t)((data_b >> 8) & 0xFF);
    ESP_ERROR_CHECK(spi_device_polling_transmit(handle, &t));
}

// ==========================================
// OPTIMIZED
// ==========================================
void send_dac_optimized(spi_device_handle_t handle, uint16_t val_a, uint16_t val_b) {
    uint16_t data_a = MCP4922_CMD_A | (val_a & 0x0FFF);
    uint16_t data_b = MCP4922_CMD_B | (val_b & 0x0FFF);

    data_a = (data_a >> 8) | (data_a << 8);
    data_b = (data_b >> 8) | (data_b << 8);

    static spi_transaction_t t = {
        .flags = SPI_TRANS_USE_TXDATA,
        .length = 16,
    };

    t.tx_data[0] = (uint8_t)(data_a & 0xFF);
    t.tx_data[1] = (uint8_t)((data_a >> 8) & 0xFF);

    ESP_ERROR_CHECK(spi_device_polling_transmit(handle, &t));

    t.tx_data[0] = (uint8_t)(data_b & 0xFF);
    t.tx_data[1] = (uint8_t)((data_b >> 8) & 0xFF);
    ESP_ERROR_CHECK(spi_device_polling_transmit(handle, &t));
}


int main() {
    int iterations = 10000000;
    clock_t start, end;
    double cpu_time_used;

    // Baseline
    start = clock();
    for(int i = 0; i < iterations; i++) {
        send_dac_unoptimized(NULL, i % 4096, (i * 2) % 4096);
    }
    end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Unoptimized Time: %f seconds\n", cpu_time_used);

    // Optimized
    start = clock();
    for(int i = 0; i < iterations; i++) {
        send_dac_optimized(NULL, i % 4096, (i * 2) % 4096);
    }
    end = clock();
    double optimized_time = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Optimized Time:   %f seconds\n", optimized_time);

    printf("Improvement:      %f%%\n", (1.0 - (optimized_time / cpu_time_used)) * 100.0);

    return 0;
}
