#ifndef ESP_HTTP_SERVER_H
#define ESP_HTTP_SERVER_H

#include "esp_err.h"
#include <sys/types.h>
#include <string.h>

#define HTTPD_RESP_USE_STRLEN -1

typedef struct {
    int content_len;
    void *user_ctx;
} httpd_req_t;

typedef enum {
    HTTP_GET,
    HTTP_POST,
    HTTP_PUT,
    HTTP_DELETE,
    HTTP_HEAD
} http_method;

typedef esp_err_t (*httpd_req_handler_t)(httpd_req_t *r);

typedef struct {
    const char *uri;
    http_method method;
    httpd_req_handler_t handler;
    void *user_ctx;
} httpd_uri_t;

typedef void* httpd_handle_t;

typedef struct {
    int max_uri_handlers;
} httpd_config_t;

#define HTTPD_DEFAULT_CONFIG() { .max_uri_handlers = 8 }

typedef esp_err_t (*httpd_resp_send_fn_t)(httpd_req_t*, const char*, ssize_t);
extern httpd_resp_send_fn_t mock_httpd_resp_send;

static inline esp_err_t httpd_start(httpd_handle_t *handle, const httpd_config_t *config) { return ESP_OK; }
static inline esp_err_t httpd_register_uri_handler(httpd_handle_t handle, const httpd_uri_t *uri_handler) { return ESP_OK; }
static inline esp_err_t httpd_resp_send(httpd_req_t *r, const char *buf, ssize_t buf_len) {
    if (mock_httpd_resp_send) return mock_httpd_resp_send(r, buf, buf_len);
    return ESP_OK;
}
static inline int httpd_req_recv(httpd_req_t *r, char *buf, size_t buf_len) { return 0; }

#endif
