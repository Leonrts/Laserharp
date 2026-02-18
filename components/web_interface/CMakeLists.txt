idf_component_register(SRCS "src/wifi_portal.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_wifi esp_event esp_http_server lwip nvs_flash esp_timer)
