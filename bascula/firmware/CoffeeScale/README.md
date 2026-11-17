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

Sin LEDs y sin hora. El Wi-Fi es opcional y solo sirve para los avisos al móvil (ver más abajo).

### Mejora recomendada: HX711 a 80 SPS
El HX711 viene a 10 lecturas/s, poco para medir caudal en espresso. En la mayoría de módulos el
pin RATE está unido a GND por una pista: córtala y cablea RATE al GPIO 4. El firmware lo pone
en 80 SPS. Si no lo haces, funciona igual a 10 SPS, con caudal más lento de reaccionar.

## Instalación
1. Arduino IDE → Boards Manager: **esp32 by Espressif Systems** 3.x.
2. Library Manager: **TFT_eSPI 2.5.43** y **WiFiManager** (tzapu). El driver HX711 va incluido.
3. Abre `CoffeeScale.ino` con toda la carpeta junta (`build_opt.h` configura la pantalla).
4. Placa **ESP32 Dev Module**. Con Wi-Fi y HTTPS hay que elegir **Tools → Partition Scheme → Huge APP (3MB No OTA/1MB SPIFFS)**, como en el Water Tracker.
5. La pantalla va a 27 MHz de SPI (el original usa 4 MHz). Si ves ruido o líneas, baja
   `SPI_FREQUENCY` en `build_opt.h` (20000000 o 10000000).

## Modos (valores iniciales orientativos, editables)

| Modo | Dosis | Ratio | Objetivo | Tiempo | Parada por falta de caudal |
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
- **Parar al peso** (activado por defecto en todos los modos con objetivo): el reloj se para solo al
  alcanzar dosis × ratio (margen de 0,2 g). Se desactiva por modo en Ajustes. Así el reloj no sigue
  corriendo cuando ya has vertido todo.
- **Botón azul** durante la extracción: parar a mano. Con parada automática (espresso) se para solo
  tras 4 s sin caudal y el tiempo mostrado es el del último flujo.
- **Pomo** durante la extracción: re-tara sin parar el reloj (útil en pour-over).
- **Botón azul largo** (en espera o al terminar): menú de Ajustes
  (dosis, ratio, tiempo mín/máx, parar al peso, auto inicio, calibrar).
- Pantalla: tiempo grande (verde dentro de la ventana, rojo si se pasa), peso, caudal en g/s
  y un fondo de café que sube con una onda hasta el peso objetivo. Al terminar muestra el
  ratio real y si fue rápido, en tiempo o lento.

## Calibración
Ajustes → Calibrar: quita todo, pulsa el pomo; gira para indicar el peso conocido y pulsa;
coloca ese peso y pulsa de nuevo. Usa una pesa de 100–500 g. El cero y el factor se guardan.

## Avisos al móvil (Wi-Fi + ntfy)
Al parar el reloj, la báscula envía una notificación a tu móvil, por ejemplo
*"Espresso listo: 36.1 g en 0:27  Ratio 1:2.0 (en tiempo)"*. Funciona en iPhone y Android con la app gratuita **ntfy**.

1. Instala la app **ntfy** en el móvil.
2. En la báscula: **Ajustes → Wi-Fi**. Conecta el móvil a la red `CoffeeScale-Setup` (clave `cafecafe`),
   abre `192.168.4.1` y elige tu red de casa. El portal dura 3 minutos y bloquea la báscula mientras tanto.
3. **Ajustes → Avisos movil → SI**.
4. **Ajustes → Codigo** muestra tu código (por ejemplo `cafe-1a2b3c4d5e6f7a8b`; también sale por el puerto serie al arrancar).
   En la app ntfy, suscríbete a un tema con ese código exacto.

Notas:
- **Privacidad:** el servidor público ntfy.sh recibe el texto del aviso (modo, peso, tiempo). El código, de 64 bits
  aleatorios, es lo único que protege el canal: no lo compartas. Se puede usar un servidor ntfy propio cambiando la URL en el sketch.
- **Retraso:** la notificación tarda uno o dos segundos y depende de la red y del móvil. **No sirve para parar la extracción a tiempo.**
  Para eso, lo mejor es un zumbador junto a la báscula (no incluido todavía).
- **Ruido en la báscula:** la radio Wi-Fi puede inyectar ruido en la alimentación del HX711. Alimenta el HX711 de forma estable
  y comprueba que el peso no oscila al enviar. Si molesta, se puede conectar solo en el momento de enviar.
- Si no hay Wi-Fi en ese momento, el aviso se descarta (no se reintenta).
- Está **sin compilar ni probar en hardware**.
