// CoffeeScale: bascula de cafe con temporizador y modos de preparacion.
// Basado en el hardware y en partes del firmware de todayicreated/water-tracker (MIT).
// Ver README.md. NO HA SIDO COMPILADO NI PROBADO EN HARDWARE.
#include <Arduino.h>
#include <SPI.h>
#include <FS.h> // Exporta nombres de FS antes de que TFT_eSPI defina FS_NO_GLOBALS.
#include <TFT_eSPI.h>
#if !defined(USER_SETUP_ID) || USER_SETUP_ID != 147
#error "Falta la configuracion de pantalla: deja build_opt.h junto al sketch y usa el core ESP32 3.x."
#endif
#include <Preferences.h>
#include <WiFi.h>
#include <WiFiManager.h>      // tzapu/WiFiManager: portal para configurar la red
#include <esp_http_client.h>
#include <esp_crt_bundle.h>
#include <esp_system.h>
#include <math.h>
#include "TrackerSans-Bold15.h"
#include "TrackerSans-Bold20.h"
#include "TrackerSans-Regular12.h"
#include "Jersey10-Digits96.h"
#include "WaterUpdateAnimation.h" // Solo se usa waveAt() para la onda del cafe.
#include "src/hx711/HX711.h"

// ---------- Pines (los mismos que el Water Tracker) ----------
constexpr int TFT_BL = 22;
constexpr int ENC_A = 33, ENC_B = 32, ENC_SW = 19, BTN_PIN = 21;
constexpr int HX_DT = 16, HX_SCK = 17;
constexpr int HX_RATE = 4;   // Opcional: pin RATE del HX711 (80 SPS). Ver README.
constexpr int BAT_ADC = 35;  // Bateria: divisor 100k/100k desde B+ a GPIO35 (ADC1, solo entrada). Ver README.
constexpr float BAT_DIV = 2.0f;   // relacion del divisor (Vbat = Vpin x 2). Ajustala si mides otra tension con un polimetro.

// ---------- Colores RGB565 ----------
constexpr uint16_t BLACK = 0x0000, WHITE = 0xFFFF, GREY = 0x7BEF, DARKGREY = 0x2104,
                   YELLOW = 0xFEA0, GREEN = 0x2EE0, RED = 0xF800,
                   COFFEE = 0x59C3, COFFEE_OVER = 0x8945, CREMA = 0xCC68,
                   LEVEL = 0x28C1, LEVEL_OK = 0x1183, LEVEL_OVER = 0x38A2,   // fondo del nivel de cafe: normal / en tiempo / pasado
                   CREMA_OK = 0x3E8D, CREMA_OVER = 0xDAA9,
                   TXT_MODE = 0xCE79, TXT_DIM = 0x8C51, TXT_DIMMER = 0x5ACB, TXT_RIGHT = 0xD69A, BUBBLE = 0x5AA9;

// ---------- Modos de preparacion ----------
struct ModeDef {
  const char *name;
  float dose, ratio;        // dosis (g de cafe) y ratio (g de salida por g de cafe)
  uint16_t tMin, tMax;      // ventana de tiempo objetivo (s)
  bool autoStart, autoStop;
};
// Valores iniciales orientativos: se pueden editar en Ajustes y se guardan por modo.
const ModeDef MODES[] = {
  {"Espresso",        18, 2.0f,  25,  32, true,  true },
  {"Ristretto",       18, 1.5f,  20,  28, true,  true },
  {"Lungo",           18, 3.0f,  30,  45, true,  true },
  {"V60",             15, 16.0f, 150, 210, true, false},
  {"Aeropress",       15, 13.0f, 90,  120, true, false},
  {"Prensa francesa", 30, 15.0f, 230, 250, true, false},
  {"Libre",           0,  0.0f,  0,   0,   false, false},
};
constexpr int MODE_COUNT = sizeof(MODES) / sizeof(MODES[0]);

struct ModeCfg { float dose, ratio; uint16_t tMin, tMax; uint8_t autoStart, stopMode; };   // stopMode: 0 = a mano, 1 = al peso, 2 = sin goteo
struct Saved {
  uint32_t magic = 0x43463034;   // cambia al modificar la estructura guardada
  float cpg = 0;            // cuentas ADC por gramo (0 = sin calibrar)
  int32_t zeroRaw = 0;      // cero de la plataforma vacia
  uint8_t mode = 0;
  ModeCfg m[MODE_COUNT];
  uint8_t notify = 0;       // avisos al movil (ntfy) activados
  char topic[24] = {0};     // codigo secreto del canal ntfy (se genera en el primer arranque)
};
Saved saved;
Preferences prefs;

// ---------- Pantalla ----------
TFT_eSPI tft;
TFT_eSprite cv(&tft);  // Frame completo 320x172 (~110 KB). Sin Wi-Fi hay RAM de sobra.

