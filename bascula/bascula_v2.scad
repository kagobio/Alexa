// Báscula de café v2 - inspirada en la distribución del Water Tracker
// (plataforma con nervios + franja de control con pantalla, pomo y botón)
pieza = "todo"; // "base" | "panel" | "plataforma" | "nervios" | "pomo" | "boton" | "todo"

W = 120;  D = 170;  H = 28;  R = 14;   // carcasa
pared = 2.4;
panel_d = 38;                 // fondo de la franja de control (frente)
gap = 1.5;                    // holgura plataforma-panel
plat_w = W - 2*pared - 2*gap - 2;
plat_d = D - panel_d - 2*pared - 2*gap;
plat_t = 5;
// pantalla LCD 1.47" (172x320, vertical, se monta horizontal)
disp_w = 36; disp_h = 19;     // ventana visible
// pomo (encoder) y botón
pomo_d = 26; boton_d = 15;
nervio_w = 8; nervio_h = 3; paso = 19;

module rrect(w,d,h,r){ linear_extrude(h) offset(r) square([w-2*r,d-2*r],center=true); }

module base() {
  difference() {
    rrect(W,D,H,R);
    translate([0,0,pared]) rrect(W-2*pared,D-2*pared,H,R-pared);
    translate([0,D/2-1,12]) cube([10,8,4],center=true);        // USB-C trasero
  }
  // soporte celda de carga (zona de la plataforma)
  translate([0,(panel_d)/2,pared]) cube([30,12,6],center=true);
  // topes de sobrecarga
  for (x=[-1,1]) translate([x*30,panel_d/2,pared+6]) cylinder(d=4,h=8,$fn=24);
}

module panel() {   // pieza negra del frente, al ras de la carcasa
  translate([0,-D/2+panel_d/2+pared,H-2]) difference() {
    rrect(W-2*pared-1, panel_d, 2, R-pared);
    translate([0,0,-1]) cube([disp_w,disp_h,6],center=true);                    // pantalla
    translate([-(W/2-pared-pomo_d/2-6),0,-1]) cylinder(d=7.5,h=6,$fn=32);       // eje encoder
    translate([ (W/2-pared-boton_d/2-8),0,-1]) cylinder(d=boton_d-3,h=6,$fn=32);// botón
  }
}

module plataforma() {   // flota sobre la celda
  rrect(plat_w, plat_d, plat_t, 10);
  translate([0,0,-4]) difference(){
    rrect(plat_w, plat_d, 4, 10);
    translate([0,0,-1]) rrect(plat_w-4, plat_d-4, 6, 8);
  }
}

module nervios() {      // nervios diagonales rojos (soporte de taza + antideslizante)
  intersection() {
    translate([0,0,plat_t]) rrect(plat_w-8, plat_d-8, nervio_h, 8);
    for (i=[-8:8]) translate([0,0,plat_t]) rotate(-45)
      translate([i*paso,0,0]) linear_extrude(nervio_h)
        offset(nervio_w/2) square([0.01, 220-abs(i)*12],center=true);
  }
}

module pomo()  { cylinder(d=pomo_d,h=12,$fn=24); }
module boton() { cylinder(d=boton_d,h=8,$fn=32); }

if (pieza=="base") base();
else if (pieza=="panel") panel();
else if (pieza=="plataforma") plataforma();
else if (pieza=="nervios") nervios();
else if (pieza=="pomo") pomo();
else if (pieza=="boton") boton();
else {
  color("#e8e8e8") base();
  color("#1a1a1a") panel();
  color("#f2efe6") translate([0,panel_d/2,H+gap-1]) plataforma();
  color("#d62d2d") translate([0,panel_d/2,H+gap-1]) nervios();
  color("#f5c518") translate([-(W/2-pared-pomo_d/2-6),-D/2+panel_d/2+pared,H]) pomo();
  color("#2a6fdb") translate([ (W/2-pared-boton_d/2-8),-D/2+panel_d/2+pared,H]) boton();
}
