// Báscula de café v3.1 - montaje mecánico basado en la guía del Water Tracker
// (una celda de carga de barra: extremo fijo a la base, extremo libre a la plataforma).
// Dimensionada para el Hario V60 Termo acero inox. 800 ml (cuerpo ~135 mm; 182 x 135 x 172 mm con asa).
// Compilado con OpenSCAD 2021.01. No impreso todavía: imprime antes "test_ajuste" (ver README).
//
// pieza: base | carcasa | plataforma | tapa | nervios | separador | panel | pomo | boton | test_ajuste | todo
pieza = "todo";

// ============ DATOS A VERIFICAR CON TUS PIEZAS REALES (son estimaciones) ============
disp_w = 36; disp_h = 19;      // ventana visible de la pantalla 1.47" (DFRobot). MIDE tu módulo.
enc_hole = 7.5;                // agujero del casquillo del encoder (KY-040: ~7 mm de rosca). MIDE.
mx_hole  = 14.0;               // hueco del interruptor MX low profile (cuadrado). MIDE.
cell_L = 80; cell_w = 12.7; cell_h = 12.7;   // celda de barra de 5 kg. MIDE la tuya.
fix_holes  = [6, 21];          // taladros M5 del extremo FIJO, distancia a ese extremo (mm)
free_holes = [5, 15];          // taladros M4 del extremo LIBRE, distancia a ese extremo (mm)
// ===================================================================================

// --- Carcasa ---
W = 152; D = 188; R = 14;      // exterior (plataforma útil ~142 x 142 mm)
base_t = 3;                    // espesor de la base
foot_h = 3;                    // patas bajo la base: dejan sitio a las cabezas de los tornillos M5
pared = 2.4; deck = 3;
panel_d = 38;                  // fondo de la franja de control (frente)

// --- Celda y plataforma flotante ---
cell_angle = 45;               // giro de la celda (comprobado que cabe con W=152, D=188)
boss = 4;                      // soporte bajo el extremo fijo (holgura para el extremo libre)
spacer = 3;                    // separador entre celda (extremo libre) y plataforma
gap = 1.5;                     // holgura plataforma-carcasa
plat_t = 6;                    // espesor de la placa (6 mm: rigidez con servidor de ~2 kg)
tapa_t = 1.5;                  // espesor de la tapa
lip_h = 2; lip_w = 2;          // borde antisalpicaduras, solidario a la plataforma
rib_deep = 2; rib_w = 2.4;     // costillas de refuerzo bajo la placa
peg_d = 5.4; peg_hole = 5.0; peg_len = 5;    // pivotes de la tapa (ajuste a presión)

// --- Derivados ---
zc = base_t + boss;                    // base de la celda
zp = zc + cell_h + spacer;             // cara inferior de la plataforma
H  = zp + plat_t + tapa_t - base_t;    // altura de la carcasa: la tapa queda a ras del borde
plat_w = W - 2*pared - 2*gap - 2;
plat_d = D - panel_d - 2*pared - 2*gap;
cy = panel_d/2;                        // centro de la plataforma en Y (los controles van al frente, -Y)
free_sp = free_holes[1] - free_holes[0];
free_mid = cell_L - (free_holes[0] + free_holes[1]) / 2;   // punto medio de los taladros libres (desde el extremo fijo)

// --- Controles ---
pomo_d = 26; boton_d = 15;
nervio_w = 8; nervio_h = 2; paso = 19;
post_d = 8; post_h = 16;
enc_x = -(W/2 - pared - pomo_d/2 - 6);   // posición X del encoder
btn_x =  (W/2 - pared - boton_d/2 - 8);  // posición X del botón

module rrect(w,d,h,r){ linear_extrude(h) offset(r) square([w-2*r,d-2*r],center=true); }

// origen local = extremo fijo de la celda; +X apunta al extremo libre.
// El punto medio de los 2 taladros del extremo libre queda en el centro de la plataforma.
module cell_frame() { translate([0,cy,0]) rotate(cell_angle) translate([-free_mid,0,0]) children(); }

function screw_pts() = [[-(W/2-8.06),-(D/2-8.06)],[ (W/2-8.06),-(D/2-8.06)],
                        [-(W/2-5.6),0],[ (W/2-5.6),0],
                        [-(W/2-8.06),(D/2-8.06)],[ (W/2-8.06),(D/2-8.06)]];
function peg_pts() = [[-(plat_w/2-14),-(plat_d/2-14)],[ (plat_w/2-14),-(plat_d/2-14)],
                      [-(plat_w/2-14), (plat_d/2-14)],[ (plat_w/2-14), (plat_d/2-14)]];
