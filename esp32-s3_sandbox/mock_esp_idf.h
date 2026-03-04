#ifndef MOCK_ESP_IDF_H
#define MOCK_ESP_IDF_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define SPI_TRANS_USE_TXDATA    (1<<1)

typedef struct spi_transaction_t {
    uint32_t flags;
    uint16_t cmd;
    uint64_t addr;
    size_t length;
    size_t rxlength;
    void *user;
    union {
        const void *tx_buffer;
        uint8_t tx_data[4];
    };
    union {
        void *rx_buffer;
        uint8_t rx_data[4];
    };
} spi_transaction_t;

extern volatile uint32_t g_dummy_accum;

// Mock function for SPI transmit
static inline void spi_device_polling_transmit(void *handle, spi_transaction_t *t) {
    // Simulate reading the data to prevent optimization
    if (t->flags & SPI_TRANS_USE_TXDATA) {
        g_dummy_accum += t->tx_data[0] + t->tx_data[1];
    } else {
        if (t->tx_buffer) {
            const uint8_t *p = (const uint8_t *)t->tx_buffer;
            g_dummy_accum += p[0] + p[1];
        }
    }
}

#define MCP4922_CMD_A 0x7000
#define MCP4922_CMD_B 0xF000

// Adding esp_timer and esp_rom functions for benchmarking
static inline int64_t esp_timer_get_time(void) {
    // Return something that changes to simulate time
    static int64_t time = 0;
    return time++;
}

static inline void esp_rom_delay_us(uint32_t us) {
    // Simulate delay
    g_dummy_accum += us;
}

#endif // MOCK_ESP_IDF_H
