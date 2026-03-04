#include <stdio.h>
#include <time.h>
#include "mock_esp_idf.h"

volatile uint32_t g_dummy_accum = 0;

// Force use of volatile to simulate non-optimized behavior
volatile int64_t g_time = 0;

static inline int64_t get_time_busy() {
    return g_time++;
}

static inline int64_t get_time_optimized() {
    return g_time;
}

void busy_wait_test(int iterations, int64_t point_period_us) {
    int64_t next_frame_time = 0;
    for (int i = 0; i < iterations; i++) {
        // Busy wait - calling a function repeatedly
        int64_t now = get_time_busy();
        while (now < next_frame_time) {
            now = get_time_busy();
        }
        next_frame_time += point_period_us;
    }
}

void optimized_delay_test(int iterations, int64_t point_period_us) {
    int64_t next_frame_time = 0;
    for (int i = 0; i < iterations; i++) {
        // Optimized delay - single call
        int64_t now = get_time_optimized();
        if (now < next_frame_time) {
            esp_rom_delay_us(next_frame_time - now);
        }
        next_frame_time += point_period_us;
    }
}

int main() {
    const int ITERATIONS = 100000000;
    const int64_t POINT_PERIOD_US = 33;
    clock_t start, end;
    double time_busy, time_optimized;

    printf("Benchmarking Busy Wait vs Optimized Delay (with forced overhead)...\n");
    printf("Iterations: %d, Target Period: %lld us\n", ITERATIONS, (long long)POINT_PERIOD_US);

    // Baseline Busy Wait
    g_time = 0;
    start = clock();
    busy_wait_test(ITERATIONS, POINT_PERIOD_US);
    end = clock();
    time_busy = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Busy Wait Time: %f seconds\n", time_busy);

    // Optimized Delay
    g_time = 0;
    start = clock();
    optimized_delay_test(ITERATIONS, POINT_PERIOD_US);
    end = clock();
    time_optimized = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Optimized Delay Time: %f seconds\n", time_optimized);

    printf("Speedup: %.2fx\n", time_busy / time_optimized);

    return 0;
}
