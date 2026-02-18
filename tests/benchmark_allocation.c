#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const char* index_html_fmt =
"<!DOCTYPE html><html><head><title>Laser Harp Config</title>"
"<meta name='viewport' content='width=device-width, initial-scale=1'>"
"<style>body{font-family:sans-serif;margin:20px;text-align:center;}"
"input{padding:10px;margin:10px;width:80%;}button{padding:10px 20px;background:#007bff;color:white;border:none;}</style>"
"</head><body><h1>Laser Harp Settings</h1>"
"<form action='/save' method='post'>"
"<label>Base Note (MIDI):</label><br><input type='number' name='base' value='%d'><br>"
"<label>String Count:</label><br><input type='number' name='count' value='%d'><br>"
"<button type='submit'>Save & Reboot</button>"
"</form></body></html>";

void benchmark_heap(int iterations) {
    clock_t start = clock();
    int base = 60;
    int count = 8;
    volatile int dummy = 0; // Prevent optimization

    for(int i=0; i<iterations; i++) {
        char *resp_str = malloc(strlen(index_html_fmt) + 20);
        if (resp_str) {
            sprintf(resp_str, index_html_fmt, base, count);
            dummy += resp_str[0]; // Access memory to prevent optimization
            free(resp_str);
        }
    }

    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
    printf("Heap Allocation: %f seconds for %d iterations\n", time_spent, iterations);
    printf("Average per op: %f ns\n", (time_spent * 1e9) / iterations);
}

void benchmark_stack(int iterations) {
    clock_t start = clock();
    int base = 60;
    int count = 8;
    volatile int dummy = 0;

    for(int i=0; i<iterations; i++) {
        char resp_str[1024];
        snprintf(resp_str, sizeof(resp_str), index_html_fmt, base, count);
        dummy += resp_str[0];
    }

    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
    printf("Stack Allocation: %f seconds for %d iterations\n", time_spent, iterations);
    printf("Average per op: %f ns\n", (time_spent * 1e9) / iterations);
}

int main() {
    int iterations = 1000000;
    printf("Running benchmark with %d iterations...\n", iterations);
    benchmark_heap(iterations);
    benchmark_stack(iterations);
    return 0;
}
