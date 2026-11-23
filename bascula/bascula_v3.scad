// Báscula de café v3.1 - montaje mecánico basado en la guía del Water Tracker
// (una celda de carga de barra: extremo fijo a la base, extremo libre a la plataforma).
// Dimensionada para el Hario V60 Termo acero inox. 800 ml (cuerpo ~135 mm; 182 x 135 x 172 mm con asa).
// Compilado con OpenSCAD 2021.01. No impreso todavía: imprime antes "test_ajuste" (ver README).
//
// pieza: base | carcasa | plataforma | tapa | nervios | separador | panel | soporte_pantalla | pomo | boton | test_ajuste | todo
pieza = "todo";
$fn = 48;   // suavidad de las curvas

// ============ DATOS A VERIFICAR CON TUS PIEZAS REALES (son estimaciones) ============
disp_w = 34.2; disp_h = 19.2;  // ventana de la pantalla 1.47" DFRobot DFR0995: área visible 33.8 x 19.1 mm (hoja de datos DFRobot)
disp_mod_l = 44.5; disp_mod_w = 19.5;   // módulo DFR0995: 44.5 x 19.5 mm (DFRobot). Grosor y posición de la regleta: MIDE.
disp_t = 5.5;                  // ESTIMADO: grosor del módulo (PCB + cristal). MIDE y ajusta: la bandeja lo aprieta contra la carcasa.
esp_l = 55; esp_w = 28;        // ESTIMADO: placa ESP32 DevKit (51-55 x 25-28 mm según modelo). MIDE la tuya.
esp_pins_down = true;          // true: pines soldados por debajo (lo habitual en DevKit); false: pines hacia arriba
hx_l = 34; hx_w = 21;          // ESTIMADO: módulo HX711 genérico (34 x 21 mm; hay de 22x22, 38x22...). MIDE el tuyo.
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

// --- Electrónica sobre la base ---
yc = -D/2 + panel_d/2 + pared;         // centro (Y) de la franja de control
esp_x = 38;                            // ESP32: a la derecha, largo en Y, con el USB hacia la pared trasera
esp_lift = esp_pins_down ? 11 : 2;     // altura de apoyo (los pines van por debajo si esp_pins_down)
esp_bar = esp_pins_down ? esp_w - 9 : esp_w;   // apoyo central: deja libres los laterales para los pines
esp_y = D/2 - pared - 0.3 - (esp_l+0.4)/2;     // centro del ESP32 (el USB toca casi la pared trasera)
hx_x = -48; hx_y = 48;                 // HX711: a la izquierda-atrás, largo en X
nest_wall = 2; nest_wh = 4;            // pared y altura extra del nido sobre el apoyo
esp_usb_z = esp_lift + 1.6 + 1.6;      // altura del centro del conector USB sobre la base (PCB 1,6 + mitad del conector)
usb_w = 14; usb_h = 8;                 // abertura trasera para el enchufe USB-C
disp_post_x = disp_mod_l/2 + 6.2;      // posición X de los postes de la bandeja (fuera del cuerpo de la bandeja: sin solape)
tray_floor = 1.5; ear_t = 2.5;
tray_h = tray_floor + disp_t;          // altura de la bandeja (el módulo queda pegado a la cara inferior de la cubierta)
disp_post_len = tray_h - ear_t;

// --- Controles ---
pomo_d = 26; pomo_h = 14;       // pomo con estrías y concavidad superior
shaft_d = 6; shaft_flat = 4.6;  // eje del encoder KY-040 (en D): ESTIMADO, mide el tuyo
boton_d = 18; boton_h = 7;      // tecla redondeada 18 x 18 mm: cubre las esquinas del hueco MX de 14 x 14
nervio_w = 8; nervio_h = 1.6; paso = 16.5;
post_d = 8; post_h = 16;
panel_w = W - 12; panel_h = 32;         // panel negro en cápsula: marco blanco de 6 mm alrededor
enc_x = -(panel_w/2 - panel_h/2);        // el pomo queda concéntrico con el extremo izquierdo de la cápsula
btn_x =  (panel_w/2 - panel_h/2);        // y el botón con el derecho