// ---------- Entradas ----------
portMUX_TYPE encoderMux = portMUX_INITIALIZER_UNLOCKED;
volatile int encoderSteps = 0;
volatile uint8_t encoderPrevious = 0;
volatile int8_t encoderPartial = 0;
DRAM_ATTR static const int8_t encoderTransitions[16] = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};
void IRAM_ATTR encoderISR() {
  uint8_t current = (digitalRead(ENC_A) << 1) | digitalRead(ENC_B);
  portENTER_CRITICAL_ISR(&encoderMux);
  encoderPartial += encoderTransitions[(encoderPrevious << 2) | current];
  encoderPrevious = current;
  if (encoderPartial >= 4)  { encoderSteps++; encoderPartial = 0; }
  if (encoderPartial <= -4) { encoderSteps--; encoderPartial = 0; }
  portEXIT_CRITICAL_ISR(&encoderMux);
}
int takeSteps() {
  portENTER_CRITICAL(&encoderMux);
  int s = encoderSteps; encoderSteps = 0;
  portEXIT_CRITICAL(&encoderMux);
  return s;
}
struct Button {
  uint8_t pin; bool raw = true, stable = true, held = false;
  uint32_t changed = 0, pressed = 0;
  // 1 = pulsacion corta (al soltar), 2 = pulsacion larga (nunca va seguida de la corta)
  int poll() {
    bool value = digitalRead(pin);
    if (value != raw) { raw = value; changed = millis(); }
    if (millis() - changed >= 30 && raw != stable) {
      stable = raw;
      if (!stable) { pressed = millis(); held = false; }
      else if (!held) return 1;
    }
    if (!stable && !held && millis() - pressed >= 800) { held = true; return 2; }
    return 0;
  }
};
Button encBtn{ENC_SW}, blueBtn{BTN_PIN};

// ---------- Bascula ----------
HX711 scale;
float rawF = 0; bool haveRaw = false;     // lectura cruda filtrada
int32_t tareRaw = 0;                      // cero actual (tara)
float shown = 0;                          // peso mostrado (con histeresis)
float wf = 0;                             // peso filtrado (para caudal)
float flow = 0;                           // g/s
constexpr int HN = 10;                    // historial a ~10 Hz
float hist[HN]; uint32_t histT[HN]; int hIdx = 0; uint32_t lastHist = 0;
uint32_t lastReadMs = 0;

// ---------- Estado ----------
enum State { READY, BREW, DONE, MENU, EDIT, CAL_EMPTY, CAL_MASS, CAL_LOAD };
State state = READY;
bool dirty = true, armed = false, busy = false;
bool targetCue = false, doneInWindow = false;
uint32_t modeChangedMs = 0, tareMs = 0, doneMs = 0, targetCueMs = 0, secMs = 0, lastActivityMs = 0;
int lastSecShown = -1; float fracShown = 0;
bool dimmed = false;
// Bateria LiPo 1S (opcional): se mide por un divisor en GPIO35. Si no hay nada conectado (< 2,5 V) el icono no se muestra.
float batV = 0, batMarkV = 0; int batPct = 0; bool batPresent = false, batCharging = false, batLowSent = false, batHaveV = false;
uint32_t batLastMs = 0, batMarkMs = 0;
uint32_t startMs = 0, endMs = 0, lastFlowMs = 0, riseAt = 0, reachedAt = 0, lastDraw = 0, toastUntil = 0;
bool rising = false;
float finalWeight = 0;
String toast;
int menuRow = 0, knownMass = 100;
float draft = 0, calEmpty = 0;
constexpr int MENU_ROWS = 12; // Dosis, Ratio, T min, T max, Parar al peso, Auto inicio, Avisos movil, Wi-Fi, Codigo, Calibrar, Volver

const ModeDef &curDef() { return MODES[saved.mode]; }
ModeCfg &curCfg() { return saved.m[saved.mode]; }
float targetG() { return curCfg().dose * curCfg().ratio; }

void saveCfg() {
  if (prefs.putBytes("cfg", &saved, sizeof(saved)) != sizeof(saved))
    Serial.println("ERROR: no se pudo guardar la configuracion");
}
// Retroiluminacion por PWM (core ESP32 3.x: ledcAttach/ledcWrite). Se atenua tras un rato sin uso y vuelve con cualquier actividad.
constexpr uint32_t DIM_AFTER_MS = 120000;
constexpr uint8_t BL_FULL = 255, BL_DIM = 40;
float lastActW = 0;
void setBacklight(uint8_t duty) { ledcWrite(TFT_BL, duty); }
void activity(uint32_t now) { lastActivityMs = now; if (dimmed) { dimmed = false; setBacklight(BL_FULL); } }
// Curva aproximada de descarga de una LiPo 1S en reposo (tension -> %). Orientativa: depende de la celda y de la carga.
int lipoPercent(float v) {
  static const float V[] = {4.20f, 4.10f, 4.00f, 3.92f, 3.85f, 3.80f, 3.75f, 3.70f, 3.65f, 3.55f, 3.45f, 3.30f};
  static const int   P[] = {100,   90,    80,    70,    60,    50,    40,    30,    20,    10,    5,     0};
  if (v >= V[0]) return 100;
  for (int i = 1; i < 12; i++) if (v >= V[i]) return int(P[i] + (P[i-1] - P[i]) * (v - V[i]) / (V[i-1] - V[i]) + 0.5f);
  return 0;
}
void showToast(const String &s, uint32_t ms = 2000);
void queueNote(const char *title, const char *body);
// Lee la bateria cada 2 s (media de 16 lecturas, filtrada). Marca "cargando" si la tension sube >= 15 mV en 30 s.
void readBattery(uint32_t now) {
  if (batLastMs && now - batLastMs < 2000) return;
  batLastMs = now;
  uint32_t sum = 0; for (int i = 0; i < 16; i++) sum += analogReadMilliVolts(BAT_ADC);
  float v = sum / 16.0f / 1000.0f * BAT_DIV;
  batPresent = v > 2.5f && v < 4.6f;
  if (!batPresent) { batHaveV = false; batCharging = false; return; }
  batV = batHaveV ? batV + 0.3f * (v - batV) : v; batHaveV = true;
  batPct = lipoPercent(batV);
  if (!batMarkMs) { batMarkMs = now; batMarkV = batV; }
  if (now - batMarkMs >= 30000) {
    float dv = batV - batMarkV;
    if (dv >= 0.015f) batCharging = true; else if (dv <= -0.005f) batCharging = false;
    if (batV >= 4.18f && dv < 0.015f) batCharging = false;     // llena y estable
    batMarkMs = now; batMarkV = batV;
  }
  // avisos: una vez por descarga, por debajo del 15 %
  if (batPct <= 15 && !batCharging && !batLowSent) {
    batLowSent = true;
    if (state == READY || state == DONE) showToast("Bateria baja", 3000);
    char b[48]; snprintf(b, sizeof b, "%d %% (%.2f V). Conecta el cargador.", batPct, batV);
    queueNote("Bateria baja", b);
  }
  if (batPct >= 25 || batCharging) batLowSent = false;
}
void showToast(const String &s, uint32_t ms) { toast = s; toastUntil = millis() + ms; dirty = true; }

