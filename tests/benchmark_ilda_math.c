#include <stdio.h>
#include <time.h>
#include <stdint.h>

volatile uint32_t g_dummy_accum = 0;

static void compute_old(void) {
    for (int k=0; k<=10; k++) {
        uint16_t y = (4095 * k) / 10;
        g_dummy_accum += y;
    }
}

static const uint16_t precomputed_y[11] = {
    0,
    (4095 * 1) / 10,
    (4095 * 2) / 10,
    (4095 * 3) / 10,
    (4095 * 4) / 10,
    (4095 * 5) / 10,
    (4095 * 6) / 10,
    (4095 * 7) / 10,
    (4095 * 8) / 10,
    (4095 * 9) / 10,
    4095
};

static void compute_new(void) {
    for (int k=0; k<=10; k++) {
        uint16_t y = precomputed_y[k];
        g_dummy_accum += y;
    }
}

int main() {
    const int ITERATIONS = 100000000;
    clock_t start, end;
    double time_old, time_new;

    printf("Benchmarking ILDA y calculation...\n");
    printf("Iterations: %d\n", ITERATIONS);

    // Benchmark Old
    g_dummy_accum = 0;
    start = clock();
    for (int i = 0; i < ITERATIONS; i++) {
        compute_old();
    }
    end = clock();
    time_old = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Old Implementation Time: %f seconds\n", time_old);
    printf("Accumulator Old: %u\n", g_dummy_accum);

    // Benchmark New
    g_dummy_accum = 0;
    start = clock();
    for (int i = 0; i < ITERATIONS; i++) {
        compute_new();
    }
    end = clock();
    time_new = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("New Implementation Time: %f seconds\n", time_new);
    printf("Accumulator New: %u\n", g_dummy_accum);

    printf("Speedup: %.2fx\n", time_old / time_new);

    return 0;
}
