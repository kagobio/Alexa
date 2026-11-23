#pragma once
#include <Arduino.h>
const int HX711_OK = 0;
extern float g_raw;
struct HX711 { bool begin(int,int,bool=false,bool=true){return true;} void set_rate_pin(int){} void set_rate_80SPS(){}
  bool is_ready(){return true;} float read(uint32_t=500){return g_raw;} int getLastError(){return 0;} };