// ---------- Lectura de peso ----------
bool readScale(uint32_t now) {
  if (!scale.is_ready()) return false;
  float raw = scale.read();
  if (scale.getLastError() != HX711_OK || raw <= -8388607 || raw >= 8388607) return false;
  rawF = haveRaw ? rawF + 0.3f * (raw - rawF) : raw;
  haveRaw = true; lastReadMs = now;
  return true;
}
void resetHistory(float w) {
  for (int i = 0; i < HN; i++) { hist[i] = w; histT[i] = millis(); }
  hIdx = 0; flow = 0;
}
void updateWeight(uint32_t now) {
  if (saved.cpg == 0) { shown = wf = 0; return; }
  wf = (rawF - tareRaw) / saved.cpg;
  if (fabsf(wf - shown) >= 0.15f) shown = wf;   // histeresis: evita parpadeo de 0.1 g
  if (fabsf(wf - lastActW) >= 1.0f) { lastActW = wf; activity(now); }   // un cambio de peso despierta la pantalla
  if (now - lastHist >= 100) {
    lastHist = now;
    // hist[hIdx] guarda la muestra mas antigua del anillo.
    float dt = (now - histT[hIdx]) / 1000.0f;
    float f = dt > 0.3f ? (wf - hist[hIdx]) / dt : 0;
    flow += 0.3f * (f - flow);
    hist[hIdx] = wf; histT[hIdx] = now; hIdx = (hIdx + 1) % HN;
  }
}
void doTare() {
  if (!haveRaw) return;
  tareRaw = (int32_t)rawF; shown = wf = 0; resetHistory(0);
  armed = true; rising = false; tareMs = millis();
  showToast("Tara", 800);
}
float sampleRaw(int n) {
  double sum = 0; int got = 0; uint32_t t0 = millis();
  while (got < n && millis() - t0 < 8000) {
    if (scale.is_ready()) {
      float r = scale.read();
      if (scale.getLastError() == HX711_OK && r > -8388607 && r < 8388607) { sum += r; got++; }
    }
    delay(1);
  }
  return got == n ? float(sum / n) : NAN;
}

// ---------- Avisos al movil (ntfy.sh por Wi-Fi) ----------
// La bascula publica un mensaje en https://ntfy.sh/<codigo>. En el iPhone, la app gratuita "ntfy"
// suscrita a ese codigo muestra la notificacion. El envio va en una tarea aparte para no bloquear
// ni el reloj ni la lectura de la bascula. El mensaje (modo, peso, tiempo) pasa por el servidor ntfy.sh.
struct Note { char title[40]; char body[120]; };
QueueHandle_t noteQ = nullptr;
void notifyTask(void *) {
  Note n;
  for (;;) {
    if (xQueueReceive(noteQ, &n, portMAX_DELAY) != pdTRUE) continue;
    if (WiFi.status() != WL_CONNECTED) { Serial.println("ntfy: sin Wi-Fi, aviso descartado"); continue; }
    char url[96]; snprintf(url, sizeof url, "https://ntfy.sh/%s", saved.topic);
    esp_http_client_config_t cfgH = {};
    cfgH.url = url; cfgH.method = HTTP_METHOD_POST;
    cfgH.crt_bundle_attach = esp_crt_bundle_attach; cfgH.timeout_ms = 6000;
    esp_http_client_handle_t h = esp_http_client_init(&cfgH);
    if (!h) continue;
    esp_http_client_set_header(h, "Title", n.title);   // solo ASCII (sin tildes)
    esp_http_client_set_header(h, "Tags", "coffee");
    esp_http_client_set_post_field(h, n.body, strlen(n.body));
    esp_err_t e = esp_http_client_perform(h);
    Serial.printf("ntfy: %s (HTTP %d)\n", e == ESP_OK ? "enviado" : "error", esp_http_client_get_status_code(h));
    esp_http_client_cleanup(h);
  }
}
void queueNote(const char *title, const char *body) {
  if (!saved.notify || !noteQ) return;
  Note n; snprintf(n.title, sizeof n.title, "%s", title); snprintf(n.body, sizeof n.body, "%s", body);
  xQueueSend(noteQ, &n, 0);   // sin esperar: si la cola esta llena se descarta
}
void wifiApply() {            // enciende o apaga el Wi-Fi segun el ajuste
  if (saved.notify) { WiFi.mode(WIFI_STA); WiFi.setAutoReconnect(true); WiFi.setSleep(true); WiFi.begin(); }
  else { WiFi.disconnect(true); WiFi.mode(WIFI_OFF); }
}

