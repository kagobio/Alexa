# Báscula de café (ESP32 + celda de carga + pantalla)

Proyecto inspirado en el [Water Tracker & Reminder](https://makerworld.com/es/models/3603669-water-tracker-reminder)
de Aaron Broom (hardware, guía de montaje) y [todayicreated/water-tracker](https://github.com/todayicreated/water-tracker) (firmware MIT).

| Archivo | Qué es |
| --- | --- |
| `bascula.scad`, `preview.png` | Boceto v1 (varias celdas, pantalla redonda). Descartado. |
| `bascula_v2.scad`, `preview_v2.png` | Boceto v2: distribución del Water Tracker. |
| `img/` (conjunto, despiezado, interior y piezas de la v3), `stl/` | Vista del conjunto v3 y las 8 piezas exportadas a STL (base, carcasa, plataforma, tapa, nervios, panel, pomo, botón). |
| `bascula_v3.scad` | **Actual**: montaje con una celda de barra (extremo fijo a la base, extremo libre a la plataforma). 152 × 188 mm, plataforma útil ~142 × 142 mm, para el Hario V60 Termo 800 ml. |
| `firmware/CoffeeScale/` | Firmware con temporizador y modos de café. Ver su README. |

> `bascula_v3.scad` **sí compila** (OpenSCAD 2021.01) y pasa comprobaciones de choques entre piezas, pero no se ha impreso ni probado con piezas reales. El firmware **no se ha compilado ni probado**.

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

## Tamaño para V60 (Hario V60 Termo acero inox. 800 ml, negro)
Medidas oficiales de Hario: 182 × 135 × 172 mm (ancho con asa × fondo × alto), unos 600–790 g vacío según la fuente.
No encontré el diámetro exacto de la base. La plataforma útil es de ~142 × 142 mm para que el cuerpo (~135 mm) quepa
entera. Con el servidor lleno (~0,8 kg de café + ~0,7 kg de servidor + dripper) el peso queda muy por debajo de los 5 kg de la celda.
- El servidor es de doble pared: la base no se calienta, así que el PETG no sufre.
- La tapa lleva un borde de 2 mm contra salpicaduras (`lip_h`).
- Mide la base de tu servidor y, si es mayor de 135 mm o quieres más margen, sube `W` y `D` en `bascula_v3.scad`
  (la plataforma es `W - 9.8` y `D - 45.8`); comprueba que la celda sigue cabiendo si cambias `cell_angle`.
- Corrección: en la v3 anterior (120 × 170 mm, celda a 25°) el extremo fijo de la celda quedaba fuera de la carcasa. Con estos valores cabe.

## Comprobaciones hechas con OpenSCAD (v3)
- Las 8 piezas compilan sin errores ni avisos y son sólidos válidos.
- Sin choques entre celda, plataforma, carcasa, base y panel.
- Corregido: los postes de los tornillos BT2.6 chocaban con la plataforma, algunos no tocaban la pared (quedaban flotando) y los de las esquinas sobresalían de la carcasa. Ahora son 8 mm de diámetro, 16 mm de alto y están pegados a la pared.
- Medidas: carcasa 152 × 188 × 26 mm; base 7 mm; plataforma 142,2 × 142,2 × 3 mm.
- **No comprobado:** ajuste con la pantalla, el encoder y el ESP32 reales (el modelo no incluye sus huecos de montaje), tolerancias de tu impresora y los taladros de tu celda.
- Los STL de `nervios` y `tapa` conservan su altura de montaje (no empiezan en z=0): el laminador los apoya en la cama solo.
