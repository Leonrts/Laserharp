#ifndef ESP_TIMER_H
#define ESP_TIMER_H

#include <stdint.h>

typedef struct esp_timer* esp_timer_handle_t;
typedef void (*esp_timer_cb_t)(void* arg);

typedef struct {
    esp_timer_cb_t callback;
    void* arg;
    const char* name;
    bool skip_unhandled_events;
} esp_timer_create_args_t;

static inline int64_t esp_timer_get_time(void) {
    return 0;
}

static inline uint32_t esp_timer_get_period(esp_timer_handle_t timer) {
    return 0;
}

static inline esp_err_t esp_timer_create(const esp_timer_create_args_t* create_args, esp_timer_handle_t* out_handle) {
    return 0;
}

static inline esp_err_t esp_timer_start_once(esp_timer_handle_t timer, uint64_t timeout_us) {
    return 0;
}

static inline esp_err_t esp_timer_stop(esp_timer_handle_t timer) {
    return 0;
}

static inline esp_err_t esp_timer_delete(esp_timer_handle_t timer) {
    return 0;
}

#endif // ESP_TIMER_H