function foot_pts() = [[-(W/2-16),-(D/2-16)],[ (W/2-16),-(D/2-16)],
                       [-(W/2-16), (D/2-16)],[ (W/2-16), (D/2-16)]];

module base() {
  difference() {
    union() {
      rrect(W,D,base_t,R);
      // soporte (boss) bajo el extremo fijo
      translate([0,0,base_t]) cell_frame() translate([-2,-cell_w/2-2,0]) cube([fix_holes[1]+12, cell_w+4, boss]);
      // patas
      for (p=foot_pts()) translate([p[0],p[1],-foot_h]) cylinder(d=14,h=foot_h+0.01,$fn=36);
    }
    // taladros M5 de la celda, con rebaje por debajo para la cabeza (d9.5 x 1.5)
    cell_frame() for (p=fix_holes) translate([p,0,-foot_h-1]) {
      cylinder(d=5.4,h=foot_h+base_t+boss+2,$fn=32);
      cylinder(d=9.5,h=foot_h+1+1.5,$fn=32);
    }
    // 6 taladros para los tornillos BT2.6 que unen base y carcasa
    for (p=screw_pts()) translate([p[0],p[1],-1]) cylinder(d=3,h=base_t+2,$fn=24);
  }
}

module carcasa() {   // marco con cubierta en el frente (panel de control); ventana trasera para la plataforma
  difference() {
    union() {
      difference() {
        rrect(W,D,H,R);
        translate([0,0,-1]) rrect(W-2*pared,D-2*pared,H-deck+1,R-pared);        // cavidad
        translate([0,cy,H-deck-1]) rrect(plat_w+2*gap,plat_d+2*gap,deck+2,10);  // ventana plataforma
        translate([0,-D/2+panel_d/2+pared,H-2]) rrect(W-2*pared-1,panel_d,3,R-pared);  // bolsillo del panel
        translate([0,D/2,12]) cube([10,2*pared+2,4],center=true);                // USB-C trasero
      }
      for (p=screw_pts()) translate([p[0],p[1],0]) cylinder(d=post_d,h=post_h,$fn=28);
    }
    for (p=screw_pts()) translate([p[0],p[1],-1]) cylinder(d=2.2,h=post_h+1,$fn=20);   // agujero guía
  }
}

module panel() {     // inserto negro del frente
  translate([0,-D/2+panel_d/2+pared,H-2]) difference() {
    rrect(W-2*pared-1.4,panel_d-0.4,2,R-pared);
    translate([0,0,1]) cube([disp_w,disp_h,4],center=true);                              // pantalla (atraviesa el grosor)
    translate([enc_x,0,-1]) cylinder(d=enc_hole,h=4,$fn=32);                             // eje del encoder
    translate([btn_x-mx_hole/2,-mx_hole/2,-1]) cube([mx_hole,mx_hole,4]);                // interruptor MX
  }
}

module separador() { // entre la celda (extremo libre) y la plataforma. Origen: punto medio de los 2 taladros.
  difference() {
    hull() for (s=[-1,1]) translate([s*free_sp/2,0,0]) cylinder(d=cell_w-0.7,h=spacer,$fn=36);
    for (s=[-1,1]) translate([s*free_sp/2,0,-1]) cylinder(d=4.4,h=spacer+2,$fn=28);
  }
}

module plataforma() { // placa maciza de 6 mm + costillas por debajo + borde antisalpicaduras + agujeros
  difference() {
    union() {
      rrect(plat_w,plat_d,plat_t,10);
      // borde perimetral (retiene el agua que cae fuera del filtro)
      translate([0,0,plat_t]) difference() {
        rrect(plat_w,plat_d,tapa_t+lip_h,10);
        translate([0,0,-1]) rrect(plat_w-2*lip_w,plat_d-2*lip_w,tapa_t+lip_h+2,8);
      }
      // costillas de refuerzo por debajo, sin tocar la zona central (separador y tornillos)
      difference() {
        for (k=[-2,-1,1,2]) {
          translate([k*28-rib_w/2,-plat_d/2+4,-rib_deep]) cube([rib_w,plat_d-8,rib_deep+0.01]);
          translate([-plat_w/2+4,k*28-rib_w/2,-rib_deep]) cube([plat_w-8,rib_w,rib_deep+0.01]);
        }
        translate([0,0,-rib_deep-1]) cylinder(d=40,h=rib_deep+2);
      }
    }
    // 2 agujeros M4 (alineados con la celda) con rebaje para la cabeza fina
    rotate(cell_angle) for (s=[-1,1]) translate([s*free_sp/2,0,0]) {
      translate([0,0,-1]) cylinder(d=4.4,h=plat_t+2,$fn=28);
      translate([0,0,plat_t-2.5]) cylinder(d=8,h=3,$fn=32);
    }
    // 4 agujeros ciegos para los pivotes de la tapa
    for (p=peg_pts()) translate([p[0],p[1],plat_t-peg_len-0.5]) cylinder(d=peg_hole,h=peg_len+1,$fn=24);
  }
}

