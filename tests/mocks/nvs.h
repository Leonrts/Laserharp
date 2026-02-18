#ifndef NVS_H
#define NVS_H

#include "esp_err.h"
#include <stdint.h>
#include <stdio.h>

typedef uint32_t nvs_handle_t;
typedef enum {
    NVS_READONLY,
    NVS_READWRITE
} nvs_open_mode_t;

typedef esp_err_t (*nvs_open_fn_t)(const char*, nvs_open_mode_t, nvs_handle_t*);
typedef void (*nvs_close_fn_t)(nvs_handle_t);
typedef esp_err_t (*nvs_get_i32_fn_t)(nvs_handle_t, const char*, int32_t*);
typedef esp_err_t (*nvs_set_i32_fn_t)(nvs_handle_t, const char*, int32_t);
typedef esp_err_t (*nvs_commit_fn_t)(nvs_handle_t);

extern nvs_open_fn_t mock_nvs_open;
extern nvs_close_fn_t mock_nvs_close;
extern nvs_get_i32_fn_t mock_nvs_get_i32;
extern nvs_set_i32_fn_t mock_nvs_set_i32;
extern nvs_commit_fn_t mock_nvs_commit;

static inline esp_err_t nvs_open(const char* name, nvs_open_mode_t open_mode, nvs_handle_t *out_handle) {
    if (mock_nvs_open) return mock_nvs_open(name, open_mode, out_handle);
    return ESP_OK;
}
static inline void nvs_close(nvs_handle_t handle) {
    if (mock_nvs_close) mock_nvs_close(handle);
}
static inline esp_err_t nvs_get_i32(nvs_handle_t handle, const char* key, int32_t* out_value) {
    if (mock_nvs_get_i32) return mock_nvs_get_i32(handle, key, out_value);
    return ESP_OK;
}
static inline esp_err_t nvs_set_i32(nvs_handle_t handle, const char* key, int32_t value) {
    if (mock_nvs_set_i32) return mock_nvs_set_i32(handle, key, value);
    return ESP_OK;
}
static inline esp_err_t nvs_commit(nvs_handle_t handle) {
    if (mock_nvs_commit) return mock_nvs_commit(handle);
    return ESP_OK;
}
#endif
