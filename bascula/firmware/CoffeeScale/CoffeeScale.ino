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

// ---------- Colores RGB565 ----------
constexpr uint16_t BLACK = 0x0000, WHITE = 0xFFFF, GREY = 0x7BEF, DARKGREY = 0x2104,
                   YELLOW = 0xFEA0, GREEN = 0x2EE0, RED = 0xF800,
                   COFFEE = 0x59C3, COFFEE_OVER = 0x8945, CREMA = 0xCC68;

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

struct ModeCfg { float dose, ratio; uint16_t tMin, tMax; uint8_t autoStart, stopAtWeight; };
struct Saved {
  uint32_t magic = 0x43463032;   // cambia al modificar la estructura guardada
  float cpg = 0;            // cuentas ADC por gramo (0 = sin calibrar)
  int32_t zeroRaw = 0;      // cero de la plataforma vacia
  uint8_t mode = 0;
  ModeCfg m[MODE_COUNT];
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
uint32_t startMs = 0, endMs = 0, lastFlowMs = 0, riseAt = 0, reachedAt = 0, lastDraw = 0, toastUntil = 0;
bool rising = false;
float finalWeight = 0;
String toast;
int menuRow = 0, knownMass = 100;
float draft = 0, calEmpty = 0;
constexpr int MENU_ROWS = 8; // Dosis, Ratio, T min, T max, Parar al peso, Auto inicio, Calibrar, Volver

const ModeDef &curDef() { return MODES[saved.mode]; }
ModeCfg &curCfg() { return saved.m[saved.mode]; }
float targetG() { return curCfg().dose * curCfg().ratio; }

void saveCfg() {
  if (prefs.putBytes("cfg", &saved, sizeof(saved)) != sizeof(saved))
    Serial.println("ERROR: no se pudo guardar la configuracion");
}
void showToast(const String &s, uint32_t ms = 2000) { toast = s; toastUntil = millis() + ms; dirty = true; }

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
  armed = true; rising = false;
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

// ---------- Temporizador / preparacion ----------
void startBrew(uint32_t at) {
  state = BREW; startMs = at; lastFlowMs = millis(); reachedAt = 0; armed = false; rising = false; dirty = true;
}
void stopBrew(uint32_t end) {
  endMs = end; finalWeight = shown; state = DONE; dirty = true;
}
void resetToReady() { state = READY; armed = false; rising = false; dirty = true; }
void changeMode(int steps) {
  int m = ((int)saved.mode + steps) % MODE_COUNT;
  if (m < 0) m += MODE_COUNT;
  saved.mode = m; saveCfg(); dirty = true;
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
    // Parar al peso: al alcanzar el objetivo (margen 0.2 g, sostenido 200 ms) se para el reloj.
    float tg = targetG();
    if (curCfg().stopAtWeight && tg > 0) {
      if (wf >= tg - 0.2f) {
        if (!reachedAt) reachedAt = now;
        if (now - reachedAt >= 200) { stopBrew(reachedAt); reachedAt = 0; return; }
      } else reachedAt = 0;
    }
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
void drawLiquid(uint32_t now, float frac, uint16_t col) {
  if (frac <= 0.005f) return;
  const int top = 30, bottom = 172;
  float f = fminf(frac, 1.0f);
  float surface = bottom - (bottom - top) * f;
  float amp = fminf(5.0f, fminf(surface - top, bottom - surface) / 3.0f);
  for (int x = 0; x < 320; x++) {
    int y = int(surface + amp * WaterUpdateAnimation::waveAt(x, now));
    y = constrain(y, top, bottom - 1);
    cv.drawFastVLine(x, y, bottom - y, col);
    cv.drawPixel(x, y, CREMA);
  }
}
void drawMain(uint32_t now) {
  const ModeDef &d = curDef(); ModeCfg &c = curCfg();
  float target = targetG();
  float frac = target > 0 ? shown / target : 0;
  drawLiquid(now, frac, frac > 1.02f ? COFFEE_OVER : COFFEE);

  // Cabecera: modo + objetivo
  txt(d.name, 8, 4, TrackerSansBold15, YELLOW);
  if (target > 0) {
    char b[48];
    snprintf(b, sizeof b, "%.0fg > %.0fg  %s-%s", c.dose, target, fmtT(c.tMin).c_str(), fmtT(c.tMax).c_str());
    txt(b, 312, 6, TrackerSansRegular12, WHITE, TR_DATUM);
  }

  // Tiempo grande
  uint32_t ms = state == BREW ? now - startMs : state == DONE ? endMs - startMs : 0;
  int sec = ms / 1000, tenth = (ms / 100) % 10;
  char tb[8]; snprintf(tb, sizeof tb, "%02d:%02d", sec / 60, sec % 60);
  uint16_t tcol = WHITE;
  if (target > 0 && (state == BREW || state == DONE)) {
    if (sec > c.tMax) tcol = RED;
    else if (sec >= c.tMin) tcol = GREEN;
  }
  int tw = txtW(tb, Jersey10Digits96), th = fontH(Jersey10Digits96);
  int x0 = (320 - (tw + 26)) / 2;
  txt(tb, x0, 22, Jersey10Digits96, tcol);
  cv.fillCircle(x0 + tw + 6, 22 + th - 14, 3, tcol);
  txt(String(tenth), x0 + tw + 12, 22 + th - 26, TrackerSansBold20, tcol);

  // Peso y caudal
  drawWeight(shown, 10, 128, WHITE);
  if (state == BREW || state == DONE) {
    char b[16]; snprintf(b, sizeof b, "%.1f g/s", fmaxf(flow, 0.0f));
    txt(b, 312, 134, TrackerSansBold15, WHITE, TR_DATUM);
  }

  // Linea de estado
  String hint;
  if (millis() < toastUntil) hint = toast;
  else if (saved.cpg == 0) hint = "Sin calibrar: manten el boton azul";
  else if (state == READY) hint = armed && c.autoStart ? "Listo: arranca solo al caer cafe" : "Pulsa pomo: tara   Azul: inicio";
  else if (state == BREW) hint = "Azul: parar   Pomo: tara";
  else {
    char b[64];
    float ratio = c.dose > 0 ? finalWeight / c.dose : 0;
    const char *verdict = target <= 0 ? "" : sec < c.tMin ? "  rapido" : sec > c.tMax ? "  lento" : "  en tiempo";
    snprintf(b, sizeof b, "%.1fg  Ratio 1:%.1f%s", finalWeight, ratio, verdict);
    hint = b;
  }
  txt(hint, 160, 156, TrackerSansRegular12, WHITE, TC_DATUM);
}
String menuLabel(int r) {
  switch (r) {
    case 0: return "Dosis";
    case 1: return "Ratio";
    case 2: return "Tiempo min";
    case 3: return "Tiempo max";
    case 4: return "Parar al peso";
    case 5: return "Auto inicio";
    case 6: return "Calibrar";
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
    case 4: return c.stopAtWeight ? "SI" : "NO";
    case 5: return c.autoStart ? "SI" : "NO";
    default: return "";
  }
}
void drawMenu() {
  txt(String("Ajustes: ") + curDef().name, 8, 4, TrackerSansBold15, YELLOW);
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
void openMenu() { state = MENU; menuRow = 0; dirty = true; }
void handleInput(uint32_t now) {
  int steps = takeSteps(), e = encBtn.poll(), b = blueBtn.poll();
  if (steps || e || b) dirty = true;
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
        else if (menuRow == 4) { c.stopAtWeight = !c.stopAtWeight; saveCfg(); }
        else if (menuRow == 5) { c.autoStart = !c.autoStart; saveCfg(); }
        else if (menuRow == 6) state = CAL_EMPTY;
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

  pinMode(TFT_BL, OUTPUT); digitalWrite(TFT_BL, LOW);
  delay(150);
  tft.init(); tft.setRotation(3); // Horizontal, girada 180 grados (igual que el Water Tracker).
  tft.writecommand(TFT_MADCTL);
  tft.writedata(TFT_MAD_MV | TFT_MAD_MY | TFT_MAD_BGR);
  cv.setColorDepth(16);
  if (!cv.createSprite(320, 172)) {
    tft.fillScreen(BLACK); digitalWrite(TFT_BL, HIGH);
    tft.setTextColor(WHITE, BLACK); tft.setCursor(10, 30);
    tft.print("Sin memoria para el frame");
    while (true) delay(1000);
  }
  cv.setTextWrap(false);
  digitalWrite(TFT_BL, HIGH);

  // Configuracion por defecto y carga desde flash.
  for (int i = 0; i < MODE_COUNT; i++)
    saved.m[i] = {MODES[i].dose, MODES[i].ratio, MODES[i].tMin, MODES[i].tMax, (uint8_t)MODES[i].autoStart,
                  (uint8_t)(MODES[i].dose > 0)};   // parar al peso: activado en los modos con objetivo
  if (prefs.begin("coffee", false)) {
    Saved loaded;
    if (prefs.getBytesLength("cfg") == sizeof(loaded) &&
        prefs.getBytes("cfg", &loaded, sizeof(loaded)) == sizeof(loaded) &&
        loaded.magic == saved.magic && loaded.mode < MODE_COUNT && isfinite(loaded.cpg)) saved = loaded;
  }
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
  bool animate = state <= DONE;
  if ((animate && now - lastDraw >= 33) || (dirty && now - lastDraw >= 20) ||
      (toastUntil && now > toastUntil)) {
    if (toastUntil && now > toastUntil) toastUntil = 0;
    draw();
  }
}
