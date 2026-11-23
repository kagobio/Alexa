// Báscula de café v3 - montaje mecánico basado en la guía del Water Tracker
// (una celda de carga de barra: extremo fijo a la base, extremo libre a la plataforma).
// NO COMPILADO: no había OpenSCAD al escribirlo. Mide tu celda y ajusta los parámetros.
pieza = "todo"; // base | carcasa | plataforma | tapa | nervios | panel | pomo | boton | todo

// --- Carcasa ---
// Dimensionada para el Hario V60 Termo acero inox. 800 ml (cuerpo ~135 mm; 182 x 135 x 172 mm con asa).
// Plataforma útil ~142 x 142 mm: la base del servidor cabe entera con margen.
W = 152; D = 188; R = 14;
base_t = 3;            // espesor de la base
H = 26;                // altura de la carcasa (sobre la base)
pared = 2.4; deck = 3; // pared y espesor de la cubierta frontal
panel_d = 38;          // fondo de la franja de control

// --- Celda de carga de barra (MIDE LA TUYA) ---
cell_L = 80; cell_w = 12.7; cell_h = 12.7;
fix_h1 = 6;  fix_h2 = 21;    // posición de los 2 taladros M5 (extremo fijo), desde ese extremo
free_h = 6;                  // taladros M4 del extremo libre: distancia al extremo libre
cell_angle = 45;             // giro de la celda dentro de la caja (grados); comprobado que cabe con W=152, D=188
boss = 4;                    // altura del soporte bajo el extremo fijo (da holgura al libre)
spacer = 3;                  // separador entre celda (extremo libre) y plataforma

// --- Plataforma flotante ---
gap = 1.5;
plat_w = W - 2*pared - 2*gap - 2;
plat_d = D - panel_d - 2*pared - 2*gap;
plat_t = 3;
cy = panel_d/2;              // centro de la plataforma en Y (la franja de control va al frente, -Y)

// --- Controles ---
disp_w = 36; disp_h = 19;    // ventana de la pantalla 1.47" (confirma con tu módulo)
pomo_d = 26; boton_d = 15;
nervio_w = 8; nervio_h = 2; paso = 19;
lip_h = 2;                   // borde bajo contra salpicaduras en el perímetro de la tapa

module rrect(w,d,h,r){ linear_extrude(h) offset(r) square([w-2*r,d-2*r],center=true); }

// posición del eje de la celda: el centro del taladro libre queda en el centro de la plataforma
module cell_frame() { translate([0,cy,0]) rotate(cell_angle) translate([-(cell_L-free_h),0,0]) children(); }
// ahora el origen local es el extremo fijo; el eje +X apunta hacia el extremo libre

module base() {
  difference() {
    union() {
      rrect(W,D,base_t,R);
      // soporte (boss) bajo el extremo fijo
      translate([0,0,base_t]) cell_frame() translate([0,0,0]) translate([-2,-cell_w/2-2,0])
        cube([fix_h2+12, cell_w+4, boss]);
    }
    // taladros M5 para la celda (desde abajo), avellanados
    cell_frame() for (p=[fix_h1,fix_h2]) translate([p,0,-1]) {
      cylinder(d=5.4,h=base_t+boss+2,$fn=32);
    }
    // 6 taladros para tornillos BT2.6 que unen base y carcasa
    for (p=screw_pts()) translate([p[0],p[1],-1]) cylinder(d=3,h=base_t+2,$fn=24);
  }
}
// Postes para 6 tornillos BT2.6: 4 en las esquinas (hacia dentro, sin salirse de la pared redondeada) y 2 en los lados.
// Quedan pegados a la pared interior (solapan ~1 mm) y su altura no llega a la plataforma (post_h).
post_d = 8; post_h = 16;
function screw_pts() = [[-(W/2-8.06),-(D/2-8.06)],[ (W/2-8.06),-(D/2-8.06)],
                        [-(W/2-5.6),0],[ (W/2-5.6),0],
                        [-(W/2-8.06),(D/2-8.06)],[ (W/2-8.06),(D/2-8.06)]];

