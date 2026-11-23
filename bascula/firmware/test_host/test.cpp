// Prueba en el servidor del firmware real (CoffeeScale.ino) con piezas falsas de Arduino.
#include <Arduino.h>
#include <deque>
#include <TFT_eSPI.h>
#include <Preferences.h>
#include <WiFi.h>
#include <esp_http_client.h>
#include <esp_crt_bundle.h>
#include <esp_system.h>
#include "src/hx711/HX711.h"
static uint32_t g_ms = 0; uint32_t millis(){ return g_ms; } void delay(uint32_t d){ g_ms += d; }
static int g_pin[64]; int digitalRead(int p){ return g_pin[p]; } void pinMode(int,int){} void digitalWrite(int,int){}
static void (*g_isr)() = nullptr; void attachInterrupt(int,void(*f)(),int){ g_isr = f; }
long random(long m){ return rand() % m; }
static int g_bl = -1; void ledcAttach(int,int,int){} void ledcWrite(int,int d){ g_bl = d; }
SerialT Serial;
struct Q { int sz; std::vector<std::vector<uint8_t>> items; };
QueueHandle_t xQueueCreate(int,int sz){ Q* q = new Q; q->sz = sz; return q; }
BaseType_t xQueueReceive(QueueHandle_t,void*,uint32_t){ return 0; }
BaseType_t xQueueSend(QueueHandle_t h,const void* it,uint32_t){ Q* q=(Q*)h; q->items.emplace_back((const uint8_t*)it,(const uint8_t*)it+q->sz); return 1; }
BaseType_t xTaskCreatePinnedToCore(void(*)(void*),const char*,int,void*,int,void*,int){ return 1; }
std::vector<std::string> g_frame_text; int g_frames = 0;
static std::map<std::string,std::vector<uint8_t>> g_store;
size_t Preferences::getBytesLength(const char*k){ return g_store.count(k)? g_store[k].size() : 0; }
size_t Preferences::getBytes(const char*k,void*b,size_t n){ if(!g_store.count(k)) return 0; memcpy(b,g_store[k].data(),std::min(n,g_store[k].size())); return n; }
size_t Preferences::putBytes(const char*k,const void*b,size_t n){ g_store[k].assign((const uint8_t*)b,(const uint8_t*)b+n); return n; }
int g_wifi_status = 3, g_wifi_on = 0, g_wifi_calls = 0; WiFiClass WiFi;
esp_http_client_handle_t esp_http_client_init(const esp_http_client_config_t*){ return (void*)1; }
void esp_http_client_set_header(esp_http_client_handle_t,const char*,const char*){} void esp_http_client_set_post_field(esp_http_client_handle_t,const char*,int){}
esp_err_t esp_http_client_perform(esp_http_client_handle_t){ return ESP_OK; } int esp_http_client_get_status_code(esp_http_client_handle_t){ return 200; } void esp_http_client_cleanup(esp_http_client_handle_t){}
esp_err_t esp_crt_bundle_attach(void*){ return 0; } uint32_t esp_random(){ return 0x1234ABCDu; }
float g_raw = 0;

#include "CoffeeScale_sketch.inc"

// ---------------- banco de pruebas ----------------
static float g_weight = 0;       // gramos reales sobre la plataforma
static int fails = 0, checks = 0;
#define CHECK(c, ...) do { checks++; if (!(c)) { fails++; printf("  FALLO: "); printf(__VA_ARGS__); printf("\n"); } } while (0)
static void step(uint32_t ms) { for (uint32_t t = 0; t < ms; t += 12) { g_ms += 12; g_raw = g_weight * saved.cpg + ((rand() % 100) - 50) * 0.001f * saved.cpg; loop(); } }
static void press(int pin, uint32_t hold) { g_pin[pin] = 0; step(hold); g_pin[pin] = 1; step(120); }
static void rotateClick(int dir) {   // un "clic" del encoder = 4 transiciones de cuadratura
  int seq_up[] = {2,3,1,0}, seq_dn[] = {1,3,2,0};
  for (int i = 0; i < 4; i++) { int st = dir > 0 ? seq_up[i] : seq_dn[i]; g_pin[ENC_A] = (st >> 1) & 1; g_pin[ENC_B] = st & 1; g_isr(); }
  step(60);
}
static bool frameHas(const char* needle) { for (auto& s : g_frame_text) if (s.find(needle) != std::string::npos) return true; return false; }
static Q* noteQueue() { return (Q*)noteQ; }
static int notes() { return (int)noteQueue()->items.size(); }
static Note lastNote() { Note n; memcpy(&n, noteQueue()->items.back().data(), sizeof n); return n; }

// vierte `total` gramos con un caudal que arranca tras `delay_s` y se estabiliza en `rate` g/s
static void pour(float total, float rate, float delay_s) {
  float added = 0; step((uint32_t)(delay_s * 1000));
  while (added < total) { float d = rate * 0.012f; if (added + d > total) d = total - added; added += d; g_weight += d; step(12); }
}

