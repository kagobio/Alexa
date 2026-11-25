// Prueba del modo juego del laboratorio de conexiones.
const { chromium } = require("playwright"); const path = require("path");
(async () => {
  const b = await chromium.launch(); const p = await b.newPage({viewport:{width:1300,height:900}}); const errs = [];
  p.on("pageerror", e => errs.push(String(e))); p.on("console", m => m.type()==="error" && errs.push(m.text()));
  await p.goto("file://" + path.resolve(__dirname, "../lab_conexiones.html"));
  let fail = 0; const ck = (c, m) => { if (!c) { fail++; console.log("FALLO:", m); } };
  await p.click("#bGame");
  ck(await p.$eval("#game", e => e.classList.contains("on")), "panel visible");
  ck(await p.$eval("#bAuto", e => e.disabled), "autoconectar desactivado");
  // cable malo: 5 V con masa
  await p.evaluate(() => lab.userWire(lab.keyFor("esp.VIN"), lab.keyFor("esp.GND")));
  let s = await p.evaluate(() => ({m: lab.G.mistakes, w: lab.G.finished, n: window.__state.wires}));
  ck(s.m === 1, "fallo contado"); ck(s.n === 0, "cable malo no se queda");
  // pista cuenta
  await p.click("#gHint"); ck(await p.evaluate(() => lab.G.hints) === 1, "pista contada");
  // cablear todo bien, red por red, con la misma lógica que el jugador
  await p.evaluate(() => {
    const an = {}; // reconstruye redes activas con las opciones por defecto
    const skip = new Set(); // bat y rate activos, zumbador no
    for (const [nid,n] of Object.entries(lab.NETS)) { if (nid==="BUZ") continue;
      const ts = n.t.filter(t => !t.startsWith("buz.")); for (const t of ts.slice(1)) lab.userWire(lab.keyFor(ts[0]), lab.keyFor(t)); }
    lab.update();
  });
  s = await p.evaluate(() => ({m: lab.G.mistakes, f: lab.G.finished, h: lab.G.hints}));
  ck(s.f === true, "juego terminado"); ck(s.m === 1, "sin fallos extra");
  ck(await p.$eval("#win", e => e.classList.contains("on")), "pantalla de victoria");
  ck((await p.textContent("#wStars")).startsWith("★★"), "estrellas: " + await p.textContent("#wStars"));
  await p.screenshot({path: process.env.SHOT || "/tmp/juego.png"});
  // salir restaura
  await p.click("#wClose"); await p.click("#gExit");
  ck(await p.$eval("#bAuto", e => !e.disabled), "salir reactiva botones");
  console.log(errs.length ? "errores: " + errs.join("|") : "sin errores de consola"); if (errs.length) fail++;
  console.log(fail ? "HAY FALLOS" : "JUEGO OK"); await b.close(); process.exit(fail ? 1 : 0);
})();