module rrect(w,d,h,r){ linear_extrude(h) offset(r) square([w-2*r,d-2*r],center=true); }
// caja redondeada con el canto SUPERIOR suavizado (radio f)
module rrect_fillet(w,d,h,r,f,n=6) {
  hull() {
    rrect(w,d,h-f,r);
    for (i=[1:n]) { a=90*i/n; k=f*(1-cos(a));
      translate([0,0,h-f+f*sin(a)-0.01*(i==n?0:0)]) rrect(w-2*k,d-2*k,0.01,max(r-k,0.5)); }
  }
}

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

// Nido para una placa: apoyo central + paredes laterales + tope en un extremo; el extremo +X queda abierto.
module nest(l,w,lift,bar,wall=2,wh=4) {
  translate([-l/2,-bar/2,0]) cube([l,bar,lift]);
  for (sg=[-1,1]) translate([-l/2, sg>0 ? w/2 : -w/2-wall, 0]) cube([l,wall,lift+wh]);
  translate([-l/2-wall,-w/2-wall,0]) cube([wall,w+2*wall,lift+wh]);
}

module base() {
  difference() {
    union() {
      translate([0,0,base_t]) mirror([0,0,1]) rrect_fillet(W,D,base_t,R,1.2);   // canto inferior redondeado
      // soporte (boss) bajo el extremo fijo
      translate([0,0,base_t]) cell_frame() translate([-2,-cell_w/2-2,0]) cube([fix_holes[1]+12, cell_w+4, boss]);
      // patas
      for (p=foot_pts()) translate([p[0],p[1],-foot_h]) cylinder(d=14,h=foot_h+0.01,$fn=36);
      // nido del ESP32 (USB hacia la pared trasera) y del HX711
      translate([esp_x,esp_y,base_t]) rotate(90) nest(esp_l+0.4,esp_w+0.4,esp_lift,esp_bar,nest_wall,nest_wh);
      translate([hx_x,hx_y,base_t]) nest(hx_l+0.4,hx_w+0.4,2,hx_w+0.4,nest_wall,nest_wh);
    }
    // ranuras para bridas (cruzan la placa por encima): ESP32 a ambos lados, HX711 a ambos lados
    for (sg=[-1,1]) {
      translate([esp_x+sg*((esp_w+0.4)/2+nest_wall+1.5)-0.8, esp_y-1.9, -1]) cube([1.6,3.8,base_t+2]);
      translate([hx_x-1.9, hx_y+sg*((hx_w+0.4)/2+nest_wall+1.5)-0.8, -1]) cube([3.8,1.6,base_t+2]);
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

module carcasa() { intersection() { carcasa_raw(); rrect_fillet(W,D,H,R,1.5); } }   // canto superior redondeado
module carcasa_raw() {   // marco con cubierta en el frente (panel de control); ventana trasera para la plataforma
  difference() {
    union() {
      difference() {
        rrect(W,D,H,R);
        translate([0,0,-1]) rrect(W-2*pared,D-2*pared,H-deck+1,R-pared);        // cavidad
        translate([0,cy,H-deck-1]) rrect(plat_w+2*gap,plat_d+2*gap,deck+2,10);  // ventana plataforma
        translate([0,yc,H-2]) rrect(panel_w+0.6,panel_h+0.6,3,(panel_h+0.6)/2-0.01);          // bolsillo del panel (cápsula)
        translate([esp_x,D/2,esp_usb_z]) cube([usb_w,2*pared+2,usb_h],center=true);   // USB-C del ESP32 (trasero)
        // huecos que atraviesan la cubierta bajo el panel: pantalla, encoder y interruptor
        translate([0,yc,H-4]) rrect(disp_w,disp_h,5,1.5);
        translate([enc_x,yc,H-4]) cylinder(d=enc_hole,h=5,$fn=32);
        translate([btn_x-mx_hole/2,yc-mx_hole/2,H-4]) cube([mx_hole,mx_hole,5]);
      }
      for (p=screw_pts()) translate([p[0],p[1],0]) cylinder(d=post_d,h=post_h,$fn=28);
      // postes bajo la cubierta para atornillar la bandeja de la pantalla (M2)
      for (sg=[-1,1]) translate([sg*disp_post_x,yc,H-deck-disp_post_len]) cylinder(d=6,h=disp_post_len+0.01,$fn=28);
    }
    for (p=screw_pts()) translate([p[0],p[1],-1]) cylinder(d=2.2,h=post_h+1,$fn=20);   // agujero guía
    for (sg=[-1,1]) translate([sg*disp_post_x,yc,H-deck-disp_post_len-1]) cylinder(d=1.8,h=disp_post_len+1.6,$fn=20);
  }
}

module panel() {     // inserto negro del frente
  translate([0,yc,H-2]) difference() {
    rrect(panel_w,panel_h,2,panel_h/2-0.01);
    // ventana de la pantalla: esquinas redondeadas y bisel de 0,6 mm hacia arriba
    hull() { translate([0,0,-0.5]) rrect(disp_w,disp_h,0.01,1.5); translate([0,0,2]) rrect(disp_w+1.2,disp_h+1.2,0.01,2.1); }
    translate([0,0,-0.5]) rrect(disp_w,disp_h,3,1.5);
    translate([enc_x,0,-1]) cylinder(d=enc_hole,h=4,$fn=32);                             // eje del encoder
    translate([btn_x-mx_hole/2,-mx_hole/2,-1]) cube([mx_hole,mx_hole,4]);                // interruptor MX
  }
}

module soporte_pantalla() {   // bandeja que aprieta el módulo contra la cara inferior de la cubierta. Origen: centro, base en z=0.
  difference() {
    union() {
      translate([-(disp_mod_l+0.4)/2-2,-(disp_mod_w+0.4)/2-2,0]) cube([disp_mod_l+0.4+4,disp_mod_w+0.4+4,tray_h]);
      // orejas con agujero M2 (los tornillos suben desde abajo hasta los postes de la carcasa)
      for (sg=[-1,1]) translate([sg*disp_post_x,0,0]) hull() {
        cylinder(d=7,h=ear_t,$fn=28);
        translate([-sg*3.5,0,0]) cylinder(d=7,h=ear_t,$fn=28);
      }
    }
    translate([-(disp_mod_l+0.4)/2,-(disp_mod_w+0.4)/2,tray_floor]) cube([disp_mod_l+0.4,disp_mod_w+0.4,disp_t+1]);   // alojamiento del módulo
    translate([-(disp_mod_l+0.4)/2-0.01,-(disp_mod_w-4)/2,-1]) cube([14,disp_mod_w-4,tray_floor+2]);               // paso de la regleta de pines (lado izquierdo)
    for (sg=[-1,1]) translate([sg*disp_post_x,0,-1]) cylinder(d=2.2,h=ear_t+2,$fn=20);                           // M2
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
module cap2d(p1,p2,w) { hull() { translate(p1) circle(d=w,$fn=32); translate(p2) circle(d=w,$fn=32); } }
module nervios() {   // nervios diagonales rojos: cápsulas de punta redondeada y cantos suavizados (sobre la tapa)
  k = 0.70710678;
  h = (plat_w - 2*lip_w - 0.6)/2 - 6 - nervio_w/2;     // los centros de las puntas quedan a 6 mm del borde de la tapa
  translate([0,0,plat_t+tapa_t-0.02]) for (i=[-7:7]) {
    cx = i*paso*k; cyy = -cx;
    t0 = max(-h-cx, -h-cyy)/k; t1 = min(h-cx, h-cyy)/k;
    if (t1 - t0 >= 0.5) {
      p1 = [cx + t0*k, cyy + t0*k]; p2 = [cx + t1*k, cyy + t1*k];
      hull() {
        linear_extrude(0.02) cap2d(p1,p2,nervio_w);
        translate([0,0,nervio_h+0.02-0.01]) linear_extrude(0.01) cap2d(p1,p2,nervio_w-1.6);   // arista superior achaflanada
      }
    }
  }
}
module pomo() {   // pomo con estrías, canto redondeado, concavidad superior, hueco para la tuerca y eje en D
  r = pomo_d/2; fl = 2.5;
  difference() {
    rotate_extrude($fn=96) polygon(concat([[0,0],[r,0]],
      [for (a=[0:10:90]) [r-fl+fl*cos(a), pomo_h-fl+fl*sin(a)]], [[0,pomo_h]]));
    for (i=[0:15]) rotate(i*360/16) translate([r+0.3,0,-1]) cylinder(d=3.2,h=pomo_h+2,$fn=24);   // estrías
    translate([0,0,pomo_h-0.9+60]) sphere(r=60,$fn=120);                                            // concavidad (0,9 mm)
    translate([0,0,-1]) cylinder(d=12,h=1+3.6,$fn=48);                                               // hueco de la tuerca
    translate([0,0,-1]) linear_extrude(11) difference() {                                            // eje en D
      circle(d=shaft_d+0.2,$fn=40);
      translate([shaft_flat-shaft_d/2+0.1,-5]) square(10);
    }
  }
}
module boton() {  // tecla redondeada con zócalo en cruz para el interruptor MX (medidas MX típicas: verifícalas)
  difference() {
    hull() {
      rrect(boton_d,boton_d,boton_h-2.5,4.5);
      translate([0,0,boton_h-2.5]) rrect(boton_d-2.4,boton_d-2.4,2.5,3.6);
    }
    translate([-2.075,-0.65,-1]) cube([4.15,1.3,6]);
    translate([-0.65,-2.075,-1]) cube([1.3,4.15,6]);
  }
}
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
  // C) marcos de comprobación: deja caer el módulo de pantalla, el ESP32 y el HX711 y mira si entran con holgura
  translate([-60,25,0]) difference() { cube([disp_mod_l+4.4,disp_mod_w+4.4,5]); translate([2,2,1.5]) cube([disp_mod_l+0.4,disp_mod_w+0.4,5]); }
  translate([-8,25,0])  difference() { cube([hx_l+4.4,hx_w+4.4,5]);  translate([2,2,1.5]) cube([hx_l+0.4,hx_w+0.4,5]); }
  translate([-60,52,0]) difference() { cube([esp_l+4.4,esp_w+4.4,5]); translate([2,2,1.5]) cube([esp_l+0.4,esp_w+0.4,5]); }
}

module vista() {
  color("#e8e8e8") base();
  color("#e8e8e8") translate([0,0,base_t]) carcasa();
  celda_en_z();
  color("#8a8f96") translate([0,cy,zc+cell_h]) rotate(cell_angle) separador();
  color("#1a1a1a") translate([0,0,base_t]) panel();
  color("#c9a227") translate([0,yc,base_t+H-deck-tray_h]) soporte_pantalla();
  // módulo de pantalla de ejemplo (solo para la vista; no se imprime)
  color("#0c1020") translate([0,yc,base_t+H-deck-disp_t/2]) cube([disp_mod_l,disp_mod_w,disp_t],center=true);
  color("#f2efe6") translate([0,cy,zp]) { plataforma(); tapa(); }
  color("#d62d2d") translate([0,cy,zp]) nervios();
  color("#f5c518") translate([enc_x,yc,base_t+H+0.4]) pomo();
  color("#2a6fdb") translate([btn_x,yc,base_t+H+1.2]) boton();
}

if (pieza=="base") base();
else if (pieza=="carcasa") carcasa();
else if (pieza=="plataforma") plataforma();
else if (pieza=="tapa") tapa();
else if (pieza=="nervios") nervios();
else if (pieza=="separador") separador();
else if (pieza=="panel") panel();
else if (pieza=="soporte_pantalla") soporte_pantalla();
else if (pieza=="pomo") pomo();
else if (pieza=="boton") boton();
else if (pieza=="test_ajuste") test_ajuste();
else vista();