int main() {
  for (int i = 0; i < 64; i++) g_pin[i] = 1;
  g_pin[ENC_A] = 0; g_pin[ENC_B] = 0;   // el encoder reposa en el estado 00 (el sketch lo lee en setup)
  setup();
  printf("setup OK. Codigo ntfy: %s\n", saved.topic);
  CHECK(saved.topic[0] == 'c', "topic generado");
  saved.cpg = 1000; tareRaw = 0; saved.notify = 1;
  step(500);

  // --- 0. Sin calibrar: aviso en pantalla
  float cpg0 = saved.cpg; saved.cpg = 0; step(100);
  CHECK(frameHas("Sin calibrar"), "aviso de calibracion");
  saved.cpg = cpg0; step(100);

  // --- 1. Espresso: para solo al peso
  printf("1) Espresso: tara, goteo, parada al peso\n");
  g_weight = 150; step(1500);
  press(ENC_SW, 60);
  CHECK(armed, "tara arma el arranque");
  CHECK(fabsf(shown) < 0.4f, "tara deja el peso en ~0 (shown=%.2f)", shown);
  pour(36.0f, 1.6f, 2.0f);
  step(1200);
  CHECK(state == DONE, "espresso termina solo (state=%d)", (int)state);
  CHECK(fabsf(finalWeight - 36.0f) < 0.6f, "peso final ~36 (%.2f)", finalWeight);
  float tsec = (endMs - startMs) / 1000.0f;
  CHECK(tsec > 15 && tsec < 40, "tiempo razonable (%.1f s)", tsec);
  CHECK(notes() == 1, "un aviso al terminar (hay %d)", notes());
  CHECK(strstr(lastNote().title, "Espresso") != nullptr, "titulo del aviso: %s", lastNote().title);
  printf("   tiempo %.1f s, peso %.1f g, aviso: \"%s\" / \"%s\"\n", tsec, finalWeight, lastNote().title, lastNote().body);
  step(900);
  CHECK(frameHas("1:2.0"), "ratio final en pantalla");

  // --- 2. V60: el peso NO para el reloj
  printf("2) V60: objetivo alcanzado, el reloj sigue, parada a mano\n");
  press(BTN_PIN, 60);                       // DONE -> READY
  CHECK(state == READY, "vuelve a READY");
  for (int i = 0; i < 3; i++) rotateClick(+1);
  CHECK(saved.mode == 3, "modo V60 (mode=%d)", saved.mode);
  press(ENC_SW, 60);                        // tara con la taza y el espresso encima
  CHECK(armed && fabsf(shown) < 0.4f, "tara V60");
  int n0 = notes();
  pour(45, 5.0f, 2.0f); step(30000);        // floracion
  pour(100, 6.0f, 0); step(25000);
  pour(95, 6.0f, 0);                        // total 240 g
  step(2000);
  CHECK(state == BREW, "V60 sigue en marcha tras el objetivo (state=%d)", (int)state);
  CHECK(targetCue, "aviso de objetivo alcanzado");
  CHECK(notes() == n0 + 1, "aviso movil de objetivo (+1)");
  printf("   aviso: \"%s\" / \"%s\"\n", lastNote().title, lastNote().body);
  step(1500); CHECK(frameHas("objetivo"), "pantalla muestra 'objetivo'");
  uint32_t t_before = g_ms; step(20000);
  CHECK(state == BREW, "sigue corriendo 20 s despues");
  press(BTN_PIN, 60);
  CHECK(state == DONE, "azul para el reloj");
  CHECK(fabsf(finalWeight - 240) < 1.0f, "peso final V60 (%.1f)", finalWeight);
  CHECK(notes() == n0 + 2, "aviso final (+1)");
  printf("   V60: %.0f s totales, peso %.1f g\n", (endMs - startMs) / 1000.0f, finalWeight);
  (void)t_before;

  // --- 3. V60 con "sin goteo"
  printf("3) V60 con parada 'sin goteo'\n");
  press(BTN_PIN, 60); saved.m[3].stopMode = 2; press(ENC_SW, 60);
  pour(240, 6.0f, 2.0f);
  step(3000); CHECK(state == BREW, "aun en marcha 3 s tras el flujo");
  step(9000); CHECK(state == DONE, "para solo tras 8 s sin caudal (state=%d)", (int)state);
  printf("   parado a los %.0f s\n", (endMs - startMs) / 1000.0f);

  // --- 4. Menu
  printf("4) Menu: cambiar 'Parar'\n");
  press(BTN_PIN, 60);                       // DONE -> READY
  press(BTN_PIN, 1000);                     // largo = menu
  CHECK(state == MENU, "abre el menu (state=%d)", (int)state);
  for (int i = 0; i < 4; i++) rotateClick(+1);
  CHECK(menuRow == 4, "fila 'Parar' (row=%d)", menuRow);
  uint8_t before = saved.m[saved.mode].stopMode;
  press(ENC_SW, 60);
  CHECK(saved.m[saved.mode].stopMode == (before + 1) % 3, "cambia el modo de parada");
  step(200); CHECK(frameHas("Parar"), "menu dibuja la fila");
  press(BTN_PIN, 60);
  CHECK(state == READY, "azul vuelve a READY");

  // --- 5. Pantalla atenuada
  printf("5) Atenuacion de la pantalla\n");
  g_weight = 0; step(2000);
  CHECK(!dimmed, "no atenuada al principio");
  step(125000);
  CHECK(dimmed && g_bl == BL_DIM, "se atenua tras 2 min (bl=%d)", g_bl);
  g_weight = 5; step(300);
  CHECK(!dimmed && g_bl == BL_FULL, "un cambio de peso la despierta (bl=%d)", g_bl);

  // --- 6. Cambio de modo anima el nombre
  rotateClick(+1); step(400);
  CHECK(frameHas("Aeropress") || frameHas("Prensa") || frameHas("Libre") || frameHas("Espresso") || frameHas("V60"), "dibuja el nombre del modo");

  printf("\n%d comprobaciones, %d fallos, %d fotogramas dibujados\n", checks, fails, g_frames);
  return fails ? 1 : 0;
}