module carcasa() {   // marco con cubierta en el frente (panel de control); ventana trasera para la plataforma
  difference() {
    union() {
      difference() {
        rrect(W,D,H,R);
        translate([0,0,-1]) rrect(W-2*pared,D-2*pared,H-deck+1,R-pared);        // cavidad
        translate([0,cy,H-deck-1]) rrect(plat_w+2*gap,plat_d+2*gap,deck+2,10);  // ventana plataforma
        // bolsillo del panel negro
        translate([0,-D/2+panel_d/2+pared,H-2]) rrect(W-2*pared-1,panel_d,3,R-pared);
        // USB-C trasero
        translate([0,D/2,12]) cube([10,2*pared+2,4],center=true);
      }
      for (p=screw_pts()) translate([p[0],p[1],0]) cylinder(d=post_d,h=post_h,$fn=28);
    }
    // agujero guía d2.2 en cada poste (también atraviesa la pared donde se solapan)
    for (p=screw_pts()) translate([p[0],p[1],-1]) cylinder(d=2.2,h=post_h+1,$fn=20);
  }
}

module panel() {     // inserto negro del frente
  translate([0,-D/2+panel_d/2+pared,H-2]) difference() {
    rrect(W-2*pared-1.4,panel_d-0.4,2,R-pared);
    translate([0,0,-1]) cube([disp_w,disp_h,4],center=true);
    translate([-(W/2-pared-pomo_d/2-6),0,-1]) cylinder(d=7.5,h=4,$fn=32);
    translate([ (W/2-pared-boton_d/2-8),0,-1]) cylinder(d=boton_d-3,h=4,$fn=32);
  }
}

module plataforma() { // se atornilla al extremo libre de la celda con 2 M4
  difference() {
    rrect(plat_w,plat_d,plat_t,10);
    translate([0,0,-1]) cylinder(d=4.4,h=plat_t+2,$fn=24);
  }
}
module tapa() {
  translate([0,0,plat_t]) {
    rrect(plat_w-0.6,plat_d-0.6,2,9);
    // borde perimetral: retiene el agua que cae fuera del filtro (no estorba: la base del servidor queda dentro)
    difference() {
      rrect(plat_w-0.6,plat_d-0.6,2+lip_h,9);
      translate([0,0,2]) rrect(plat_w-0.6-4,plat_d-0.6-4,lip_h+1,7);
    }
  }
}
module nervios() {
  intersection() {
    translate([0,0,plat_t+2-0.02]) rrect(plat_w-8,plat_d-8,nervio_h+0.02,8);
    for (i=[-8:8]) translate([0,0,plat_t+2-0.02]) rotate(-45) translate([i*paso,0,0])
      linear_extrude(nervio_h+0.02) offset(nervio_w/2) square([0.01,220-abs(i)*12],center=true);
  }
}
module pomo()  { cylinder(d=pomo_d,h=12,$fn=24); }
module boton() { cylinder(d=boton_d,h=8,$fn=32); }
module celda() { color("#b8bec4") cell_frame() translate([0,-cell_w/2,0]) cube([cell_L,cell_w,cell_h]); }

zc = base_t + boss;                 // base de la celda
zp = zc + cell_h + spacer;          // parte inferior de la plataforma
module vista() {
  color("#e8e8e8") base();
  color("#e8e8e8") translate([0,0,base_t]) carcasa();
  translate([0,0,base_t]) { /* carcasa ya empieza en z=base_t */ }
  translate([0,0,0]) celda_en_z();
  color("#1a1a1a") translate([0,0,base_t]) panel();
  color("#f2efe6") translate([0,cy,zp]) { plataforma(); tapa(); }
  color("#d62d2d") translate([0,cy,zp]) nervios();
  color("#f5c518") translate([-(W/2-pared-pomo_d/2-6),-D/2+panel_d/2+pared,base_t+H]) pomo();
  color("#2a6fdb") translate([ (W/2-pared-boton_d/2-8),-D/2+panel_d/2+pared,base_t+H]) boton();
}
module celda_en_z() { translate([0,0,zc]) celda(); }

if (pieza=="base") base();
else if (pieza=="carcasa") carcasa();
else if (pieza=="plataforma") plataforma();
else if (pieza=="tapa") tapa();
else if (pieza=="nervios") nervios();
else if (pieza=="panel") panel();
else if (pieza=="pomo") pomo();
else if (pieza=="boton") boton();
else vista();
