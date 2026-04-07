#include "laser_engine.h"
#include "sys_config.h"
#include "ble_midi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "math.h"
#include "esp_rom_sys.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

// External declaration from dac_spi.c
void dac_init(void);
void dac_output_point(ilda_point_t p);

static const char *TAG = "LASER_ENGINE";
static volatile bool s_engine_running = false;

// Precomputed Y positions for strings (10 points per string)
static const uint16_t s_y_points[11] = {
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

// Configuration for the harp strings
#define MAX_STRINGS 12
static int s_num_strings = 8;
static uint16_t s_string_pos[MAX_STRINGS];
static int s_base_note = 60; // Middle C

// Hit detection state
static volatile int s_current_string_index = -1;
static volatile bool s_string_hit_flags[MAX_STRINGS]; // Set by ISR
static bool s_string_state[MAX_STRINGS]; // Current Note State (On/Off)
static int s_string_debounce[MAX_STRINGS]; // Simple debounce counter

// Called by ISR
void IRAM_ATTR laser_engine_register_hit(void) {
    if (s_current_string_index >= 0 && s_current_string_index < MAX_STRINGS) {
        s_string_hit_flags[s_current_string_index] = true;
    }
}

// Initialize default fan
void init_default_strings() {
    for (int i=0; i<s_num_strings; i++) {
        // Spread across X axis (1000-3000)
        // Avoid Division by Zero if num_strings=1
        if (s_num_strings > 1) {
            s_string_pos[i] = 1000 + (2000 * i / (s_num_strings - 1));
        } else {
            s_string_pos[i] = 2048;
        }
        s_string_hit_flags[i] = false;
        s_string_state[i] = false;
        s_string_debounce[i] = 0;
    }
}

void laser_engine_task(void *arg) {
    dac_init();
    init_default_strings();

    ESP_LOGI(TAG, "Engine Started. Strings: %d, Base Note: %d", s_num_strings, s_base_note);

    // Scan rate control
    // 30 kpps = 33.3 us per point.
    int point_period_us = 33;
    #ifdef CONFIG_SCAN_RATE_KPPS
    point_period_us = 1000 / CONFIG_SCAN_RATE_KPPS;
    #endif

    int64_t next_frame_time = esp_timer_get_time();

    while (1) {
        // 1. Draw Frame
        for (int i = 0; i < s_num_strings; i++) {
            s_current_string_index = i;
            uint16_t x = s_string_pos[i];

            // Move to bottom (Blanking)
            ilda_point_t p_move = {x, 0, 0, 0, 0, 0};
            dac_output_point(p_move);
            esp_rom_delay_us(point_period_us);

            // Draw String (Upwards)
            // Color: Red if active, Green if inactive (Visual Feedback)
            uint8_t r = s_string_state[i] ? 255 : 0;
            uint8_t g = s_string_state[i] ? 0 : 255;

            // 10 points per string
            for (int k=0; k<=10; k++) {
                uint16_t y = s_y_points[k];
                ilda_point_t p_draw = {x, y, r, g, 0, 255};
                dac_output_point(p_draw);

                // Timing
                int64_t now = esp_timer_get_time();
                if (now < next_frame_time) {
                    esp_rom_delay_us(next_frame_time - now);
                }
                next_frame_time += point_period_us;
            }
        }
        s_current_string_index = -1; // End of scan

        // 2. Process Hits (End of Frame)
        for (int i = 0; i < s_num_strings; i++) {
            bool hit = s_string_hit_flags[i];
            s_string_hit_flags[i] = false; // Reset for next frame

            if (hit) {
                // Detected
                if (!s_string_state[i]) {
                    // Was Off, Now On -> Note On
                    s_string_state[i] = true;
                    ble_midi_send_note_on(s_base_note + i, 127);
                }
                s_string_debounce[i] = 5; // Keep active for 5 frames if signal lost briefly
            } else {
                // Not Detected
                if (s_string_state[i]) {
                    if (s_string_debounce[i] > 0) {
                        s_string_debounce[i]--;
                    } else {
                        // Debounce expired -> Note Off
                        s_string_state[i] = false;
                        ble_midi_send_note_off(s_base_note + i);
                    }
                }
            }
        }

        // 3. Frame Rate Control / Yield
        // Minimize delay to keep scanning fast.
        // Yield is necessary for Watchdog (Core 1 WDT).
        vTaskDelay(1);
        next_frame_time = esp_timer_get_time();
    }
}

void laser_engine_init(void) {
    // Read Config from NVS
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READONLY, &my_handle);
    if (err == ESP_OK) {
        int32_t base = 0, count = 0;
        if (nvs_get_i32(my_handle, "base_note", &base) == ESP_OK) {
            s_base_note = base;
        }
        if (nvs_get_i32(my_handle, "str_count", &count) == ESP_OK) {
            if (count < 1) count = 1;
            if (count > MAX_STRINGS) count = MAX_STRINGS;
            s_num_strings = count;
        }
        nvs_close(my_handle);
    }
}

void laser_engine_start(void) {
    if (!s_engine_running) {
        xTaskCreatePinnedToCore(laser_engine_task, "laser_task", 4096, NULL, 5, NULL, TASK_CORE_LASER);
        s_engine_running = true;
    }
}
