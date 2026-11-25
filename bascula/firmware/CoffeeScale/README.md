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

| Modo | Dosis | Ratio | Objetivo | Tiempo | Parar por defecto |
| --- | --- | --- | --- | --- | --- |
| Espresso | 18 g | 1:2 | 36 g | 25–32 s | al peso |
| Ristretto | 18 g | 1:1.5 | 27 g | 20–28 s | al peso |
| Lungo | 18 g | 1:3 | 54 g | 30–45 s | al peso |
| V60 | 15 g | 1:16 | 240 g | 2:30–3:30 | a mano |
| Aeropress | 15 g | 1:13 | 195 g | 1:30–2:00 | a mano |
| Prensa francesa | 30 g | 1:15 | 450 g | 3:50–4:10 | a mano |
| Libre | – | – | sin objetivo | – | a mano |

Dosis, ratio y ventana de tiempo se editan por modo en **Ajustes** y se guardan en la memoria.

## Uso
- **Girar el pomo** (en espera): cambiar de modo.
- **Pulsar el pomo**: tara. Arma el arranque automático.
- **Arranque automático**: tras la tara, el reloj arranca solo cuando el peso sube ≥ 1 g
  (primer goteo o primer chorro). También puedes arrancar con el **botón azul**.
- **Parar** (Ajustes, por modo): *a mano*, *al peso* o *sin goteo*.
  - **Espresso, Ristretto y Lungo** vienen en *al peso*: el reloj se para solo al llegar a dosis × ratio.
  - **V60, Aeropress y Prensa francesa** vienen en *a mano*: en filtro el peso no marca el final de la preparación
    (el café sigue escurriendo). Al llegar al objetivo la báscula solo avisa ("Objetivo alcanzado" y, si están
    activados, un aviso al móvil) y **el reloj sigue** hasta que lo paras con el botón azul.
  - *sin goteo*: tras el objetivo, para cuando el caudal lleva 8 s por debajo de 0,4 g/s. Solo funciona si la
    báscula está **debajo de la jarra** y ve caer el café; si pesa todo el conjunto (agua en el filtro incluida), el peso
    no cambia al escurrir y no detecta el final: ahí usa *a mano*.
- **Botón azul** durante la extracción: parar a mano. Con parada automática (espresso) se para solo
  tras 4 s sin caudal y el tiempo mostrado es el del último flujo.
- **Pomo** durante la extracción: re-tara sin parar el reloj (útil en pour-over).
- **Botón azul largo** (en espera o al terminar): menú de Ajustes
  (dosis, ratio, tiempo mín/máx, parar, auto inicio, avisos al móvil, Wi-Fi, código, calibrar).


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

## Pantalla (v2: la misma que la demo)
Fondo negro, tiempo grande y peso. Un **nivel de café** sube con una onda suave hasta el peso objetivo (dosis × ratio) y pocas burbujas
mientras fluye. Verde dentro de la ventana de tiempo, rojo si se pasa. Detalles:
- el reloj da un pequeño salto cada segundo; al cambiar de modo el nombre se desliza con un fundido;
- al hacer la **tara**, un aro azul rodea el peso;
- al llegar al **objetivo** en filtro: aro verde y la derecha pasa a decir "objetivo" (el reloj sigue);
- al parar el reloj: aro verde si estás en tiempo, naranja si no; el nivel pasa a verde y a la derecha sale el ratio;
- puntito arriba: verde = tara hecha, arranca solo; hueco = falta la tara; texto rojo si no está calibrada.
- **Atenuación:** tras 2 minutos sin tocar nada ni cambiar el peso en espera, la luz baja (PWM, 40/255) y casi no redibuja; cualquier giro, pulsación o cambio de peso de 1 g la despierta. Nunca se atenúa mientras prepara café, tras terminar ni en los menús.

## Prueba en el servidor (sin placa)
`firmware/test_host/run.sh` compila **el firmware real** contra piezas falsas de Arduino (pantalla, HX711, Wi-Fi, memoria, colas) y lo ejecuta
con un café simulado. Comprueba (45 comprobaciones, hoy 0 fallos, también con AddressSanitizer/UBSan):
- tara y arranque automático; espresso parado al peso (22 s, 35,9 g, ratio 1:2,0, un aviso al móvil);
- V60: al llegar a 240 g el reloj **sigue**, avisa "objetivo alcanzado" y solo se para con el botón azul;
- modo "sin goteo": para solo a los 8 s sin caudal tras el objetivo;
- menú (el ajuste "Parar" cambia y se guarda), aviso de "Sin calibrar", atenuación y despertar, nombre del modo al girar el pomo.

Esto **valida la lógica, no el hardware**: no prueba el HX711, el ST7789, el Wi-Fi ni la velocidad real del ESP32, y no comprueba que las librerías reales de Arduino compilen
(las piezas falsas son más permisivas). Sigue pendiente compilarlo con el Arduino IDE y probarlo en la placa. El `.ino` usa `ledcAttach`/`ledcWrite` (core ESP32 3.x).

## Batería (opcional)
Si conectas una LiPo, la pantalla muestra un **icono con el porcentaje** arriba a la derecha (verde > 30 %, naranja 15–30 %, rojo < 15 % y parpadeando por debajo del 10 %),
un rayo si detecta que carga, y en **Ajustes → Bateria** el porcentaje y la tensión. Si no hay batería (< 2,5 V en el pin) el icono no aparece.
- **Medida:** divisor 100 kΩ / 100 kΩ desde B+ al **GPIO35** (ADC1, solo entrada) y 100 nF a masa; `Vbat = 2 × Vpin` (`BAT_DIV`). Media de 16 lecturas cada 2 s con filtro.
- **Porcentaje:** curva de descarga de una LiPo en reposo (4,20 V = 100 %, 3,70 V = 30 %, 3,30 V = 0 %). Es **orientativa**: con carga o con la celda fría varía.
- **Carga:** se deduce porque la tensión sube ≥ 15 mV en 30 s (no se lee el pin de estado del módulo de carga). Puede tardar en detectarla y fallar con módulos que cortan la carga.
- **Avisos:** por debajo del 15 % sin carga, una vez por descarga: aviso en pantalla y, si están activados, **al móvil** ("Bateria baja"). Por debajo del 5 %: "Bateria critica" fija en espera.
- **Ajuste fino:** mide la tensión real con un polímetro y compárala con la de Ajustes → Bateria; si difieren, corrige `BAT_DIV`. El ADC del ESP32 no es lineal en los extremos.
- La batería **no afecta a la báscula**: el HX711 sigue alimentándose del 3,3 V del ESP32. Con la radio Wi-Fi activa el consumo sube y la autonomía baja.
- Módulos con **IP5306** y similares pueden **apagar la salida** si el consumo es bajo (< ~50 mA); la pantalla atenuada y el ESP32 sin Wi-Fi quedan cerca de ese límite. Si se apaga solo, usa TP4056 + elevador.
- `firmware/test_host/run.sh` incluye ahora la batería: **45 comprobaciones, 0 fallos** (sin batería, 3,90 V, menú, batería baja con un solo aviso, carga, llena y desconexión). Sigue sin probarse en la placa.
