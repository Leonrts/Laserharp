#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

// Mock definitions
#include "esp_err.h"
#include "nvs.h"
#include "esp_http_server.h"
#include "freertos/task.h"

// Define mock function pointers
nvs_open_fn_t mock_nvs_open = NULL;
nvs_close_fn_t mock_nvs_close = NULL;
nvs_get_i32_fn_t mock_nvs_get_i32 = NULL;
nvs_set_i32_fn_t mock_nvs_set_i32 = NULL;
nvs_commit_fn_t mock_nvs_commit = NULL;
httpd_resp_send_fn_t mock_httpd_resp_send = NULL;

// Mock implementations for the test
static int32_t mock_base_note = 60;
static int32_t mock_str_count = 8;
static char last_response[2048];

esp_err_t my_nvs_open(const char* name, nvs_open_mode_t open_mode, nvs_handle_t *out_handle) {
    *out_handle = 1;
    return ESP_OK;
}

void my_nvs_close(nvs_handle_t handle) {
}

esp_err_t my_nvs_get_i32(nvs_handle_t handle, const char* key, int32_t* out_value) {
    if (strcmp(key, "base_note") == 0) *out_value = mock_base_note;
    else if (strcmp(key, "str_count") == 0) *out_value = mock_str_count;
    else return ESP_FAIL;
    return ESP_OK;
}

esp_err_t my_httpd_resp_send(httpd_req_t *r, const char *buf, ssize_t buf_len) {
    if (buf_len == HTTPD_RESP_USE_STRLEN) buf_len = strlen(buf);
    if (buf_len >= sizeof(last_response)) buf_len = sizeof(last_response) - 1;
    memcpy(last_response, buf, buf_len);
    last_response[buf_len] = '\0';
    return ESP_OK;
}

// Dummies for other symbols
void esp_netif_create_default_wifi_ap(void) {}
void esp_restart(void) {}

// Include the source file under test
// We need to trick the compiler to find the includes
#include "wifi_portal.c"

void test_root_get_handler() {
    printf("Testing root_get_handler...\n");

    // Setup mocks
    mock_nvs_open = my_nvs_open;
    mock_nvs_close = my_nvs_close;
    mock_nvs_get_i32 = my_nvs_get_i32;
    mock_httpd_resp_send = my_httpd_resp_send;

    // Test case 1: Default values
    mock_base_note = 60;
    mock_str_count = 8;
    httpd_req_t req = {0};

    esp_err_t res = root_get_handler(&req);
    assert(res == ESP_OK);

    // Verify response contains expected values
    assert(strstr(last_response, "value='60'") != NULL);
    assert(strstr(last_response, "value='8'") != NULL);
    assert(strstr(last_response, "<!DOCTYPE html>") != NULL);

    // Test case 2: Changed values
    mock_base_note = 48;
    mock_str_count = 12;
    res = root_get_handler(&req);
    assert(res == ESP_OK);
    assert(strstr(last_response, "value='48'") != NULL);
    assert(strstr(last_response, "value='12'") != NULL);

    printf("root_get_handler passed.\n");
}

int main() {
    test_root_get_handler();
    return 0;
}