// ---------- Temporizador / preparacion ----------
void startBrew(uint32_t at) {
  state = BREW; startMs = at; lastFlowMs = millis(); reachedAt = 0; targetCue = false; armed = false; rising = false; dirty = true;
}
void stopBrew(uint32_t end) {
  endMs = end; finalWeight = shown; state = DONE; dirty = true; doneMs = millis();
  const ModeCfg &c = curCfg(); int sec = (endMs - startMs) / 1000;
  doneInWindow = targetG() > 0 && sec >= c.tMin && sec <= c.tMax;
  char title[40], body[120];
  snprintf(title, sizeof title, "%s listo", curDef().name);
  const char *v = targetG() <= 0 ? "" : sec < c.tMin ? " (rapido)" : sec > c.tMax ? " (lento)" : " (en tiempo)";
  snprintf(body, sizeof body, "%.1f g en %d:%02d  Ratio 1:%.1f%s", finalWeight, sec / 60, sec % 60,
           c.dose > 0 ? finalWeight / c.dose : 0.0f, v);
  queueNote(title, body);
}
void resetToReady() { state = READY; armed = false; rising = false; dirty = true; }
void changeMode(int steps) {
  int m = ((int)saved.mode + steps) % MODE_COUNT;
  if (m < 0) m += MODE_COUNT;
  saved.mode = m; saveCfg(); dirty = true; modeChangedMs = millis();
}
void brewLogic(uint32_t now) {
  if (state == READY && curCfg().autoStart && armed && saved.cpg != 0) {
    // Arranca cuando el peso sube >= 1 g de forma sostenida (primer goteo / primer chorro).
    if (shown >= 0.6f) {
      if (!rising) { rising = true; riseAt = now; }
      if (shown >= 1.0f && now - riseAt >= 150) startBrew(riseAt);
    } else rising = false;
  } else if (state == BREW) {
    if (flow >= 0.4f) lastFlowMs = now;
    // Objetivo alcanzado (margen 0.2 g, sostenido 200 ms).
    float tg = targetG();
    if (tg > 0) {
      if (wf >= tg - 0.2f) {
        if (!reachedAt) reachedAt = now;
        if (now - reachedAt >= 200) {
          if (!targetCue) {
            targetCue = true; targetCueMs = now;
            if (curCfg().stopMode != 1) {   // filtro: avisa pero NO para el reloj
              showToast("Objetivo alcanzado", 3000);
              char title[40], body[120]; int s = (now - startMs) / 1000;
              snprintf(title, sizeof title, "%s: objetivo alcanzado", curDef().name);
              snprintf(body, sizeof body, "%.0f g en %d:%02d. El reloj sigue: para cuando termine de escurrir.", tg, s / 60, s % 60);
              queueNote(title, body);
            }
          }
          if (curCfg().stopMode == 1) { stopBrew(reachedAt); reachedAt = 0; return; }   // espresso: para al peso
        }
      } else reachedAt = 0;
    }
    // "Sin goteo": tras el objetivo, para cuando el caudal lleva 8 s por debajo de 0.4 g/s.
    if (curCfg().stopMode == 2 && targetCue && now - lastFlowMs >= 8000) stopBrew(lastFlowMs);
    // Parada automatica (espresso): sin caudal durante 4 s tras haber salido >= 5 g.
    if (curDef().autoStop && shown >= 5.0f && now - lastFlowMs >= 4000) stopBrew(lastFlowMs);
  }
}

// ---------- Dibujo ----------
void txt(const String &s, int x, int y, const uint8_t *font, uint16_t col, uint8_t datum = TL_DATUM) {
  cv.loadFont(font); cv.setTextDatum(datum); cv.setTextColor(col);
  cv.drawString(s, x, y); cv.unloadFont();
}
int txtW(const String &s, const uint8_t *font) {
  cv.loadFont(font); int w = cv.textWidth(s); cv.unloadFont(); return w;
}
int fontH(const uint8_t *font) {
  cv.loadFont(font); int h = cv.fontHeight(); cv.unloadFont(); return h;
}
String fmtT(int sec) {
  char b[12];
  if (sec < 100) snprintf(b, sizeof b, "%ds", sec);
  else snprintf(b, sizeof b, "%d:%02d", sec / 60, sec % 60);
  return String(b);
}
// Peso: parte entera y decimal en Bold20 (sin '.'; el punto se dibuja a mano) + "g".
void drawWeight(float g, int x, int y, uint16_t col) {
  int t = int(fabsf(g) * 10 + 0.5f);
  String ip = String((g < 0 && t > 0) ? "-" : "") + String(t / 10);
  int h = fontH(TrackerSansBold20);
  txt(ip, x, y, TrackerSansBold20, col);
  int w = txtW(ip, TrackerSansBold20);
  cv.fillCircle(x + w + 4, y + h - 6, 2, col);
  String fr = String(t % 10);
  txt(fr, x + w + 10, y, TrackerSansBold20, col);
  txt("g", x + w + 12 + txtW(fr, TrackerSansBold20), y + 5, TrackerSansBold15, GREY);
}
// Mezcla un color con el negro (a = 0..1) para fundidos y aros que se desvanecen.
uint16_t fade565(uint8_t r, uint8_t g, uint8_t b, float a) {
  a = constrain(a, 0.0f, 1.0f);
  return tft.color565(uint8_t(r * a), uint8_t(g * a), uint8_t(b * a));
}
float easeS(float t) { t = constrain(t, 0.0f, 1.0f); return t * t * (3 - 2 * t); }

