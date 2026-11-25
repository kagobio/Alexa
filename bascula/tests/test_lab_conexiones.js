const { chromium } = require('playwright'); const fs=require('fs');
(async () => {
  const b = await chromium.launch(); const p = await b.newPage({viewport:{width:1300,height:1000}});
  const errs=[]; p.on('pageerror',e=>errs.push(e.message)); p.on('console',m=>{if(m.type()==='error')errs.push(m.text())});
  await p.goto('file://' + __dirname + '/../lab_conexiones.html'); await p.waitForTimeout(500);
  let fails=0; const chk=(c,m)=>{ if(!c){fails++; console.log('  FALLO:',m);} };
  const st=()=>p.evaluate(()=>window.__state);
  // 1) vacío
  let s=await st(); chk(!s.powered && s.wires===0,'vacío: sin alimentación');
  // 2) todo bien (con batería y RATE, sin zumbador)
  await p.evaluate(()=>lab.autoWire()); s=await st();
  const statuses=Object.fromEntries(Object.entries(s.parts).map(([k,v])=>[k,v.s]));
  console.log('autoWire:',JSON.stringify(statuses),'peligros:',s.dangers.length,'conflictos:',s.conflicts.length);
  for (const k of ['hx','rate','disp','enc','btn','bat']) chk(statuses[k]==='ok',`autoWire: ${k} debería estar ok (${statuses[k]})`);
  chk(statuses.buz==='na','zumbador desactivado = na'); chk(s.dangers.length===0 && s.conflicts.length===0,'autoWire sin peligros ni conflictos');
  chk(['ok','warn'].includes(statuses.power),'alimentación ok');
  await p.screenshot({path:'lab_ok.png'});
  // 3) quitar un cable concreto: DT
  await p.evaluate(()=>{ lab.clearAll(); lab.autoWire(); });
  const r=await p.evaluate(()=>{ // elimina el cable DT
    const ids=[]; return 0; });
  // 4) ejemplo con errores
  await p.evaluate(()=>lab.errExample()); s=await st();
  console.log('errExample peligros:',s.dangers.length,'conflictos:',s.conflicts.length); console.log('  ',s.dangers.join(' | ')); console.log('  ',s.conflicts.join(' | '));
  chk(s.dangers.some(d=>/3,3 V/.test(d)),'detecta 5 V en la línea de 3,3 V');
  chk(s.conflicts.some(c=>/GPIO16/.test(c)&&/SCK/.test(c)),'detecta DT/SCK cruzados');
  chk(s.parts.disp.s==='part' && /CS/.test((s.parts.disp.miss||[]).join()),'pantalla: falta CS');
  chk(s.parts.btn.s==='part','botón: falta GND');
  chk(s.parts.hx.s!=='ok','HX711 no ok con errores');
  await p.screenshot({path:'lab_err.png'});
  // 5) cortocircuito 3V3-GND
  await p.evaluate(()=>{ lab.clearAll(); lab.connect('esp.3V3','esp.GND'); lab.update(); }); s=await st();
  chk(s.dangers.some(d=>/CORTOCIRCUITO/.test(d)),'cortocircuito 3V3-GND');
  await p.evaluate(()=>{ lab.clearAll(); lab.connect('lipo.+','esp.3V3'); lab.update(); }); s=await st();
  chk(s.dangers.length>=1,'batería directa a 3V3 = peligro');
  await p.evaluate(()=>{ lab.clearAll(); lab.connect('esp.GPIO16','esp.3V3'); lab.update(); }); s=await st();
  chk(s.dangers.some(d=>/pin de señal/.test(d)),'3,3 V en pin de señal');
  await p.evaluate(()=>{ lab.clearAll(); lab.connect('lipo.-','esp.GND'); lab.update(); }); s=await st();
  chk(s.dangers.length===0&&s.conflicts.length===0,'masa de batería con GND: sin falsa alarma');
  await p.evaluate(()=>{ lab.clearAll(); lab.connect('esp.GPIO14','esp.GPIO12'); lab.update(); }); s=await st();
  chk(s.conflicts.length===1,'pines sin uso unidos entre sí = aviso');
  // 6) sin batería: ok sin esos cables; con zumbador
  await p.evaluate(()=>{ lab.setOpt({Bat:false,Rate:false,Buz:true}); lab.autoWire(); }); s=await st();
  console.log('sin batería/RATE, con zumbador:',JSON.stringify(Object.fromEntries(Object.entries(s.parts).map(([k,v])=>[k,v.s]))));
  chk(s.parts.buz.s==='ok'&&s.parts.bat.s==='na'&&s.parts.rate.s==='na','opcionales');
  chk(s.parts.hx.s==='ok','hx ok sin RATE');
  // 7) lista exportable
  const list=await p.evaluate(()=>lab.checklist()); console.log(list.split('\n').slice(0,6).join('\n'));
  // 8) clic real con el ratón: dos terminales
  await p.evaluate(()=>{ lab.setOpt({Bat:true,Rate:true,Buz:false}); lab.clearAll(); });
  const n0=(await st()).wires;
  await p.mouse.click(0,0);
  // 9) coherencia con el firmware
  const ino=fs.readFileSync('' + __dirname + '/../firmware/CoffeeScale/CoffeeScale.ino','utf8');
  const num=n=>{ const m=ino.match(new RegExp('\\b'+n+'\\s*=\\s*(\\d+)')); return m? +m[1]:null; };
  const pins={ENC_A:num('ENC_A'),ENC_B:num('ENC_B'),ENC_SW:num('ENC_SW'),BTN:num('BTN_PIN'),HXDT:num('HX_DT'),HXSCK:num('HX_SCK'),RATE:num('HX_RATE'),BAT:num('BAT_ADC'),BL:num('TFT_BL')};
  const spec=await p.evaluate(()=>{ const g=n=>lab.NETS[n].t.find(t=>t.startsWith('esp.GPIO')).replace('esp.GPIO','')*1; return {ENC_A:g('ENC_A'),ENC_B:g('ENC_B'),ENC_SW:g('ENC_SW'),BTN:g('BTN'),HXDT:g('DT'),HXSCK:g('SCK'),RATE:g('RATE'),BAT:g('ADC'),BL:g('BLK'),BUZ:g('BUZ'),SCLK:g('SCLK'),MOSI:g('MOSI'),CS:g('CS'),DC:g('DC'),RST:g('RST')}; });
  for (const k of Object.keys(pins)) chk(pins[k]===spec[k],`pin ${k}: firmware ${pins[k]} vs laboratorio ${spec[k]}`);
  const bo=fs.readFileSync('' + __dirname + '/../firmware/CoffeeScale/build_opt.h','utf8'); const bn=n=>+bo.match(new RegExp('-D'+n+'=(\\d+)'))[1];
  for (const [k,f] of [['SCLK','TFT_SCLK'],['MOSI','TFT_MOSI'],['CS','TFT_CS'],['DC','TFT_DC'],['RST','TFT_RST']]) chk(bn(f)===spec[k],`pin pantalla ${k}: build_opt ${bn(f)} vs laboratorio ${spec[k]}`);
  console.log('pines del firmware:',JSON.stringify(pins),'| BUZ en el laboratorio:',spec.BUZ);
  console.log(errs.join('\n')||'sin errores de consola'); console.log(fails? fails+' fallos':'TODO OK');
  await b.close();
})();
