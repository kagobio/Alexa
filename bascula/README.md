# Báscula de café (ESP32 + celda de carga + pantalla)

Proyecto inspirado en el [Water Tracker & Reminder](https://makerworld.com/es/models/3603669-water-tracker-reminder)
de Aaron Broom (hardware, guía de montaje) y [todayicreated/water-tracker](https://github.com/todayicreated/water-tracker) (firmware MIT).

| Archivo | Qué es |
| --- | --- |
| `bascula.scad`, `preview.png` | Boceto v1 (varias celdas, pantalla redonda). Descartado. |
| `bascula_v2.scad`, `preview_v2.png` | Boceto v2: distribución del Water Tracker. |
| `img/` (conjunto, despiezado, interior y piezas de la v3), `stl/` | Vista del conjunto v3 y las 8 piezas exportadas a STL (base, carcasa, plataforma, tapa, nervios, panel, pomo, botón). |
| `bascula_v3.scad` | **Actual (v3.1)**: montaje con una celda de barra (extremo fijo a la base, extremo libre a la plataforma). 152 × 188 mm, plataforma útil ~142 × 142 mm, para el Hario V60 Termo 800 ml. |
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

## v3.1: cambios respecto a la primera v3
1. **Plataforma rígida:** placa maciza de **6 mm** (antes 3 mm) con costillas de 2 mm por debajo y un borde de 2 mm contra salpicaduras. Con 3 mm la flexión en el extremo era de unos 2 mm con 2 kg; con 6 mm baja a unas décimas (cálculo orientativo de viga en voladizo, sin ensayo).
2. **Separador** de 3 mm impreso (`separador`) entre la celda y la plataforma. Antes solo existía el hueco.
3. **Dos tornillos M4** en el extremo libre de la celda (antes uno), con rebaje para la cabeza fina. M4×12 da ~5 mm de rosca en la celda.
5. **Patas de 3 mm** bajo la base y rebaje de 1,5 mm para las cabezas de los M5 (antes la báscula se habría balanceado).
6. **Tapa sujeta con 4 pivotes a presión** (con hendidura en cruz) en agujeros ciegos de la plataforma. Se imprime boca abajo, con los pivotes hacia arriba. Los nervios rojos se imprimen en un segundo color sobre la tapa o se pegan.
7. **Datos estimados agrupados** al principio de `bascula_v3.scad` ("DATOS A VERIFICAR") y pieza `test_ajuste` (abajo). No se pueden "corregir" sin medir tus piezas.

La altura de la carcasa ahora se calcula (27,2 mm) para que la tapa quede a ras del borde.

## Antes de imprimir todo: `test_ajuste`
`pieza = "test_ajuste"` (o `stl/test_ajuste.stl`) es una placa pequeña (~143 × 74 mm, 3 mm) de unos 15–20 min de impresión:
- **Placa de controles:** ventana de pantalla (36 × 19 mm), agujero del encoder (7,5 mm) y hueco del interruptor MX (14 × 14 mm). Comprueba que tus piezas entran.
- **Plantilla de la celda:** apoya tu celda en el alojamiento y comprueba que los taladros M5 (a 6 y 21 mm) y M4 (a 5 y 15 mm del extremo) coinciden. Si no, cambia `fix_holes`, `free_holes` y las medidas de la celda en el `.scad`.

## Comprobaciones hechas con OpenSCAD 2021.01 (v3.1)
- Las 9 piezas compilan sin errores ni avisos y son sólidos válidos.
- Sin choques entre celda, base, carcasa, separador, plataforma, tapa, nervios y panel. Los 4 pivotes de la tapa solapan con los agujeros a propósito (ajuste a presión).
- La celda cabe dentro de la carcasa a 45°.
- Medidas: carcasa 152 × 188 × 27,2 mm; base 7 mm (+3 mm de patas); plataforma 142,2 × 142,2 mm.
- **No comprobado:** impresión real, el ajuste a presión de los pivotes (depende de tu impresora), la rigidez real de la plataforma, y el ajuste con pantalla, encoder, ESP32 y celda reales (el modelo no incluye soportes para el ESP32, el HX711 ni la pantalla).
