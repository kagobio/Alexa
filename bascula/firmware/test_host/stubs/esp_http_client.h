#pragma once
#include <Arduino.h>
typedef int esp_err_t; enum { ESP_OK = 0 }; enum { HTTP_METHOD_POST = 1 };
struct esp_http_client_config_t { const char* url; int method; esp_err_t (*crt_bundle_attach)(void*); int timeout_ms; };
typedef void* esp_http_client_handle_t;
esp_http_client_handle_t esp_http_client_init(const esp_http_client_config_t*);
void esp_http_client_set_header(esp_http_client_handle_t,const char*,const char*);
void esp_http_client_set_post_field(esp_http_client_handle_t,const char*,int);
esp_err_t esp_http_client_perform(esp_http_client_handle_t);
int esp_http_client_get_status_code(esp_http_client_handle_t);
void esp_http_client_cleanup(esp_http_client_handle_t);
