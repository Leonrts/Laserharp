#include "web_interface.h"
#include <string.h>
#include <stdlib.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#define TAG "WEB_IF"

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

static esp_err_t root_get_handler(httpd_req_t *req) {
    // Read current config
    nvs_handle_t my_handle;
    int base = 60;
    int count = 8;

    esp_err_t err = nvs_open("storage", NVS_READONLY, &my_handle);
    if (err == ESP_OK) {
        nvs_get_i32(my_handle, "base_note", &base);
        nvs_get_i32(my_handle, "str_count", &count);
        nvs_close(my_handle);
    }

    char *resp_str = malloc(strlen(index_html_fmt) + 20);
    sprintf(resp_str, index_html_fmt, base, count);
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
    free(resp_str);
    return ESP_OK;
}

// Simple URL decoder / parser
static int get_param_value(char *buf, const char *param_name) {
    char *start = strstr(buf, param_name);
    if (!start) return -1;
    start += strlen(param_name) + 1; // skip name and '='
    return atoi(start);
}

static esp_err_t save_post_handler(httpd_req_t *req) {
    char buf[100];
    int ret, remaining = req->content_len;

    if (remaining >= sizeof(buf)) {
        remaining = sizeof(buf) - 1;
    }

    ret = httpd_req_recv(req, buf, remaining);
    if (ret <= 0) return ESP_FAIL;
    buf[ret] = '\0';

    ESP_LOGI(TAG, "Received config: %s", buf);

    int base = get_param_value(buf, "base");
    int count = get_param_value(buf, "count");

    if (base > 0 && count > 0) {
        nvs_handle_t my_handle;
        esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
        if (err == ESP_OK) {
            nvs_set_i32(my_handle, "base_note", base);
            nvs_set_i32(my_handle, "str_count", count);
            nvs_commit(my_handle);
            nvs_close(my_handle);
            ESP_LOGI(TAG, "Config Saved: Base=%d, Count=%d", base, count);
        }
    }

    httpd_resp_send(req, "Configuration Saved. Rebooting...", HTTPD_RESP_USE_STRLEN);

    // Reboot after delay
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_restart();

    return ESP_OK;
}

static httpd_handle_t start_webserver(void) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 8;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t uri_root = {
            .uri       = "/",
            .method    = HTTP_GET,
            .handler   = root_get_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &uri_root);

        httpd_uri_t uri_save = {
            .uri       = "/save",
            .method    = HTTP_POST,
            .handler   = save_post_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &uri_save);
    }
    return server;
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data) {
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
}

void web_interface_init(void) {
    // Note: NVS, Netif, EventLoop must be initialized before calling this
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = "LaserHarp_Config",
            .ssid_len = strlen("LaserHarp_Config"),
            .channel = 1,
            .password = "laserharp",
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };
    if (strlen("laserharp") == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s",
             "LaserHarp_Config", "laserharp");

    start_webserver();
}
