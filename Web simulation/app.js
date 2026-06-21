'use strict';
/* ============================================================
   DYCE - web simulator. Faithful port of the firmware:
   dice.cpp (game state machine) + ui.cpp (rendering on a canvas).
   Two control modes (one toggle): turn the ring like the real
   encoder (drag), or scroll the wheel. The side button = power.
   ============================================================ */
(() => {

const cv  = document.getElementById('screen');
const ctx = cv.getContext('2d');
ctx.scale(2, 2);                                  // 480 internal, drawn in 240 coords (sharp)

const logoImg = new Image(); logoImg.src = 'assets/logo.png';   // full logo (intro)
const iconImg = new Image(); iconImg.src = 'assets/icon.png';   // just the "D" (attract)
const qrImg   = new Image(); qrImg.src   = 'assets/qr.png';

// ---- config (mirrors include/config.h) ----
const STEP_MAX_INC = 28, STEP_MIN_INC = 10, FILL_SPEED = 8;
const COOLDOWN_MS = 250, REVEAL_MS = 2500;
const SECRET_LO = 55, SECRET_HI = 80, SECRET_MS = 1500;
const INTRO_MS = 4500, ATTRACT_MS = 10000, FADE_MS = 900, ATTRACT_OUT_MS = 250, FIRST_MS = 8000, HEHE_MS = 6000;
const N_VALUES = [2, 5, 10, 50, 99], N_COUNT = 5, N_START = 1;
const CONSEILS = ["Spin to roll\nThe Dice", "Find\nThe hidden 67", "Turn left to\nChange Dice", "Bored ?\nSpin the Dice"];
const NUMPX = 128;

// ---- colors (RGB565 -> rgb), same palette as the firmware ----
const FOND = [16,40,74], RAIL = [41,40,41], TEXTE = [197,198,197], GRIS = [132,130,132], OR = [255,206,0];

// ---- helpers ----
const clamp = (v,a,b) => v < a ? a : v > b ? b : v;
const cssc  = c => `rgb(${c[0]|0},${c[1]|0},${c[2]|0})`;
function melange(a, b, t) { t = clamp(t,0,100); return [a[0]+(b[0]-a[0])*t/100, a[1]+(b[1]-a[1])*t/100, a[2]+(b[2]-a[2])*t/100]; }
function teinte(h) {
  h = ((h % 360) + 360) % 360; const x = (h % 60) / 60; let r,g,b;
  switch ((h/60)|0) {
    case 0: [r,g,b]=[1,x,0]; break;   case 1: [r,g,b]=[1-x,1,0]; break;
    case 2: [r,g,b]=[0,1,x]; break;   case 3: [r,g,b]=[0,1-x,1]; break;
    case 4: [r,g,b]=[x,0,1]; break;   default:[r,g,b]=[1,0,1-x];
  }
  return [r*255, g*255, b*255];
}
function point(deg, r, size, color) {
  const a = (deg - 90) * Math.PI / 180;
  ctx.fillStyle = cssc(color);
  ctx.beginPath(); ctx.arc(120 + Math.cos(a)*r, 120 + Math.sin(a)*r, size, 0, 7); ctx.fill();
}
function setFont(px, w) { ctx.font = `${w} ${px}px "Google Sans", "Product Sans", Arial, sans-serif`; }
function text(s, x, y, color, px, w) {
  setFont(px, w); ctx.fillStyle = cssc(color); ctx.textAlign = 'center'; ctx.textBaseline = 'middle';
  ctx.fillText(s, x, y);
}
function texteCentre(s, cx, cy, color, px, w) {
  const p = s.split('\n');
  if (p.length === 1) { text(p[0], cx, cy, color, px, w); return; }
  text(p[0], cx, cy - 14, color, px, w); text(p[1], cx, cy + 14, color, px, w);
}

// ============================================================
// GAME STATE  (port of dice.cpp)
// ============================================================
let phase = 'INTRO', nombre = 7, indexN = N_START, charge = 0, affiche = 0, flash = 0;
let special = false, montre = false;
let introStart = 0, revDebut = 0, revMs = 0, tRoll = 0, zoneDebut = 0, lastInput = 0;
let attractStart = 0, attractLeft = 0, specialStart = 0;
let t0 = 0, powered = false;

const ms   = () => performance.now() - t0;
const roll = n => n < 2 ? n : (Math.floor(Math.random() * n) + 1);
function pas() { let p = STEP_MIN_INC + (STEP_MAX_INC-STEP_MIN_INC)*(100-charge)/100; p += roll(3)-2; return Math.max(1, Math.round(p)); }
function enContact() { return indexN === N_COUNT; }

function versAttente() { phase='ATTRACT'; attractStart=ms(); special=false; montre=false; charge=0; affiche=0; }
function lanceReveal() { nombre=roll(N_VALUES[indexN]); special=false; montre=true; revMs=REVEAL_MS; phase='REVEAL'; revDebut=ms(); tRoll=ms(); flash=100; zoneDebut=0; }
function lanceSecret() { nombre=67; special=true; montre=true; specialStart=ms(); revMs=HEHE_MS; phase='REVEAL'; revDebut=ms(); tRoll=ms(); flash=100; zoneDebut=0; }
function dice_begin() { nombre=roll(N_VALUES[indexN]); phase='INTRO'; introStart=ms(); lastInput=ms(); charge=0; affiche=0; flash=0; special=false; montre=false; }

function dice_input(sens) {
  lastInput = ms();
  if (phase === 'INTRO') phase = 'ATTRACT';
  if (sens > 0) {
    if (enContact()) return;
    if (ms() - tRoll < COOLDOWN_MS) return;
    if (phase === 'ATTRACT') attractLeft = ms();
    if (phase === 'REVEAL' || phase === 'ATTRACT') { phase = 'CHARGE'; charge = 0; affiche = 0; }
    charge += pas();
    if (charge >= 100) { charge = 100; lanceReveal(); }
  } else if (sens < 0) {
    indexN = (indexN + 1) % (N_COUNT + 1);
  }
}

function dice_update() {
  if (phase === 'INTRO') { if (ms()-introStart >= INTRO_MS) versAttente(); return; }
  if (phase === 'CHARGE') {
    if (affiche < charge) { affiche += FILL_SPEED; if (affiche > charge) affiche = charge; }
    if (affiche > charge) { affiche -= FILL_SPEED; if (affiche < charge) affiche = charge; }
    if (charge >= SECRET_LO && charge <= SECRET_HI) { if (zoneDebut===0) zoneDebut=ms(); if (ms()-zoneDebut >= SECRET_MS) lanceSecret(); }
    else zoneDebut = 0;
    if (charge === 0 && ms()-lastInput >= ATTRACT_MS) versAttente();
  } else if (phase === 'REVEAL') {
    if (ms()-revDebut >= revMs) { if (special) versAttente(); else { phase='CHARGE'; charge=0; affiche=0; zoneDebut=0; } }
  }
  if (flash > 0) flash -= 5;
}

function dice_view() {
  const v = {};
  v.nombre = nombre; v.n = N_VALUES[indexN < N_COUNT ? indexN : 0]; v.flash = Math.max(0, flash);
  v.special = special; v.hehe = special && (ms()-specialStart < HEHE_MS); v.contact = enContact();
  v.intro = phase==='INTRO'; v.attract = phase==='ATTRACT'; v.enReveal = phase==='REVEAL';
  v.introProg = v.intro ? clamp((ms()-introStart)*100/INTRO_MS, 0, 100) : 0;
  if (v.enReveal) { const e = ms()-revDebut; v.reveal = clamp(revMs>0 ? e*100/revMs : 100, 0, 100); } else v.reveal = 0;
  v.charge = v.enReveal ? 100 : affiche;
  v.dwell = (zoneDebut && phase==='CHARGE') ? clamp((ms()-zoneDebut)*100/SECRET_MS, 0, 100) : 0;
  v.montreNombre = montre;
  v.attractAge = ms() - attractStart;
  if (phase === 'ATTRACT') v.attractFade = clamp((ms()-attractStart)*100/FADE_MS, 0, 100);
  else if (ms()-attractLeft < ATTRACT_OUT_MS) v.attractFade = clamp(100-(ms()-attractLeft)*100/ATTRACT_OUT_MS, 0, 100);
  else v.attractFade = 0;
  v.numFade = 100;
  if (phase==='CHARGE' && charge===0) { const idle = ms()-lastInput; if (idle > ATTRACT_MS-FADE_MS) v.numFade = clamp((ATTRACT_MS-idle)*100/FADE_MS, 0, 100); }
  if (phase==='REVEAL' && special) { const left = revMs-(ms()-revDebut); if (left < FADE_MS) v.numFade = clamp(left*100/FADE_MS, 0, 100); }
  return v;
}

// ============================================================
// RENDER  (port of ui.cpp)  — logo/icon are static images now
// ============================================================
function drawImg(img, cx, cy, w, alpha) {
  if (!img.complete || !img.naturalWidth) return;
  const h = w * img.naturalHeight / img.naturalWidth;
  ctx.globalAlpha = alpha / 100;
  ctx.drawImage(img, cx - w/2, cy - h/2, w, h);
  ctx.globalAlpha = 1;
}
const drawLogo = (cx, cy, a) => drawImg(logoImg, cx, cy, 158, a);
const drawIcon = (cx, cy, a) => drawImg(iconImg, cx, cy, 46, a);

function dessineAnneau(v, t, pulse, rainbow) {
  for (let a = 0; a < 360; a += 3) point(a, 113, 8, RAIL);
  const fin = v.enReveal ? 360 : v.charge*360/100;
  const retour = (v.enReveal && !v.special) ? clamp(v.reveal*4, 0, 100) : 0;
  const flashCol = v.special ? teinte(t/3) : [255,255,255];
  for (let a = 0; a < fin; a += 3) {
    let c;
    if (rainbow) c = teinte(a*2 + t/3);
    else { c = teinte(160 + a/2 + t/25); if (v.dwell > 0) c = melange(c, teinte(a*3 + t/2), v.dwell); }
    c = melange(c, [255,255,255], pulse);
    c = melange(c, flashCol, v.flash);
    c = melange(c, RAIL, retour);
    point(a, 113, 8, c);
  }
}

function fondu(prog, s, e, f) {
  if (prog <= s || prog >= e) return 0;
  let a = 100; if (prog < s+f) a = (prog-s)*100/f; if (prog > e-f) a = (e-prog)*100/f;
  return clamp(a, 0, 100);
}
function drapeauCH(cx, cy, s, a) {
  ctx.fillStyle = cssc(melange(FOND, [248,0,0], a)); ctx.fillRect(cx-s/2, cy-s/2, s, s);
  const t = (s/5|0) + 1; ctx.fillStyle = cssc(melange(FOND, [255,255,255], a));
  ctx.fillRect(cx-t/2, cy-s/3, t, 2*s/3); ctx.fillRect(cx-s/3, cy-t/2, 2*s/3, t);
}

function dessineIntro(prog, t) {
  ctx.fillStyle = cssc(FOND); ctx.fillRect(0, 0, 240, 240);
  setFont(22, 700); ctx.textAlign = 'left'; ctx.textBaseline = 'middle';
  const nom = "merlin-rce"; let x = 120 - ctx.measureText(nom).width/2;
  const sortie = prog <= 30 ? 100 : (38-prog)*100/8;
  for (let i = 0; i < nom.length; i++) {
    let la = clamp((prog-3-i)*100/10, 0, 100);
    ctx.fillStyle = cssc(melange(FOND, TEXTE, la*Math.max(0,sortie)/100));
    ctx.fillText(nom[i], x, 104); x += ctx.measureText(nom[i]).width;
  }
  const af = fondu(prog, 14, 38, 8); if (af > 0) drapeauCH(120, 150, 18, af);
  const a2 = fondu(prog, 40, 62, 8); if (a2 > 0) text("presents", 120, 120, melange(FOND, GRIS, a2), 16, 700);
  if (prog > 64) { let a3 = 100; if (prog < 78) a3 = (prog-64)*100/14; if (prog > 92) a3 = (100-prog)*100/8; drawLogo(120, 110, clamp(a3,0,100)); }
}

function dessineContact() {
  ctx.fillStyle = '#ffffff'; ctx.fillRect(0, 0, 240, 240);
  if (qrImg.complete && qrImg.naturalWidth) { const s = 166; ctx.drawImage(qrImg, 120-s/2, 120-s/2, s, s); }
}

function ui_draw(v) {
  const t = ms();
  if (v.intro)   { dessineIntro(v.introProg, t); return; }
  if (v.contact) { dessineContact(); return; }

  ctx.fillStyle = cssc(melange(FOND, v.special ? teinte(t/3) : [255,255,255], v.flash));
  ctx.fillRect(0, 0, 240, 240);

  let dx = 0, dy = 0, pulse = 0;
  if (!v.enReveal && v.charge > 65) {
    const tt = v.charge - 65, amp = 1 + (tt/12|0);
    dx = amp * Math.sin(t/22); dy = amp * Math.sin(t/19);
    pulse = (20 + 20*Math.sin(t/55)) * tt/35;
  }
  dessineAnneau(v, t, pulse, v.special);

  // le nombre
  if (!v.attract && v.montreNombre) {
    const by = (v.enReveal && v.reveal < 12) ? -(12-v.reveal) : 0;
    if (v.special) {
      text("67", 120+dx, 100+dy+by, melange(FOND, teinte(t/3+180), v.numFade), NUMPX, 800);
      if (v.hehe) text("hehe you found it", 120, 178, melange(FOND, OR, v.numFade), 12, 700);
    } else {
      text(String(v.nombre), 120+dx, 100+dy+by, melange(FOND, TEXTE, v.numFade), NUMPX, 800);
      text("1 in " + v.n, 120, 182, melange(FOND, GRIS, v.numFade), 16, 700);
    }
  }

  // au repos : juste le "D" en haut, et l'advice en gros (element principal)
  if (v.attractFade > 0) {
    drawIcon(120, 50, v.attractFade);
    let age = v.attractAge, idx, tt, per;
    if (age < FIRST_MS) { idx = 0; per = FIRST_MS; tt = age; }
    else { const k = ((age-FIRST_MS)/5000)|0; idx = (k+1)%4; per = 5000; tt = (age-FIRST_MS)%5000; }
    let br = 100; if (tt < 700) br = tt*100/700; else if (tt > per-700) br = (per-tt)*100/700;
    texteCentre(CONSEILS[idx], 120, 128, melange(FOND, TEXTE, br*v.attractFade/100), 16, 700);
  }
}

// ============================================================
// LOOP
// ============================================================
function frame() {
  if (powered) { dice_update(); ui_draw(dice_view()); }
  else { ctx.fillStyle = '#05070a'; ctx.fillRect(0, 0, 240, 240); }
  requestAnimationFrame(frame);
}
requestAnimationFrame(frame);

// ============================================================
// CONTROLS : power, mode toggle, encoder-drag / mouse-wheel
// ============================================================
const device = document.getElementById('device');
const pwr = document.getElementById('pwr');
const pwrHint = document.getElementById('pwrHint');
const modeToggle = document.getElementById('modeToggle');
const modeCaption = document.getElementById('modeCaption');
const info = document.getElementById('info');
const help = document.getElementById('help');
let mode = 'encoder';

function setPower(on) {
  powered = on; pwr.classList.toggle('on', on); pwrHint.classList.toggle('on', on);
  if (on) { t0 = performance.now(); acc = 0; angAcc = 0; dice_begin(); }
}
pwr.addEventListener('click', (e) => { e.stopPropagation(); setPower(!powered); });

// input mode : one small toggle (drag the ring  <->  scroll the wheel)
function setMode(m) {
  mode = m;
  device.classList.toggle('encoder', m === 'encoder');
  device.classList.toggle('mouse',   m === 'mouse');
  modeToggle.classList.toggle('mouse', m === 'mouse');
  modeCaption.textContent = m === 'mouse' ? 'scroll the wheel to spin' : 'drag the ring to spin';
}
modeToggle.addEventListener('click', () => setMode(mode === 'encoder' ? 'mouse' : 'encoder'));

// help popover (symbols, replaces the old wall of tips)
info.addEventListener('click', (e) => { e.stopPropagation(); help.classList.toggle('hidden'); });
document.addEventListener('click', (e) => { if (!help.contains(e.target) && e.target !== info) help.classList.add('hidden'); });

// ----- MOUSE mode : scroll wheel (one notch ~ one detent) -----
let acc = 0; const STEP = 100;
device.addEventListener('wheel', (e) => {
  if (mode !== 'mouse' || !powered) return;
  e.preventDefault();
  acc += e.deltaY;
  while (acc >=  STEP) { dice_input(-1); acc -= STEP; }   // scroll down -> change difficulty
  while (acc <= -STEP) { dice_input(+1); acc += STEP; }   // scroll up   -> charge & roll
}, { passive: false });

// ----- ENCODER mode : drag the ring around the screen -----
let dragging = false, lastAng = 0, angAcc = 0; const DEG = 14;   // degrees of turn per detent
function angleAt(e) {
  const r = device.getBoundingClientRect();
  return Math.atan2(e.clientY - (r.top + r.height/2), e.clientX - (r.left + r.width/2)) * 180 / Math.PI;
}
device.addEventListener('pointerdown', (e) => {
  if (mode !== 'encoder' || !powered || e.target === pwr) return;
  dragging = true; lastAng = angleAt(e); device.setPointerCapture(e.pointerId); e.preventDefault();
});
device.addEventListener('pointermove', (e) => {
  if (!dragging) return;
  let a = angleAt(e), d = a - lastAng;
  if (d > 180) d -= 360; if (d < -180) d += 360;       // unwrap
  angAcc += d; lastAng = a;
  while (angAcc >=  DEG) { dice_input(+1); angAcc -= DEG; }   // clockwise -> charge
  while (angAcc <= -DEG) { dice_input(-1); angAcc += DEG; }   // counter-cw -> difficulty
});
const stopDrag = () => { dragging = false; };
device.addEventListener('pointerup', stopDrag);
device.addEventListener('pointercancel', stopDrag);

// bonus : arrow keys
addEventListener('keydown', (e) => {
  if (!powered) return;
  if (e.key === 'ArrowUp'   || e.key === 'ArrowRight') dice_input(+1);
  if (e.key === 'ArrowDown' || e.key === 'ArrowLeft')  dice_input(-1);
});

})();
