#ifndef ESP_WIFI_H
#define ESP_WIFI_H
#include "esp_err.h"
#include <stdint.h>
#define WIFI_EVENT 0
#define WIFI_EVENT_AP_STACONNECTED 1
#define WIFI_EVENT_AP_STADISCONNECTED 2
#define WIFI_MODE_AP 1
#define WIFI_IF_AP 0
#define WIFI_AUTH_WPA_WPA2_PSK 3
#define WIFI_AUTH_OPEN 0
typedef struct { uint8_t mac[6]; int aid; } wifi_event_ap_staconnected_t;
typedef struct { uint8_t mac[6]; int aid; } wifi_event_ap_stadisconnected_t;
typedef struct { struct { char ssid[32]; int ssid_len; int channel; char password[64]; int max_connection; int authmode; } ap; } wifi_config_t;
typedef struct {} wifi_init_config_t;
#define WIFI_INIT_CONFIG_DEFAULT() {}
static inline esp_err_t esp_wifi_init(const wifi_init_config_t *c) { return ESP_OK; }
static inline esp_err_t esp_wifi_set_mode(int mode) { return ESP_OK; }
static inline esp_err_t esp_wifi_set_config(int interface, void *conf) { return ESP_OK; }
static inline esp_err_t esp_wifi_start(void) { return ESP_OK; }
#ifndef MACSTR
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"
#endif
#ifndef MAC2STR
#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#endif
#endif
