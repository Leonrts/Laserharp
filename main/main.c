#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sys_config.h"
#include "nvs_flash.h"
#include "esp_log.h"

// Component Headers
#include "laser_engine.h"
#include "input_capture.h"
#include "ble_midi.h"
#include "web_interface.h"

static const char *TAG = "MAIN";

// Callback from Input Capture ISR
void IRAM_ATTR sensor_callback(void) {
    laser_engine_register_hit();
}

void app_main(void)
{
    printf("\n\n");
    ESP_LOGI(TAG, "Laser Harp Controller Starting...");
    ESP_LOGI(TAG, "Simulation Mode: %s", CONFIG_LASER_SIMULATION_MODE ? "ENABLED" : "DISABLED");

    // 1. Initialize NVS (Required for WiFi & BLE)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. Initialize Web Interface (Core 0)
    web_interface_init();

    // 3. Initialize BLE MIDI (Core 0)
    ble_midi_init();

    // 4. Initialize Input Capture
    input_capture_init(PIN_SENSOR_IN, sensor_callback);

    // 5. Start Laser Engine (Core 1)
    laser_engine_init();
    laser_engine_start();

    ESP_LOGI(TAG, "System Running.");
}
