#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <stdlib.h>

#define ITERATIONS 10000000

uint16_t volatile sum_baseline = 0;
uint16_t volatile sum_optimized = 0;

void run_baseline() {
    uint16_t local_sum = 0;
    for (int i = 0; i < ITERATIONS; i++) {
        for (int k = 0; k <= 10; k++) {
            uint16_t y = (4095 * k) / 10;
            local_sum += y;
        }
    }
    sum_baseline = local_sum;
}

void run_optimized() {
    uint16_t local_sum = 0;
    static const uint16_t y_lookup[11] = {
        0, 409, 819, 1228, 1638, 2047, 2457, 2866, 3276, 3685, 4095
    };
    for (int i = 0; i < ITERATIONS; i++) {
        for (int k = 0; k <= 10; k++) {
            uint16_t y = y_lookup[k];
            local_sum += y;
        }
    }
    sum_optimized = local_sum;
}

int main() {
    clock_t start, end;
    double cpu_time_used_baseline, cpu_time_used_optimized;

    start = clock();
    run_baseline();
    end = clock();
    cpu_time_used_baseline = ((double) (end - start)) / CLOCKS_PER_SEC;

    start = clock();
    run_optimized();
    end = clock();
    cpu_time_used_optimized = ((double) (end - start)) / CLOCKS_PER_SEC;

    printf("Baseline Time (Math): %f seconds\n", cpu_time_used_baseline);
    printf("Optimized Time (Lookup): %f seconds\n", cpu_time_used_optimized);

    // Verify correctness
    for(int k=0; k<=10; k++) {
        uint16_t base = (4095 * k) / 10;
        uint16_t opt = (k==0?0:(k==1?409:(k==2?819:(k==3?1228:(k==4?1638:(k==5?2047:(k==6?2457:(k==7?2866:(k==8?3276:(k==9?3685:4095))))))))));
        if (base != opt) {
            printf("Mismatch!\n");
            return 1;
        }
    }
    return 0;
}
