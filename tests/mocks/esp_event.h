#ifndef ESP_EVENT_H
#define ESP_EVENT_H
#include "esp_err.h"
#include <stdint.h>
typedef int esp_event_base_t;
#define ESP_EVENT_ANY_ID -1
static inline esp_err_t esp_event_handler_instance_register(esp_event_base_t b, int32_t id, void* h, void* a, void* i) { return ESP_OK; }
#endif
