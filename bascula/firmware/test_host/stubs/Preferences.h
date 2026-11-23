#pragma once
#include <Arduino.h>
struct Preferences {
  bool begin(const char*,bool){return true;}
  size_t getBytesLength(const char*k); size_t getBytes(const char*k,void*b,size_t n); size_t putBytes(const char*k,const void*b,size_t n);
};
