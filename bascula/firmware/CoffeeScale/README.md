# CoffeeScale (firmware)

Báscula de café con **temporizador** y **modos de preparación**, basada en el hardware y en
partes del firmware de [todayicreated/water-tracker](https://github.com/todayicreated/water-tracker)
(licencia MIT, ver `LICENSE-WaterTracker`, `FONT-LICENSE.txt` y `src/hx711/LICENSE`).

> **Estado: sin compilar y sin probar en hardware.** Se escribió sin Arduino IDE ni placa.
> Espera algún error de compilación o ajuste al primer intento.

## Hardware y pines (iguales al Water Tracker)

| Componente | Señales | GPIO |
| --- | --- | --- |
| Pantalla ST7789 172×320 | SCLK / MOSI / CS / DC / RST / luz | 18 / 23 / 27 / 26 / 25 / 22 |
| Encoder (pomo) | A / B / pulsador | 33 / 32 / 19 |
| Botón azul | pulsador | 21 |
| HX711 | DT / SCK | 16 / 17 |
| HX711 RATE (opcional) | RATE | 4 |

Sin Wi-Fi, sin LEDs y sin hora: arranca rápido y hay menos que fallar.

### Mejora recomendada: HX711 a 80 SPS
El HX711 viene a 10 lecturas/s, poco para medir caudal en espresso. En la mayoría de módulos el
pin RATE está unido a GND por una pista: córtala y cablea RATE al GPIO 4. El firmware lo pone
en 80 SPS. Si no lo haces, funciona igual a 10 SPS, con caudal más lento de reaccionar.

## Instalación
1. Arduino IDE → Boards Manager: **esp32 by Espressif Systems** 3.x.
2. Library Manager: **TFT_eSPI 2.5.43**. El driver HX711 va incluido.
3. Abre `CoffeeScale.ino` con toda la carpeta junta (`build_opt.h` configura la pantalla).
4. Placa **ESP32 Dev Module**. La partición por defecto basta (no usa Wi-Fi).
5. La pantalla va a 27 MHz de SPI (el original usa 4 MHz). Si ves ruido o líneas, baja
   `SPI_FREQUENCY` en `build_opt.h` (20000000 o 10000000).

## Modos (valores iniciales orientativos, editables)

| Modo | Dosis | Ratio | Objetivo | Tiempo | Parada auto |
| --- | --- | --- | --- | --- | --- |
| Espresso | 18 g | 1:2 | 36 g | 25–32 s | sí |
| Ristretto | 18 g | 1:1.5 | 27 g | 20–28 s | sí |
| Lungo | 18 g | 1:3 | 54 g | 30–45 s | sí |
| V60 | 15 g | 1:16 | 240 g | 2:30–3:30 | no |
| Aeropress | 15 g | 1:13 | 195 g | 1:30–2:00 | no |
| Prensa francesa | 30 g | 1:15 | 450 g | 3:50–4:10 | no |
| Libre | – | – | sin objetivo | – | no |

Dosis, ratio y ventana de tiempo se editan por modo en **Ajustes** y se guardan en la memoria.

## Uso
- **Girar el pomo** (en espera): cambiar de modo.
- **Pulsar el pomo**: tara. Arma el arranque automático.
- **Arranque automático**: tras la tara, el reloj arranca solo cuando el peso sube ≥ 1 g
  (primer goteo o primer chorro). También puedes arrancar con el **botón azul**.
- **Botón azul** durante la extracción: parar. Con parada automática (espresso) se para solo
  tras 4 s sin caudal y el tiempo mostrado es el del último flujo.
- **Pomo** durante la extracción: re-tara sin parar el reloj (útil en pour-over).
- **Botón azul largo** (en espera o al terminar): menú de Ajustes
  (dosis, ratio, tiempo mín/máx, auto inicio, calibrar).
- Pantalla: tiempo grande (verde dentro de la ventana, rojo si se pasa), peso, caudal en g/s
  y un fondo de café que sube con una onda hasta el peso objetivo. Al terminar muestra el
  ratio real y si fue rápido, en tiempo o lento.

## Calibración
Ajustes → Calibrar: quita todo, pulsa el pomo; gira para indicar el peso conocido y pulsa;
coloca ese peso y pulsa de nuevo. Usa una pesa de 100–500 g. El cero y el factor se guardan.