struct Bubble { float x, y, r, ph; bool on; };
Bubble bubbles[9];
uint32_t nextBubbleMs = 0, lastFrameMs = 0;

// Icono de bateria (arriba a la derecha): verde > 30 %, naranja 15-30 %, rojo < 15 % (parpadea por debajo del 10 %). Rayo si carga.
void drawBattery(int x, int y, uint32_t now) {
  if (!batPresent) return;
  uint16_t col = batPct > 30 ? GREEN : batPct > 15 ? 0xFD20 : RED;
  bool blink = batPct <= 10 && !batCharging && ((now / 500) & 1);
  if (!blink) {
    cv.drawRoundRect(x, y, 28, 12, 2, col); cv.fillRect(x + 28, y + 3, 2, 6, col);
    int w = constrain((batPct * 24) / 100, 0, 24);
    if (w > 0) cv.fillRect(x + 2, y + 2, w, 8, col);
  }
  if (batCharging) { cv.drawLine(x + 16, y + 1, x + 11, y + 6, WHITE); cv.drawLine(x + 11, y + 6, x + 17, y + 6, WHITE); cv.drawLine(x + 17, y + 6, x + 12, y + 11, WHITE); }
  char b[8]; snprintf(b, sizeof b, "%d%%", batPct);
  txt(b, x - 5, y - 1, TrackerSansRegular12, TXT_DIM, TR_DATUM);
}
// Pantalla: negro, tiempo grande, peso, y un nivel de cafe que sube con una onda suave.
void drawMain(uint32_t now) {
  const ModeDef &d = curDef(); ModeCfg &c = curCfg();
  float target = targetG();
  float frac = target > 0 ? fmaxf(shown, 0.0f) / target : 0;
  float flowNow = fmaxf(flow, 0.0f);
  bool running = state == BREW;
  float dt = lastFrameMs ? fminf(0.05f, (now - lastFrameMs) / 1000.0f) : 0.033f; lastFrameMs = now;
  fracShown += (fminf(frac, 1.0f) - fracShown) * fminf(1.0f, dt * 6);        // el nivel sube suave

  // ---- nivel de cafe: color plano, onda suave y linea de crema ----
  bool over = target > 0 && frac > 1.02f;
  bool okc = (state == DONE && doneInWindow) || (running && targetCue);
  if (fracShown > 0.004f) {
    const int top = 6, bottom = 172;
    float surface = bottom - (bottom - top) * fracShown;
    float amp = 1.2f + (running ? fminf(3.2f, flowNow) : 0.0f);
    uint16_t fill = over ? LEVEL_OVER : (state == DONE && doneInWindow ? LEVEL_OK : LEVEL);
    uint16_t line = over ? CREMA_OVER : (okc ? CREMA_OK : CREMA);
    for (int x = 0; x < 320; x++) {
      float w = WaterUpdateAnimation::waveAt(x, now);
      int y = constrain(int(surface + amp * w), top, bottom - 1);
      cv.drawFastVLine(x, y, bottom - y, fill);
      cv.drawPixel(x, y, line); cv.drawPixel(x, y + 1, line);
    }
    // pocas burbujas suaves
    if (running && flowNow > 0.5f && now > nextBubbleMs) {
      for (auto &bb : bubbles) if (!bb.on) { bb = {float(30 + random(260)), 172.0f, 1.2f + random(14) / 10.0f, float(random(6))}; bb.on = true; break; }
      nextBubbleMs = now + 260;
    }
    for (auto &bb : bubbles) if (bb.on) {
      bb.y -= (16 + bb.r * 5) * dt; bb.x += sinf(now * 0.004f + bb.ph) * 5 * dt;
      if (bb.y < surface + 4) bb.on = false; else cv.drawCircle(int(bb.x), int(bb.y), int(bb.r + 0.5f), BUBBLE);
    }
  } else for (auto &bb : bubbles) bb.on = false;

  // ---- cabecera: modo (con fundido y desplazamiento al cambiar) y objetivo ----
  float k = easeS((now - modeChangedMs) / 240.0f);
  txt(d.name, 14 + int((1 - k) * 22), 10, TrackerSansRegular12, fade565(0xCE, 0xCF, 0xCE, 0.25f + 0.75f * k));
  if (target > 0) {
    char b[48];
    snprintf(b, sizeof b, "%.0f > %.0f g   %s-%s", c.dose, target, fmtT(c.tMin).c_str(), fmtT(c.tMax).c_str());
    txt(b, batPresent ? 226 : 306, 11, TrackerSansRegular12, TXT_DIM, TR_DATUM);
  }
  drawBattery(270, 9, now);

  // ---- tiempo grande (salta 3 px cada segundo) ----
  uint32_t ms = running ? now - startMs : state == DONE ? endMs - startMs : 0;
  int sec = ms / 1000, tenth = (ms / 100) % 10;
  if (sec != lastSecShown) { lastSecShown = sec; secMs = now; }
  uint16_t tcol = WHITE;
  if (target > 0 && (running || state == DONE)) {
    if (sec > c.tMax) tcol = RED;
    else if (sec >= c.tMin) tcol = GREEN;
  }
  char tb[16]; snprintf(tb, sizeof tb, "%02d:%02d", sec / 60, sec % 60);
  int tw = txtW(tb, Jersey10Digits96);
  int tick = running ? int((1 - easeS((now - secMs) / 170.0f)) * 3) : 0;
  txt(tb, (320 - tw) / 2, 20 - tick, Jersey10Digits96, tcol);
  if (running || state == DONE) {
    cv.fillCircle((320 + tw) / 2 + 8, 88, 2, TXT_DIM);
    txt(String(tenth), (320 + tw) / 2 + 14, 70, TrackerSansBold15, TXT_DIM);
  }

  // ---- peso (izquierda) y caudal / ratio / aviso (derecha) ----
  drawWeight(shown, 14, 126, WHITE);
  String right;
  char rb[24];
  if (millis() < toastUntil) right = toast;
  else if (running && targetCue && c.stopMode != 1) right = "objetivo";
  else if (running) { snprintf(rb, sizeof rb, "%.1f g/s", flowNow); right = rb; }
  else if (state == DONE && c.dose > 0) { snprintf(rb, sizeof rb, "1:%.1f", finalWeight / c.dose); right = rb; }
  if (right.length()) txt(right, 306, 134, TrackerSansBold15, TXT_RIGHT, TR_DATUM);
  // pista discreta solo cuando hace falta
  if (state == READY) {
    if (batPresent && batPct <= 5 && !batCharging) txt("Bateria critica: conecta el cargador", 160, 152, TrackerSansRegular12, RED, TC_DATUM);
    else if (saved.cpg == 0) txt("Sin calibrar: azul largo > Ajustes", 160, 152, TrackerSansRegular12, RED, TC_DATUM);
    else if (!armed) txt("pomo: tara", 160, 154, TrackerSansRegular12, TXT_DIMMER, TC_DATUM);
    else { cv.fillCircle(160, 18, 3, c.autoStart ? GREEN : GREY); }
    if (!armed) cv.drawCircle(160, 18, 3, GREY);
  }

  // ---- tara: aro azul alrededor del peso ----
  float tt = (now - tareMs) / 600.0f;
  if (tareMs && tt >= 0 && tt < 1)
    cv.drawRoundRect(10 - int(tt * 6), 120 - int(tt * 6), 80 + int(tt * 12), 34 + int(tt * 12), 8, fade565(80, 150, 255, 1 - tt));
  // ---- objetivo alcanzado con el reloj en marcha (filtro): aro verde, sin parar ----
  float cr = (now - targetCueMs) / 800.0f;
  if (running && targetCueMs && cr >= 0 && cr < 1) cv.drawCircle(160, 86, 20 + int(cr * 190), fade565(63, 211, 106, 1 - cr));
  // ---- reloj parado: pulso (aro que se expande) ----
  float pr = (now - doneMs) / 800.0f;
  if (state == DONE && target > 0 && doneMs && pr >= 0 && pr < 1)
    cv.drawCircle(160, 86, 20 + int(pr * 190), doneInWindow ? fade565(63, 211, 106, 1 - pr) : fade565(255, 140, 60, 1 - pr));
}
String menuLabel(int r) {
  switch (r) {
    case 0: return "Dosis";
    case 1: return "Ratio";
    case 2: return "Tiempo min";
    case 3: return "Tiempo max";
    case 4: return "Parar";
    case 5: return "Auto inicio";
    case 6: return "Avisos movil";
    case 7: return "Wi-Fi";
    case 8: return "Codigo";
    case 9: return "Bateria";
    case 10: return "Calibrar";
    default: return "Volver";
  }
}
String menuValue(int r) {
  ModeCfg &c = curCfg(); char b[16];
  bool editing = state == EDIT && r == menuRow;
  switch (r) {
    case 0: snprintf(b, sizeof b, "%.1f g", editing ? draft : c.dose); return b;
    case 1: snprintf(b, sizeof b, "1:%.1f", editing ? draft : c.ratio); return b;
    case 2: return fmtT(editing ? int(draft) : c.tMin);
    case 3: return fmtT(editing ? int(draft) : c.tMax);
    case 4: { const char *n[] = {"a mano", "al peso", "sin goteo"}; return n[c.stopMode % 3]; }
    case 5: return c.autoStart ? "SI" : "NO";
    case 6: return saved.notify ? "SI" : "NO";
    case 7: return WiFi.status() == WL_CONNECTED ? "conectado" : "sin red";
    case 8: return String(saved.topic);
    case 9: { if (!batPresent) return "sin bateria"; snprintf(b, sizeof b, "%d%% %.2fV", batPct, batV); return b; }
    default: return "";
  }
}
void drawMenu() {
  txt(String("Ajustes: ") + curDef().name, 8, 4, TrackerSansBold15, YELLOW);
  if (millis() < toastUntil) txt(toast, 312, 6, TrackerSansRegular12, WHITE, TR_DATUM);
  const int visible = 5, pitch = 26, y0 = 30;
  int first = constrain(menuRow - 2, 0, MENU_ROWS - visible);
  for (int i = 0; i < visible; i++) {
    int r = first + i, y = y0 + i * pitch;
    if (r == menuRow) cv.fillRoundRect(4, y - 2, 312, pitch - 2, 5, state == EDIT ? COFFEE : GREY);
    txt(menuLabel(r), 12, y + 2, TrackerSansBold15, r == menuRow ? BLACK : WHITE);
    txt(menuValue(r), 306, y + 2, TrackerSansBold15, state == EDIT && r == menuRow ? WHITE : (r == menuRow ? BLACK : WHITE), TR_DATUM);
  }
}
void drawCal() {
  txt("Calibracion", 8, 4, TrackerSansBold15, YELLOW);
  if (state == CAL_EMPTY) {
    txt("Quita todo de la plataforma", 160, 60, TrackerSansBold15, WHITE, TC_DATUM);
    txt("y pulsa el pomo", 160, 84, TrackerSansBold15, WHITE, TC_DATUM);
  } else if (state == CAL_MASS) {
    txt("Peso conocido (gira el pomo)", 160, 40, TrackerSansBold15, WHITE, TC_DATUM);
    drawWeight(knownMass, 100, 80, YELLOW);
    txt("Pulsa para continuar", 160, 130, TrackerSansRegular12, GREY, TC_DATUM);
  } else {
    txt("Coloca ese peso y pulsa", 160, 60, TrackerSansBold15, WHITE, TC_DATUM);
    txt("el pomo cuando este quieto", 160, 84, TrackerSansBold15, WHITE, TC_DATUM);
  }
}
void draw() {
  uint32_t now = millis();
  cv.fillSprite(BLACK);
  if (state == MENU || state == EDIT) drawMenu();
  else if (state >= CAL_EMPTY) drawCal();
  else drawMain(now);
  if (busy) {
    cv.fillRoundRect(60, 56, 200, 50, 8, DARKGREY);
    txt("Midiendo...", 160, 72, TrackerSansBold15, WHITE, TC_DATUM);
  }
  cv.pushSprite(0, 0);
  lastDraw = now; dirty = false;
}

