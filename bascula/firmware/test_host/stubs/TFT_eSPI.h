#pragma once
#include <Arduino.h>
#define TL_DATUM 0
#define TC_DATUM 1
#define TR_DATUM 2
#define TFT_MADCTL 0x36
#define TFT_MAD_MV 0x20
#define TFT_MAD_MY 0x80
#define TFT_MAD_BGR 0x08
struct TFT_eSPI {
  void init(){} void setRotation(int){} void writecommand(int){} void writedata(int){} void fillScreen(uint16_t){}
  void setTextColor(uint16_t,uint16_t){} void setCursor(int,int){} void print(const char*){}
  uint16_t color565(uint8_t r,uint8_t g,uint8_t b){ return ((r&0xF8)<<8)|((g&0xFC)<<3)|(b>>3); }
};
extern std::vector<std::string> g_frame_text; extern int g_frames;
struct TFT_eSprite {
  TFT_eSprite(TFT_eSPI*){}
  void setColorDepth(int){} bool createSprite(int,int){return true;} void setTextWrap(bool){}
  void fillSprite(uint16_t){ g_frame_text.clear(); }
  void pushSprite(int,int){ g_frames++; }
  void loadFont(const uint8_t*){} void unloadFont(){} void setTextDatum(int){}
  void setTextColor(uint16_t){} void setTextColor(uint16_t,uint16_t){}
  int drawString(const String&s,int,int){ g_frame_text.push_back(s.s); return (int)s.length()*8; }
  int textWidth(const String&s){ return (int)s.length()*8; } int fontHeight(){ return 20; }
  void fillCircle(int,int,int,uint16_t){} void drawCircle(int,int,int,uint16_t){}
  void fillRoundRect(int,int,int,int,int,uint16_t){} void drawRoundRect(int,int,int,int,int,uint16_t){}
  void drawFastVLine(int,int,int,uint16_t){} void drawPixel(int,int,uint16_t){}
};
