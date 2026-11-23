#pragma once
#include <Arduino.h>
struct WiFiManager { void setConfigPortalTimeout(int){} void setConnectTimeout(int){} bool startConfigPortal(const char*,const char*){return true;} };
