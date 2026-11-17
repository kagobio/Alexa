#pragma once
#include <stdint.h>
#include <math.h>
static constexpr int16_t WATER_SINE[256]={
0, 804, 1608, 2410, 3212, 4011, 4808, 5602, 6393, 7179, 7962, 8739, 9512, 10278, 11039, 11793,
12539, 13279, 14010, 14732, 15446, 16151, 16846, 17530, 18204, 18868, 19519, 20159, 20787, 21403, 22005, 22594,
23170, 23731, 24279, 24811, 25329, 25832, 26319, 26790, 27245, 27683, 28105, 28510, 28898, 29268, 29621, 29956,
30273, 30571, 30852, 31113, 31356, 31580, 31785, 31971, 32137, 32285, 32412, 32521, 32609, 32678, 32728, 32757,
32767, 32757, 32728, 32678, 32609, 32521, 32412, 32285, 32137, 31971, 31785, 31580, 31356, 31113, 30852, 30571,
30273, 29956, 29621, 29268, 28898, 28510, 28105, 27683, 27245, 26790, 26319, 25832, 25329, 24811, 24279, 23731,
23170, 22594, 22005, 21403, 20787, 20159, 19519, 18868, 18204, 17530, 16846, 16151, 15446, 14732, 14010, 13279,
12539, 11793, 11039, 10278, 9512, 8739, 7962, 7179, 6393, 5602, 4808, 4011, 3212, 2410, 1608, 804,
0, -804, -1608, -2410, -3212, -4011, -4808, -5602, -6393, -7179, -7962, -8739, -9512, -10278, -11039, -11793,
-12539, -13279, -14010, -14732, -15446, -16151, -16846, -17530, -18204, -18868, -19519, -20159, -20787, -21403, -22005, -22594,
-23170, -23731, -24279, -24811, -25329, -25832, -26319, -26790, -27245, -27683, -28105, -28510, -28898, -29268, -29621, -29956,
-30273, -30571, -30852, -31113, -31356, -31580, -31785, -31971, -32137, -32285, -32412, -32521, -32609, -32678, -32728, -32757,
-32767, -32757, -32728, -32678, -32609, -32521, -32412, -32285, -32137, -31971, -31785, -31580, -31356, -31113, -30852, -30571,
-30273, -29956, -29621, -29268, -28898, -28510, -28105, -27683, -27245, -26790, -26319, -25832, -25329, -24811, -24279, -23731,
-23170, -22594, -22005, -21403, -20787, -20159, -19519, -18868, -18204, -17530, -16846, -16151, -15446, -14732, -14010, -13279,
-12539, -11793, -11039, -10278, -9512, -8739, -7962, -7179, -6393, -5602, -4808, -4011, -3212, -2410, -1608, -804
};
struct WaterDirtySpan {int top,bottom;};
struct WaterUpdateAnimation {
  static constexpr int topLimit=40,bottom=172;
  static constexpr uint32_t riseMs=1100,holdMs=250,fallMs=1300,frameMs=33;
  static constexpr uint32_t durationMs=riseMs+holdMs+fallMs;
  static constexpr int waveLength=168,amplitude=14,blockWidth=8,blocks=320/blockWidth;
  static constexpr uint32_t wavePeriodMs=1500;
  bool active=false,covered=false;
  uint32_t began=0,lastFrame=0,lastElapsed=0;
  int top=bottom;
  void start(uint32_t now){active=true;covered=false;began=now;lastFrame=now-frameMs;lastElapsed=0;top=bottom;}
  void stop(){active=false;covered=false;top=bottom;}
  bool frameDue(uint32_t now)const{return active && uint32_t(now-lastFrame)>=frameMs;}
  static float eased(float t){return t*t*(3.0f-2.0f*t);}
  static float meanAt(uint32_t elapsed){
    if(elapsed<riseMs) return bottom-(bottom-topLimit)*eased(elapsed/float(riseMs));
    if(elapsed<riseMs+holdMs) return topLimit;
    if(elapsed>=durationMs) return bottom;
    return topLimit+(bottom-topLimit)*eased((elapsed-riseMs-holdMs)/float(fallMs));
  }
  static int topAt(uint32_t elapsed){return int(meanAt(elapsed)+0.5f);}
  static float waveAt(int x,uint32_t elapsed){
    // Fixed-point lookup with interpolation avoids hundreds of sinf() calls/frame.
    uint32_t phase=(uint32_t(x)*65536/waveLength-
                    (elapsed%wavePeriodMs)*65536/wavePeriodMs)&65535;
    unsigned index=phase>>8,fraction=phase&255;
    int32_t sample=int32_t(WATER_SINE[index])*(256-int(fraction))+
                   int32_t(WATER_SINE[(index+1)&255])*int(fraction);
    return sample/(32767.0f*256.0f);
  }
  static float surfaceAt(int x,uint32_t elapsed){
    float mean=meanAt(elapsed);
    float distance=fminf(mean-topLimit,bottom-mean);
    float taper=fminf(1.0f,fmaxf(0.0f,distance/20.0f));
    return fminf(float(bottom),fmaxf(float(topLimit),mean+amplitude*eased(taper)*waveAt(x,elapsed)));
  }
};
