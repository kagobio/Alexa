#pragma once
#include <Arduino.h>
#define WIFI_STA 1
#define WIFI_OFF 0
#define WL_CONNECTED 3
extern int g_wifi_status; extern int g_wifi_on; extern int g_wifi_calls;
struct WiFiClass { void mode(int m){ g_wifi_on = m; g_wifi_calls++; } void setAutoReconnect(bool){} void setSleep(bool){} void begin(){} void disconnect(bool){} int status(){return g_wifi_status;} String SSID(){return String("casa");} };
extern WiFiClass WiFi;
