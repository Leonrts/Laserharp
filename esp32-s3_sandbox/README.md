# ESP32-S3 SPI Benchmark Sandbox

This folder contains a micro-benchmark to evaluate the performance impact of using designated initializers and `SPI_TRANS_USE_TXDATA` versus `memset` and pointer indirection for `spi_transaction_t` setup.

## Files

- `mock_esp_idf.h`: Mocks the `spi_transaction_t` struct and `SPI_TRANS_USE_TXDATA` constant from ESP-IDF driver.
- `benchmark.c`: Compares "Old" (memset + tx_buffer) vs "New" (designated init + tx_data) vs "Static" (static struct) approaches.
- `Makefile`: Compiles the benchmark.

## Usage

```bash
make
./benchmark
make clean
```

## Results

On host (x86_64), the setup overhead is comparable (slightly faster for "New" approach due to fewer instructions/cache misses). The primary benefit on actual ESP32 hardware is the `SPI_TRANS_USE_TXDATA` flag which avoids DMA setup for small transactions, a significant performance gain not fully captured here.
