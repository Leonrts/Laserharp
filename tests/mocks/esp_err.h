#ifndef ESP_ERR_H
#define ESP_ERR_H

#include <stdlib.h>
#include <stdio.h>

#define ESP_OK 0
#define ESP_FAIL -1
typedef int esp_err_t;

#define ESP_ERROR_CHECK(x) do { \
    esp_err_t __err_rc = (x); \
    if (__err_rc != ESP_OK) { \
        printf("ESP_ERROR_CHECK failed: %d\n", __err_rc); \
        abort(); \
    } \
} while(0)

#endif
