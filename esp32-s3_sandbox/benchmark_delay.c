#include <stdio.h>
#include <time.h>
#include <stdint.h>

// Mock esp_timer_get_time to simulate timer increments
static uint64_t simulated_time = 0;
uint64_t esp_timer_get_time() {
    // Increment time by a small amount each call to simulate time passing
    // This allows the while loop to eventually terminate
    simulated_time += 1;
    return simulated_time;
}

// Mock esp_rom_delay_us
void esp_rom_delay_us(uint32_t us) {
    simulated_time += us; // Instantly advance simulated time
}

// Test scenario for busy wait
void wait_busy(int64_t next_frame_time) {
    int64_t now = esp_timer_get_time();
    while (now < next_frame_time) {
        now = esp_timer_get_time();
    }
}

// Test scenario for delay wait
void wait_delay(int64_t next_frame_time) {
    int64_t now = esp_timer_get_time();
    if (now < next_frame_time) {
        esp_rom_delay_us(next_frame_time - now);
        // Ensure simulated time correctly matches
        simulated_time = next_frame_time;
    }
}

int main() {
    const int ITERATIONS = 1000000;
    const int DELAY_AMOUNT_US = 33; // ~30kpps point delay
    clock_t start, end;
    double time_busy, time_delay;

    printf("Benchmarking wait mechanism overhead...\n");
    printf("Iterations: %d\n", ITERATIONS);

    // Benchmark Busy Wait
    simulated_time = 0;
    start = clock();
    for (int i = 0; i < ITERATIONS; i++) {
        int64_t next_time = esp_timer_get_time() + DELAY_AMOUNT_US;
        wait_busy(next_time);
    }
    end = clock();
    time_busy = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Busy Wait Implementation Time: %f seconds\n", time_busy);

    // Benchmark Delay Wait
    simulated_time = 0;
    start = clock();
    for (int i = 0; i < ITERATIONS; i++) {
        int64_t next_time = esp_timer_get_time() + DELAY_AMOUNT_US;
        wait_delay(next_time);
    }
    end = clock();
    time_delay = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Delay Implementation Time: %f seconds\n", time_delay);

    printf("Speedup (Delay vs Busy Wait): %.2fx\n", time_busy / time_delay);

    return 0;
}
