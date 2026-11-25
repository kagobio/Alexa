#pragma once
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#define IRAM_ATTR
#define DRAM_ATTR
#define PROGMEM
#define INPUT_PULLUP 2
#define OUTPUT 1
#define HIGH 1
#define LOW 0
#define CHANGE 3
#define constrain(a,l,h) ((a)<(l)?(l):((a)>(h)?(h):(a)))
uint32_t millis(); void delay(uint32_t);
int digitalRead(int); void pinMode(int,int); void digitalWrite(int,int);
inline int digitalPinToInterrupt(int p){return p;}
void attachInterrupt(int,void(*)(),int);
long random(long m);
void ledcAttach(int,int,int); void ledcWrite(int,int);
uint32_t analogReadMilliVolts(int); void analogSetPinAttenuation(int,int);
#define ADC_11db 3
struct String {
  std::string s;
  String(){} String(const char*c):s(c){} String(int v):s(std::to_string(v)){} String(unsigned v):s(std::to_string(v)){}
  String(const std::string&x):s(x){}
  String operator+(const String&o)const{return String(s+o.s);}
  String operator+(const char*o)const{return String(s+o);}
  String& operator=(const char*c){s=c;return *this;}
  const char* c_str()const{return s.c_str();} size_t length()const{return s.size();}
};
struct SerialT { void begin(int){} template<class...A> void printf(const char*f,A...a){ std::printf(f,a...);} void println(const char*m){std::puts(m);} };
extern SerialT Serial;
// FreeRTOS minimo
typedef void* QueueHandle_t; typedef int BaseType_t;
#define portMAX_DELAY 0xFFFFFFFF
#define pdTRUE 1
struct portMUX_TYPE { int x; };
#define portMUX_INITIALIZER_UNLOCKED {0}
#define portENTER_CRITICAL_ISR(m)
#define portEXIT_CRITICAL_ISR(m)
#define portENTER_CRITICAL(m)
#define portEXIT_CRITICAL(m)
QueueHandle_t xQueueCreate(int,int);
BaseType_t xQueueReceive(QueueHandle_t,void*,uint32_t);
BaseType_t xQueueSend(QueueHandle_t,const void*,uint32_t);
BaseType_t xTaskCreatePinnedToCore(void(*)(void*),const char*,int,void*,int,void*,int);