// ---------- Entrada por estado ----------
void runWifiPortal() {
  cv.fillSprite(BLACK);
  txt("Conecta el movil a la red", 160, 30, TrackerSansBold15, WHITE, TC_DATUM);
  txt("CoffeeScale-Setup", 160, 54, TrackerSansBold15, YELLOW, TC_DATUM);
  txt("clave: cafecafe", 160, 80, TrackerSansRegular12, WHITE, TC_DATUM);
  txt("y abre 192.168.4.1 (3 min)", 160, 104, TrackerSansRegular12, WHITE, TC_DATUM);
  cv.pushSprite(0, 0);
  WiFiManager wm; wm.setConfigPortalTimeout(180); wm.setConnectTimeout(20);
  bool ok = wm.startConfigPortal("CoffeeScale-Setup", "cafecafe");   // bloquea hasta terminar o caducar
  wifiApply();
  showToast(ok ? "Wi-Fi conectado" : "Wi-Fi sin cambios", 2500);
}
void openMenu() { state = MENU; menuRow = 0; dirty = true; }
void handleInput(uint32_t now) {
  int steps = takeSteps(), e = encBtn.poll(), b = blueBtn.poll();
  if (steps || e || b) { dirty = true; activity(now); }
  switch (state) {
    case READY:
      if (steps) changeMode(steps);
      if (e == 1) doTare();
      if (b == 1) startBrew(now);
      if (b == 2) openMenu();
      break;
    case BREW:
      if (e == 1) doTare();            // re-tara sin parar el reloj (util en pour-over)
      if (b == 1) stopBrew(now);
      break;
    case DONE:
      if (e == 1) { doTare(); resetToReady(); }
      if (b == 1) resetToReady();
      if (b == 2) openMenu();
      break;
    case MENU:
      if (steps) menuRow = constrain(menuRow + (steps > 0 ? 1 : -1), 0, MENU_ROWS - 1);
      if (b) { resetToReady(); break; }
      if (e == 1) {
        ModeCfg &c = curCfg();
        if (menuRow == 0) { draft = c.dose; state = EDIT; }
        else if (menuRow == 1) { draft = c.ratio; state = EDIT; }
        else if (menuRow == 2) { draft = c.tMin; state = EDIT; }
        else if (menuRow == 3) { draft = c.tMax; state = EDIT; }
        else if (menuRow == 4) { c.stopMode = (c.stopMode + 1) % 3; saveCfg(); }
        else if (menuRow == 5) { c.autoStart = !c.autoStart; saveCfg(); }
        else if (menuRow == 6) { saved.notify = !saved.notify; saveCfg(); wifiApply();
                                 if (saved.notify && WiFi.SSID().length() == 0) showToast("Falta configurar el Wi-Fi", 2500); }
        else if (menuRow == 7) runWifiPortal();
        else if (menuRow == 8) showToast(String("ntfy: ") + saved.topic, 5000);
        else if (menuRow == 9) { /* solo informacion */ }
        else if (menuRow == 10) state = CAL_EMPTY;
        else resetToReady();
      }
      break;
    case EDIT:
      if (steps) {
        if (menuRow == 0) draft = constrain(draft + 0.5f * steps, 0.0f, 100.0f);
        else if (menuRow == 1) draft = constrain(draft + 0.1f * steps, 0.0f, 30.0f);
        else draft = constrain(draft + 1.0f * steps, 0.0f, 900.0f);
      }
      if (b) state = MENU;               // cancelar sin guardar
      if (e == 1) {
        ModeCfg &c = curCfg();
        if (menuRow == 0) c.dose = draft;
        else if (menuRow == 1) c.ratio = draft;
        else if (menuRow == 2) c.tMin = (uint16_t)draft;
        else if (menuRow == 3) c.tMax = (uint16_t)draft;
        if (c.tMax < c.tMin) c.tMax = c.tMin;
        saveCfg(); state = MENU;
      }
      break;
    case CAL_EMPTY:
      if (b) { state = MENU; break; }
      if (e == 1) {
        busy = true; draw(); float r = sampleRaw(40); busy = false;
        if (isnan(r)) { showToast("Error HX711"); state = MENU; }
        else { calEmpty = r; knownMass = 100; state = CAL_MASS; }
      }
      break;
    case CAL_MASS:
      if (b) { state = MENU; break; }
      if (steps) knownMass = constrain(knownMass + 5 * steps, 5, 5000);
      if (e == 1) state = CAL_LOAD;
      break;
    case CAL_LOAD:
      if (b) { state = MENU; break; }
      if (e == 1) {
        busy = true; draw(); float r = sampleRaw(40); busy = false;
        float cpg = isnan(r) ? 0 : (r - calEmpty) / knownMass;
        if (fabsf(cpg) < 50) { showToast("Peso no detectado"); state = MENU; }
        else {
          saved.cpg = cpg; saved.zeroRaw = (int32_t)calEmpty; tareRaw = saved.zeroRaw;
          saveCfg(); resetHistory(0); showToast("Calibrado");
          resetToReady();
        }
      }
      break;
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(ENC_A, INPUT_PULLUP); pinMode(ENC_B, INPUT_PULLUP);
  pinMode(ENC_SW, INPUT_PULLUP); pinMode(BTN_PIN, INPUT_PULLUP);
  encoderPrevious = (digitalRead(ENC_A) << 1) | digitalRead(ENC_B);
  attachInterrupt(digitalPinToInterrupt(ENC_A), encoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_B), encoderISR, CHANGE);

  analogSetPinAttenuation(BAT_ADC, ADC_11db);   // rango hasta ~3,1 V en el pin (la bateria a 4,2 V llega como 2,1 V)
  ledcAttach(TFT_BL, 5000, 8); setBacklight(0);
  delay(150);
  tft.init(); tft.setRotation(3); // Horizontal, girada 180 grados (igual que el Water Tracker).
  tft.writecommand(TFT_MADCTL);
  tft.writedata(TFT_MAD_MV | TFT_MAD_MY | TFT_MAD_BGR);
  cv.setColorDepth(16);
  if (!cv.createSprite(320, 172)) {
    tft.fillScreen(BLACK); setBacklight(BL_FULL);
    tft.setTextColor(WHITE, BLACK); tft.setCursor(10, 30);
    tft.print("Sin memoria para el frame");
    while (true) delay(1000);
  }
  cv.setTextWrap(false);
  setBacklight(BL_FULL); lastActivityMs = millis();

  // Configuracion por defecto y carga desde flash.
  for (int i = 0; i < MODE_COUNT; i++)
    saved.m[i] = {MODES[i].dose, MODES[i].ratio, MODES[i].tMin, MODES[i].tMax, (uint8_t)MODES[i].autoStart,
                  (uint8_t)(MODES[i].autoStop ? 1 : 0)};   // espresso/ristretto/lungo: al peso; filtro: a mano
  if (prefs.begin("coffee", false)) {
    Saved loaded;
    if (prefs.getBytesLength("cfg") == sizeof(loaded) &&
        prefs.getBytes("cfg", &loaded, sizeof(loaded)) == sizeof(loaded) &&
        loaded.magic == saved.magic && loaded.mode < MODE_COUNT && isfinite(loaded.cpg)) saved = loaded;
  }
  if (saved.topic[0] == 0) {   // codigo secreto de 64 bits en el primer arranque
    snprintf(saved.topic, sizeof saved.topic, "cafe-%08x%08x", (unsigned)esp_random(), (unsigned)esp_random());
    saveCfg();
  }
  Serial.printf("Codigo ntfy: %s\n", saved.topic);
  noteQ = xQueueCreate(4, sizeof(Note));
  xTaskCreatePinnedToCore(notifyTask, "ntfy", 8192, nullptr, 1, nullptr, 0);
  wifiApply();
  tareRaw = saved.zeroRaw;

  scale.begin(HX_DT, HX_SCK, true, false);   // timing ESP32, sin reset bloqueante
  scale.set_rate_pin(HX_RATE);               // inofensivo si el pin RATE no esta cableado
  scale.set_rate_80SPS();
  resetHistory(0);
  dirty = true;
}

void loop() {
  uint32_t now = millis();
  if (readScale(now)) updateWeight(now);
  handleInput(now);
  brewLogic(now);
  readBattery(now);
  if (state != READY) lastActivityMs = now;                       // preparando, terminado o en menus: no se atenua
  if (state == READY && !dimmed && now - lastActivityMs > DIM_AFTER_MS) { dimmed = true; setBacklight(BL_DIM); }
  bool animate = state <= DONE;
  uint32_t frameMs = dimmed ? 500 : 33;                           // atenuada: casi sin redibujar
  if ((animate && now - lastDraw >= frameMs) || (dirty && now - lastDraw >= 20) ||
      (toastUntil && now > toastUntil)) {
    if (toastUntil && now > toastUntil) toastUntil = 0;
    draw();
  }
}
