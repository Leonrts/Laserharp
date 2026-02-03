idf_component_register(SRCS "src/dac_spi.c" "src/ilda_generator.c"
                    INCLUDE_DIRS "include"
                    REQUIRES driver esp_timer main ble_midi nvs_flash)