module peg() {      // pivote con hendidura en cruz: se deforma al entrar (ajuste a presión)
  difference() {
    union() {
      cylinder(d1=peg_hole-0.4,d2=peg_d,h=1,$fn=24);
      translate([0,0,1]) cylinder(d=peg_d,h=peg_len-1,$fn=24);
    }
    translate([-0.4,-peg_d,-0.01]) cube([0.8,2*peg_d,peg_len*0.7]);
    translate([-peg_d,-0.4,-0.01]) cube([2*peg_d,0.8,peg_len*0.7]);
  }
}
module tapa() {     // se imprime boca abajo (cara superior en la cama), con los pivotes hacia arriba
  translate([0,0,plat_t]) {
    rrect(plat_w-2*lip_w-0.6,plat_d-2*lip_w-0.6,tapa_t,8);
    for (p=peg_pts()) translate([p[0],p[1],0.01]) mirror([0,0,1]) peg();
  }
}
module nervios() {   // nervios diagonales rojos sobre la tapa (segundo color o pegados)
  intersection() {
    translate([0,0,plat_t+tapa_t-0.02]) rrect(plat_w-2*lip_w-8,plat_d-2*lip_w-8,nervio_h+0.02,8);
    for (i=[-8:8]) translate([0,0,plat_t+tapa_t-0.02]) rotate(-45) translate([i*paso,0,0])
      linear_extrude(nervio_h+0.02) offset(nervio_w/2) square([0.01,220-abs(i)*12],center=true);
  }
}
module pomo()  { cylinder(d=pomo_d,h=12,$fn=24); }
module boton() { cylinder(d=boton_d,h=8,$fn=32); }
module celda() { color("#b8bec4") cell_frame() translate([0,-cell_w/2,0]) cube([cell_L,cell_w,cell_h]); }
module celda_en_z() { translate([0,0,zc]) celda(); }

// Pieza de prueba: imprímela en 15-20 minutos y comprueba que tus piezas reales encajan ANTES de imprimir lo demás.
module test_ajuste() {
  // A) placa de controles: ventana de pantalla, agujero del encoder y hueco del interruptor MX
  difference() {
    translate([-5,0,0]) rrect(110,36,2,4);
    translate([-30,0,1]) cube([disp_w,disp_h,4],center=true);
    translate([-62,0,-1]) cylinder(d=enc_hole,h=4,$fn=32);
    translate([22,-mx_hole/2,-1]) cube([mx_hole,mx_hole,4]);
  }
  // B) plantilla de la celda: apoya tu celda encima y comprueba que los taladros coinciden
  translate([-45,-45,0]) difference() {
    rrect(cell_L+16,cell_w+10,3,3);
    translate([0,0,2]) cube([cell_L+0.6,cell_w+0.6,2],center=true);          // alojamiento de 1 mm
    for (p=fix_holes)  translate([-cell_L/2+p,0,-1]) cylinder(d=5.4,h=5,$fn=28);   // M5 (extremo fijo)
    for (p=free_holes) translate([ cell_L/2-p,0,-1]) cylinder(d=4.4,h=5,$fn=28);   // M4 (extremo libre)
  }
}

module vista() {
  color("#e8e8e8") base();
  color("#e8e8e8") translate([0,0,base_t]) carcasa();
  celda_en_z();
  color("#8a8f96") translate([0,cy,zc+cell_h]) rotate(cell_angle) separador();
  color("#1a1a1a") translate([0,0,base_t]) panel();
  color("#f2efe6") translate([0,cy,zp]) { plataforma(); tapa(); }
  color("#d62d2d") translate([0,cy,zp]) nervios();
  color("#f5c518") translate([enc_x,-D/2+panel_d/2+pared,base_t+H]) pomo();
  color("#2a6fdb") translate([btn_x,-D/2+panel_d/2+pared,base_t+H]) boton();
}

if (pieza=="base") base();
else if (pieza=="carcasa") carcasa();
else if (pieza=="plataforma") plataforma();
else if (pieza=="tapa") tapa();
else if (pieza=="nervios") nervios();
else if (pieza=="separador") separador();
else if (pieza=="panel") panel();
else if (pieza=="pomo") pomo();
else if (pieza=="boton") boton();
else if (pieza=="test_ajuste") test_ajuste();
else vista();
