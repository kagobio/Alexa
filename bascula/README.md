# Báscula de café (ESP32 + celda de carga + pantalla)

Proyecto inspirado en el [Water Tracker & Reminder](https://makerworld.com/es/models/3603669-water-tracker-reminder)
de Aaron Broom (hardware, guía de montaje) y [todayicreated/water-tracker](https://github.com/todayicreated/water-tracker) (firmware MIT).

| Archivo | Qué es |
| --- | --- |
| `bascula.scad`, `preview.png` | Boceto v1 (varias celdas, pantalla redonda). Descartado. |
| `bascula_v2.scad`, `preview_v2.png` | Boceto v2: distribución del Water Tracker. |
| `bascula_v3.scad` | **Actual**: montaje con una celda de barra (extremo fijo a la base, extremo libre a la plataforma). |
| `firmware/CoffeeScale/` | Firmware con temporizador y modos de café. Ver su README. |

> `bascula_v3.scad` y el firmware **no se han compilado ni probado**.

## Montaje mecánico (de la guía del Water Tracker)
1. Base: celda de carga atornillada por abajo con 2 × M5×12 sobre un soporte (da holgura al extremo libre).
2. Electrónica sobre la base (ESP32 con 2 × M2×4; HX711 con cinta doble cara; pantalla con 2 × M2×8).
3. Carcasa exterior sobre la base con 6 × BT2.6×8 autorroscantes.
4. Plataforma atornillada al extremo libre de la celda con M4×12 (separador de 3 mm).
5. Tapa superior a presión, nervios, pomo y botón.

La plataforma **no debe rozar** la carcasa (holgura 1,5 mm). Todo el peso pasa por la celda.

## Si usas la celda de tu báscula rota
Compruébala con un multímetro y mide largo, sección y posición de los taladros. Las celdas de
barra suelen llevar M5 en el extremo fijo y M4 en el libre. Ajusta `cell_*`, `fix_h*` y `free_h`
en `bascula_v3.scad`. Si tu báscula tiene 4 celdas pequeñas (una por esquina), esta base no
sirve tal cual: avísame.

## Cableado
5 V por USB-C al ESP32 (pin VIN). El HX711, la pantalla y el encoder van a 3,3 V del ESP32.
Masa común. Pines en `firmware/CoffeeScale/README.md`. Fuente de 5 V sencilla: según la guía
original, los cargadores rápidos pueden no funcionar.

## Lista de compra (resumen)
ESP32-WROOM-32D · HX711 + celda de 5 kg (o la tuya) · pantalla 1.47" 172×320 ST7789 (DFRobot) ·
encoder con pulsador · interruptor MX low profile (botón azul) · conector USB-C hembra ·
tornillos 2×M5×12, 2×M4×12, 6×BT2.6×8, 2×M2×4, 4×M2×8. Los LEDs WS2812 son opcionales y mi
firmware no los usa.
