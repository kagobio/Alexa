// Báscula de café - diseño v1 (parametrizable)
// Piezas: base, plataforma, bandeja. Cambia 'pieza' para exportar cada una.
pieza = "todo"; // "base" | "plataforma" | "bandeja" | "todo"

// --- Dimensiones (mm) ---
W = 150;        // ancho
D = 170;        // fondo total
H = 32;         // altura de la base
R = 10;         // radio de esquinas
pared = 2.4;
pantalla_d = 33;     // hueco pantalla redonda GC9A01 1.28"
pantalla_pcb = 39;   // PCB de la pantalla
inclinacion = 35;    // ángulo del frontal
gap = 1.5;           // holgura plataforma-base (no debe rozar)
celda_h = 6;         // altura de la celda de carga
plat_t = 5;          // espesor plataforma
bandeja_t = 12;      // altura bandeja antigoteo

module rrect(w, d, h, r) {
  linear_extrude(h) offset(r) square([w-2*r, d-2*r], center=true);
}

module base() {
  difference() {
    union() {
      rrect(W, D, H, R);
    }
    // cavidad interior
    translate([0,0,pared]) rrect(W-2*pared, D-2*pared, H, R-pared);
    // frontal inclinado (corte) para la pantalla
    translate([0, -D/2, H]) rotate([-inclinacion,0,0])
      translate([-W, -10, 0]) cube([2*W, 60, 60]);
    // hueco pantalla en el plano inclinado
    translate([0, -D/2 + 17, H-9]) rotate([-inclinacion,0,0])
      translate([0,0,-10]) cylinder(d=pantalla_d, h=30, $fn=80);
    // USB-C
    translate([0, D/2-1, 12]) cube([10, 8, 4], center=true);
  }
  // postes para celdas (4 esquinas)
  for (x=[-1,1], y=[-1,1])
    translate([x*(W/2-25), y*(D/2-45)+5, pared]) cylinder(d=12, h=celda_h, $fn=40);
  // topes de sobrecarga
  for (x=[-1,1], y=[-1,1])
    translate([x*(W/2-25), y*(D/2-45)+5, pared+celda_h+3]) cylinder(d=4, h=celda_h+1, $fn=24);
}

module plataforma() {
  tw = W - 2*pared - 2*gap - 6;
  td = D - 60;
  rrect(tw, td, plat_t, 6);
  // faldón perimetral
  translate([0,0,-4]) difference() {
    rrect(tw, td, 4, 6);
    translate([0,0,-1]) rrect(tw-4, td-4, 6, 4);
  }
}

module bandeja() {
  tw = W - 2*pared - 2*gap - 14;
  td = D - 70;
  difference() {
    rrect(tw, td, bandeja_t, 5);
    translate([0,0,2]) rrect(tw-4, td-4, bandeja_t, 3);
  }
  // rejilla
  for (i=[-6:6]) translate([i*10, 0, bandeja_t-3]) cube([3, td-6, 3], center=false);
}

if (pieza=="base") base();
else if (pieza=="plataforma") plataforma();
else if (pieza=="bandeja") bandeja();
else {
  color("#2b2b2b") base();
  color("#9aa5b1") translate([0,5,H+gap]) plataforma();
  color("#c9822b") translate([0,5,H+gap+plat_t]) bandeja();
}
