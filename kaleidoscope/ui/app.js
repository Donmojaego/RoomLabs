"use strict";
// =============================================================================
// Kaleidoscope — interface. Pont avec le plugin (JUCE) ; sans plugin : aperçu autonome (mélodies simulées).
// =============================================================================
const NATIVE = !!(window.__JUCE__ && window.__JUCE__.backend);
const $ = s => document.querySelector(s), $$ = s => Array.from(document.querySelectorAll(s));
const QS = new URLSearchParams(location.search);
const ROOTS = ["Do", "Do#", "Ré", "Mib", "Mi", "Fa", "Fa#", "Sol", "Lab", "La", "Sib", "Si"];
const SCALES = ["Auto", "Majeur", "Mineur naturel", "Mineur harmonique", "Mineur mélodique", "Dorien", "Phrygien", "Lydien", "Mixolydien", "Locrien",
  "Penta mineure", "Penta majeure", "Blues", "Phrygien dominant", "Double harmonique", "Hirajoshi", "In sen", "Bhairav", "Kafi", "Par tons", "Hongroise mineure"];
const SCALE_IV = [[0,2,4,5,7,9,11],[0,2,3,5,7,8,10],[0,2,3,5,7,8,11],[0,2,3,5,7,9,11],[0,2,3,5,7,9,10],[0,1,3,5,7,8,10],[0,2,4,6,7,9,11],[0,2,4,5,7,9,10],
  [0,1,3,5,6,8,10],[0,3,5,7,10],[0,2,4,7,9],[0,3,5,6,7,10],[0,1,4,5,7,8,10],[0,1,4,5,7,8,11],[0,2,3,7,8],[0,1,5,7,10],[0,1,4,5,7,8,11],[0,2,3,5,7,9,10],[0,2,4,6,8,10],[0,2,3,6,7,8,11]];
const TABLES = ["Analogique", "Impulsion", "Formants", "Verre", "Orgue", "Numérique", "Chœur", "Spectral"];
const BARS = ["1 mesure", "2 mesures", "4 mesures", "8 mesures"];
const EVOLVE = ["Infini : arrêt", "Infini : chaque boucle", "Infini : toutes les 2", "Infini : toutes les 4", "Infini : toutes les 8"];
const LAYOUTS = [["retro", "Rétro"], ["telescope", "Télescope"], ["holo", "Holographique"], ["laiton", "Laiton & Verre"]];

// ---------------------------------------------------------------------------
// pont natif
const call = NATIVE ? (() => { let n = 0; const pend = new Map();
  window.__JUCE__.backend.addEventListener("__juce__complete", ({ promiseId, result }) => { const f = pend.get(promiseId); if (f) { pend.delete(promiseId); f(result); } });
  return (name, ...params) => new Promise(res => { const id = n++; pend.set(id, res); window.__JUCE__.backend.emitEvent("__juce__invoke", { name, params, resultId: id }); });
})() : null;
const on = (ev, fn) => { if (NATIVE) window.__JUCE__.backend.addEventListener(ev, fn); else Mock.listen(ev, fn); };
const initPrefs = NATIVE ? ((window.__JUCE__.initialisationData.prefs || [])[0] || {}) : {};
const VERSION = NATIVE ? ((window.__JUCE__.initialisationData.version || [])[0] || "") : "aperçu";
const api = (name, ...a) => NATIVE ? call(name, ...a) : Mock.call(name, ...a);

// ---------------------------------------------------------------------------
// état
const st = { params: {}, ranges: {}, presets: [], preset: 0, styles: null, pattern: null, banks: { installed: [] }, layout: "retro", prefs: {}, beat: 0, playing: false };
const knobs = {};           // id -> objet molette
const watchers = {};        // id -> [fonctions]
const watch = (id, fn) => (watchers[id] = watchers[id] || []).push(fn);

function toNorm(id, v) { const r = st.ranges[id]; if (!r) return v; const [a, b, , sk] = r; const x = Math.min(1, Math.max(0, (v - a) / (b - a))); return sk && sk !== 1 ? Math.pow(x, sk) : x; }
function fromNorm(id, n) { const r = st.ranges[id]; if (!r) return n; const [a, b, , sk] = r; n = Math.min(1, Math.max(0, n)); return a + (b - a) * (sk && sk !== 1 ? Math.pow(n, 1 / sk) : n); }
function setParam(id, v, fromHost) {
  st.params[id] = v;
  (watchers[id] || []).forEach(f => f(v));
  if (!fromHost) api("setParam", id, v);
}
const P = id => st.params[id] ?? 0;
const gesture = (id, b) => NATIVE && call("gesture", id, b);

// ---------------------------------------------------------------------------
// mise à l'échelle de la scène (fenêtre redimensionnable)
function fit() {
  const s = Math.min(innerWidth / 1100, innerHeight / 700);
  const stg = $("#stage");
  stg.style.transform = `scale(${s})`;
  stg.style.left = Math.max(0, (innerWidth - 1100 * s) / 2) + "px"; stg.style.top = Math.max(0, (innerHeight - 700 * s) / 2) + "px";
  KScope.resize(); drawRoll(); drawWave();
}
addEventListener("resize", fit);

// ---------------------------------------------------------------------------
// infobulles et messages
let tipT = 0;
document.addEventListener("pointerover", e => {
  const el = e.target.closest("[data-tip]"); const tip = $("#tip");
  clearTimeout(tipT);
  if (!el) { tip.classList.remove("show"); return; }
  tipT = setTimeout(() => {
    tip.textContent = el.dataset.tip;
    const r = el.getBoundingClientRect(), sr = $("#stage").getBoundingClientRect(), s = sr.width / 1100;
    let x = (r.left - sr.left) / s, y = (r.bottom - sr.top) / s + 8;
    if (y > 640) y = (r.top - sr.top) / s - 40;
    tip.style.left = Math.min(830, Math.max(6, x)) + "px"; tip.style.top = y + "px"; tip.classList.add("show");
  }, 550);
});
document.addEventListener("pointerdown", () => { clearTimeout(tipT); $("#tip").classList.remove("show"); });
let toastT = 0;
function toast(msg, ms = 2600) { const t = $("#toast"); t.textContent = msg; t.classList.add("show"); clearTimeout(toastT); toastT = setTimeout(() => t.classList.remove("show"), ms); }

// ---------------------------------------------------------------------------
// menus déroulants (style, humeur, gamme…) — un seul composant
let menuClose = null;
function closePops() { $$(".pop").forEach(p => p.classList.remove("show")); $("#veil").classList.remove("show"); if (menuClose) { const f = menuClose; menuClose = null; f(); } }
$("#veil").addEventListener("pointerdown", closePops);
$$("[data-close]").forEach(b => b.addEventListener("click", closePops));
document.addEventListener("keydown", e => { if (e.key === "Escape") closePops(); });
function openMenu(anchor, groups, current, pick, opts = {}) {
  closePops();
  const m = $("#menu"); m.innerHTML = "";
  const cols = opts.cols || 1;
  for (const g of groups) {
    if (g.title) { const h = document.createElement("h4"); h.textContent = g.title; m.appendChild(h); }
    const grid = document.createElement("div"); grid.className = "grid"; grid.style.gridTemplateColumns = `repeat(${g.cols || cols},1fr)`;
    for (const it of g.items) {
      const d = document.createElement("div"); d.className = "it" + (it.v === current ? " on" : "");
      d.innerHTML = it.label + (it.sub ? `<small>${it.sub}</small>` : "");
      d.addEventListener("click", () => { closePops(); pick(it.v); });
      grid.appendChild(d);
    }
    m.appendChild(grid);
  }
  const sr = $("#stage").getBoundingClientRect(), s = sr.width / 1100, r = anchor.getBoundingClientRect();
  const w = opts.width || Math.max(180, r.width / s);
  m.style.width = w + "px";
  let x = (r.left - sr.left) / s, y = (r.bottom - sr.top) / s + 6;
  x = Math.min(1100 - w - 8, Math.max(8, x));
  m.style.left = x + "px"; m.style.top = y + "px"; m.style.maxHeight = Math.max(160, 690 - y) + "px";
  if (y > 420 && opts.up !== false) { m.style.top = "auto"; m.style.bottom = (700 - (r.top - sr.top) / s + 6) + "px"; m.style.maxHeight = ((r.top - sr.top) / s - 14) + "px"; } else m.style.bottom = "auto";
  $("#veil").classList.add("show"); requestAnimationFrame(() => m.classList.add("show"));
  const cur = m.querySelector(".it.on"); if (cur) cur.scrollIntoView({ block: "nearest" });
}
function bindPick(sel, id, groupsFn, labelFn, opts) {
  const el = $(sel);
  const show = () => { el.textContent = labelFn(Math.round(P(id))); };
  watch(id, show);
  el.addEventListener("click", () => openMenu(el, groupsFn(), Math.round(P(id)), v => { setParam(id, v); afterChoice(id); }, opts));
  return show;
}
// changer de style, d'humeur, de gamme… relance une proposition (on entend tout de suite le résultat)
const REGEN = new Set(["style", "mood", "genMode", "root", "scale", "bars", "octave"]);
let regenT = 0;
function afterChoice(id) { if (REGEN.has(id)) { clearTimeout(regenT); regenT = setTimeout(() => doGenerate(true), 60); } }

// ---------------------------------------------------------------------------
// molettes (SVG ; image <layout>_molette.webp si fournie)
function fmt(id, v) {
  const r = st.ranges[id] || [0, 1];
  if (id === "levelDb") return (v > 0 ? "+" : "") + v.toFixed(1) + " dB";
  if (["aA", "aD", "aR", "fA", "fD", "fR", "glideMs", "granSize"].includes(id)) return v >= 1000 ? (v / 1000).toFixed(2) + " s" : Math.round(v) + " ms";
  if (id === "lfoRate") return v.toFixed(2) + " Hz";
  if (id === "granDensity") return Math.round(v) + " /s";
  if (id === "tempo") return Math.round(v) + " BPM";
  if (id === "semiB" || id === "smpTune" || id === "bendRange") return (v > 0 ? "+" : "") + Math.round(v) + " dt";
  if (id === "fineB") return (v > 0 ? "+" : "") + Math.round(v) + " ct";
  if (r[0] < 0) return (v > 0 ? "+" : "") + Math.round(v / Math.max(Math.abs(r[0]), r[1]) * 100) + " %";
  return Math.round((v - r[0]) / (r[1] - r[0]) * 100) + " %";
}
const ARC = 270, A0 = 135;
function arcPath(cx, cy, rad, a0, a1) {
  const p = a => [cx + rad * Math.cos(a * Math.PI / 180), cy + rad * Math.sin(a * Math.PI / 180)];
  const [x0, y0] = p(a0), [x1, y1] = p(a1); const large = Math.abs(a1 - a0) > 180 ? 1 : 0;
  return `M${x0.toFixed(2)} ${y0.toFixed(2)}A${rad} ${rad} 0 ${large} ${a1 > a0 ? 1 : 0} ${x1.toFixed(2)} ${y1.toFixed(2)}`;
}
function makeKnob(parent, id, label, size = 48) {
  const el = document.createElement("div"); el.className = "knob"; el.style.width = Math.max(64, size + 22) + "px";
  const R = size / 2, cx = R + 6, cy = R + 6, W = size + 12;
  el.innerHTML = `<svg width="${W}" height="${W}" viewBox="0 0 ${W} ${W}">
    <defs><radialGradient id="g_${id}" cx="40%" cy="32%" r="75%"><stop offset="0" stop-color="var(--knob1)"/><stop offset=".55" stop-color="var(--knob2)"/><stop offset="1" stop-color="var(--knob3)"/></radialGradient></defs>
    <path class="trk" d="${arcPath(cx, cy, R + 3, A0, A0 + ARC)}" stroke="var(--track)" stroke-width="3.5" fill="none" stroke-linecap="round"/>
    <path class="val" d="" stroke="var(--acc)" stroke-width="3.5" fill="none" stroke-linecap="round" style="filter:drop-shadow(0 0 3px var(--glow))"/>
    <g class="cap"><circle cx="${cx}" cy="${cy + 1.5}" r="${R - 3}" fill="rgba(0,0,0,.45)"/><circle cx="${cx}" cy="${cy}" r="${R - 3}" fill="url(#g_${id})" stroke="rgba(0,0,0,.45)"/>
    <circle cx="${cx}" cy="${cy}" r="${R - 9}" fill="none" stroke="rgba(255,255,255,.18)"/>
    <line class="ptr" x1="${cx}" y1="${cy - R + 7}" x2="${cx}" y2="${cy - R / 2.6}" stroke="#2a1a08" stroke-width="3" stroke-linecap="round"/></g></svg>
    <div class="k-lbl">${label}</div><div class="k-val"></div>`;
  parent.appendChild(el);
  const val = el.querySelector(".val"), cap = el.querySelector(".cap"), vt = el.querySelector(".k-val");
  const bip = (st.ranges[id] || [0])[0] < 0;
  let img = null;
  const k = {
    el, id, size,
    render(v) {
      const n = toNorm(id, v), a = A0 + ARC * n, mid = A0 + ARC * (bip ? toNorm(id, 0) : 0);
      val.setAttribute("d", Math.abs(a - mid) < .5 ? "" : arcPath(cx, cy, R + 3, Math.min(a, mid), Math.max(a, mid)));
      const rot = -135 + ARC * n;
      cap.setAttribute("transform", `rotate(${rot.toFixed(1)} ${cx} ${cy})`);
      if (img) img.style.transform = `rotate(${rot.toFixed(1)}deg)`;
      vt.textContent = fmt(id, v);
    },
    useImage(src) {
      if (!src) { if (img) { img.remove(); img = null; cap.style.display = ""; } return; }
      if (!img) { img = document.createElement("img"); img.className = "capImg"; el.appendChild(img); }
      img.src = src; img.style.left = (el.clientWidth / 2 - (R - 2)) + "px"; img.style.top = "4px"; img.style.width = img.style.height = (2 * R - 4) + 4 + "px";
      cap.style.display = "none"; k.render(P(id));
    }
  };
  watch(id, v => k.render(v));
  // glisser vertical, Maj = fin, double clic = valeur par défaut, molette de la souris
  let y0 = 0, n0 = 0;
  el.addEventListener("pointerdown", e => {
    e.preventDefault(); el.setPointerCapture(e.pointerId); el.classList.add("drag"); y0 = e.clientY; n0 = toNorm(id, P(id)); gesture(id, true);
    const move = ev => { const sens = ev.shiftKey ? 0.0012 : 0.0055; setParam(id, fromNorm(id, n0 + (y0 - ev.clientY) * sens)); };
    const up = () => { el.classList.remove("drag"); el.removeEventListener("pointermove", move); el.removeEventListener("pointerup", up); el.removeEventListener("pointercancel", up); gesture(id, false); };
    el.addEventListener("pointermove", move); el.addEventListener("pointerup", up); el.addEventListener("pointercancel", up);
  });
  el.addEventListener("dblclick", () => { const r = st.ranges[id]; if (r) { gesture(id, true); setParam(id, r[2]); gesture(id, false); } });
  el.addEventListener("wheel", e => { e.preventDefault(); setParam(id, fromNorm(id, toNorm(id, P(id)) - Math.sign(e.deltaY) * (e.shiftKey ? .005 : .025))); }, { passive: false });
  knobs[id] = k;
  return k;
}

// ---------------------------------------------------------------------------
// bascules et interrupteurs
function bindSeg(seg) {
  const id = seg.dataset.param;
  const btns = Array.from(seg.querySelectorAll("button"));
  btns.forEach(b => b.addEventListener("click", () => { setParam(id, +b.dataset.v); afterChoice(id); }));
  watch(id, v => btns.forEach(b => b.classList.toggle("on", +b.dataset.v === Math.round(v))));
}
function bindPow(b) {
  const id = b.dataset.param;
  b.addEventListener("click", () => setParam(id, P(id) > .5 ? 0 : 1));
  watch(id, v => { b.classList.toggle("on", v > .5); const p = b.closest(".panel"); if (p) p.classList.toggle("off", v <= .5); });
}

// ---------------------------------------------------------------------------
// presets et navigateur
function showPreset() {
  const p = st.presets[st.preset]; if (!p) return;
  $("#pnm").textContent = p.name;
  $("#pcat").textContent = (p.fond ? "Fond · " : "") + p.cat;
  refreshBank();
}
async function loadPreset(i) {
  i = (i + st.presets.length) % st.presets.length;
  const r = await api("loadPreset", i);
  st.preset = i;
  if (r && r.params) for (const k in r.params) setParam(k, r.params[k], true);
  showPreset();
  const p = st.presets[i];
  if (p && p.fond && !st.banks.installed.includes(p.bank)) toast("« " + p.name + " » : fond à installer (installeur des fonds Kaleidoscope). En attendant, le moteur joue sans échantillon.", 4200);
}
$("#prevP").addEventListener("click", () => loadPreset(st.preset - 1));
$("#nextP").addEventListener("click", () => loadPreset(st.preset + 1));
$("#pname").addEventListener("click", openBrowser); $("#browseP").addEventListener("click", openBrowser);
let bTab = "all", bCat = "";
function openBrowser() {
  closePops(); renderBrowser();
  $("#veil").classList.add("show"); requestAnimationFrame(() => $("#browser").classList.add("show"));
  setTimeout(() => $("#bSearch").focus(), 50);
}
$$("#browser .tabs button").forEach(b => b.addEventListener("click", () => { bTab = b.dataset.tab; bCat = ""; $$("#browser .tabs button").forEach(x => x.classList.toggle("on", x === b)); renderBrowser(); }));
$("#bSearch").addEventListener("input", renderBrowser);
$("#bOpenBanks").addEventListener("click", () => api("openFolder", "banks"));
function renderBrowser() {
  const q = $("#bSearch").value.trim().toLowerCase();
  const styleName = i => st.styles && i >= 0 ? st.styles.styles[i].name : "";
  const list = st.presets.map((p, i) => ({ ...p, i })).filter(p => bTab === "all" || (bTab === "fond") === !!p.fond);
  const cats = [...new Set(list.map(p => p.cat))];
  const cEl = $("#bCats"); cEl.innerHTML = "";
  const mk = (label, v, n) => { const d = document.createElement("div"); d.className = "it" + (bCat === v ? " on" : ""); d.innerHTML = label + `<small>${n}</small>`;
    d.addEventListener("click", () => { bCat = v; renderBrowser(); }); cEl.appendChild(d); };
  mk("Toutes les catégories", "", list.length);
  cats.forEach(c => mk(c, c, list.filter(p => p.cat === c).length));
  const shown = list.filter(p => (!bCat || p.cat === bCat) && (!q || (p.name + " " + p.cat + " " + styleName(p.style)).toLowerCase().includes(q)));
  const L = $("#bList"); L.innerHTML = "";
  for (const p of shown) {
    const miss = p.fond && !st.banks.installed.includes(p.bank);
    const d = document.createElement("div"); d.className = "it" + (p.i === st.preset ? " on" : "") + (miss ? " missing" : "");
    d.innerHTML = `<b>${p.name}</b><small>${p.cat}${p.style >= 0 ? " · " + styleName(p.style) : ""}${miss ? " · à installer" : ""}</small>`;
    d.addEventListener("click", () => { loadPreset(p.i); $$("#bList .it").forEach(x => x.classList.remove("on")); d.classList.add("on"); });
    d.addEventListener("dblclick", closePops);
    L.appendChild(d);
  }
  const nat = st.presets.filter(p => !p.fond).length, fon = st.presets.length - nat;
  $("#bCount").textContent = `${shown.length} affichés · ${nat} presets natifs + ${fon} fonds = ${st.presets.length} · ${st.banks.installed.length} fonds installés sur ${st.banks.total || fon}`;
}
function refreshBank() {
  const p = st.presets[st.preset] || {};
  const b = st.banks;
  const loadedName = b.loaded ? (st.presets.find(x => x.bank === b.loaded) || {}).name : "";
  if (b.loading) { $("#bankName").textContent = "Chargement du fond…"; $("#bankSub").textContent = (st.presets.find(x => x.bank === b.wanted) || {}).name || ""; }
  else if (b.error) { $("#bankName").textContent = "Fond non disponible"; $("#bankSub").textContent = b.error; }
  else if (loadedName) { $("#bankName").textContent = loadedName; $("#bankSub").textContent = P("smpOn") > .5 ? "Fond actif — joué par le moteur d'échantillons" : "Fond chargé (couche éteinte)"; }
  else { $("#bankName").textContent = "Aucun fond chargé"; $("#bankSub").textContent = `${b.installed.length} fonds installés — choisissez-en un dans le navigateur`; }
}
$("#bankState").addEventListener("click", () => { bTab = "fond"; $$("#browser .tabs button").forEach(x => x.classList.toggle("on", x.dataset.tab === "fond")); openBrowser(); });

// ---------------------------------------------------------------------------
// génération
async function doGenerate(quiet) {
  $("#genBtn").classList.remove("flash"); void $("#genBtn").offsetWidth; $("#genBtn").classList.add("flash");
  showPattern(await api("generate"));
  if (!quiet && P("play") < .5) setParam("play", 1);
}
$("#genBtn").addEventListener("click", () => doGenerate(false));
$("#declBtn").addEventListener("click", async () => showPattern(await api("decline")));
$("#prevBtn").addEventListener("click", async () => showPattern(await api("previous")));
$("#playBtn").addEventListener("click", () => setParam("play", P("play") > .5 ? 0 : 1));
watch("play", v => { $("#playBtn").classList.toggle("on", v > .5); $("#playIco").innerHTML = v > .5 ? '<rect x="6" y="5" width="4.5" height="14" rx="1"/><rect x="13.5" y="5" width="4.5" height="14" rx="1"/>' : '<path d="M7 4v16l13-8z"/>'; });
$$("[data-fb]").forEach(b => b.addEventListener("click", async () => {
  const k = b.dataset.fb;
  showPattern(await api("feedback", k));
  toast(k === "like" ? "Noté : Personal retient ce que vous aimez." : k === "keep" ? "Gardé : compte double dans Personal." : "Compris : Personal évitera ce genre de mélodie. Nouvelle proposition.");
}));
$("#resetPerso").addEventListener("click", async () => { showPattern(await api("resetPersonal")); toast("Personal remis à zéro : vos goûts repartent de rien."); });
$("#midiBtn").addEventListener("click", async () => {
  const f = await api("exportMidi");
  toast(f ? "Mélodie exportée : " + String(f).split(/[\\/]/).pop() : "Export impossible (aucune mélodie)", 3600);
});
// glisser la mélodie depuis le mini-rouleau vers une piste de l'hôte
$("#roll").addEventListener("pointerdown", e => {
  const x0 = e.clientX, y0 = e.clientY;
  const mv = ev => { if (Math.hypot(ev.clientX - x0, ev.clientY - y0) > 8) { cleanup(); if (NATIVE) call("dragMidi"); else toast("Glisser-déposer disponible dans le plugin"); } };
  const cleanup = () => { removeEventListener("pointermove", mv); removeEventListener("pointerup", cleanup); };
  addEventListener("pointermove", mv); addEventListener("pointerup", cleanup);
});
document.addEventListener("keydown", e => {
  if (e.target.tagName === "INPUT") return;
  if (e.code === "Space") { e.preventDefault(); doGenerate(false); }
  else if (e.key === "d" || e.key === "D") $("#declBtn").click();
  else if (e.key === "ArrowLeft" && e.ctrlKey) loadPreset(st.preset - 1);
  else if (e.key === "ArrowRight" && e.ctrlKey) loadPreset(st.preset + 1);
});

function showPattern(p) {
  if (!p || !p.notes) return;
  st.pattern = p;
  KScope.setPattern(p);
  const sty = st.styles ? st.styles.styles[p.style] : null, mood = st.styles ? st.styles.moods[p.mood] : null;
  if (sty) KScope.setPalette(sty.palette, mood ? mood.id : "", p.root);
  const sc = p.scale >= 0 ? SCALES[p.scale + 1] : "";
  $("#lensInfo").textContent = `${sty ? sty.name : ""} · ${mood ? mood.name : ""} · ${ROOTS[p.root] || ""} ${sc.toLowerCase()}`;
  markScale(p.root, p.scale);
  drawRoll();
}

// ---------------------------------------------------------------------------
// mini-rouleau : notes de la proposition + tête de lecture
function drawRoll() {
  const cv = $("#rollCv"), p = st.pattern; if (!cv) return;
  const r = cv.getBoundingClientRect(), dpr = Math.min(2, devicePixelRatio || 1) * ($("#stage").getBoundingClientRect().width / 1100 || 1) / ((r.width / cv.clientWidth) || 1);
  const W = Math.max(10, Math.round(cv.clientWidth * dpr)), H = Math.max(10, Math.round(cv.clientHeight * dpr));
  if (cv.width !== W || cv.height !== H) { cv.width = W; cv.height = H; }
  const c = cv.getContext("2d"); c.clearRect(0, 0, W, H);
  const g = c.createLinearGradient(0, 0, 0, H); g.addColorStop(0, "rgba(0,0,0,0)"); g.addColorStop(.45, "rgba(0,0,0,.55)"); g.addColorStop(1, "rgba(0,0,0,.75)");
  c.fillStyle = g; c.fillRect(0, 0, W, H);
  if (!p || !p.notes.length) return;
  const beats = p.beats || 16, pad = 14 * dpr, x = b => pad + (W - 2 * pad) * b / beats;
  let lo = 127, hi = 0; p.notes.forEach(n => { lo = Math.min(lo, n[2]); hi = Math.max(hi, n[2]); });
  lo -= 2; hi += 2; const y = m => H - 6 * dpr - (H * .62) * (m - lo) / Math.max(1, hi - lo);
  c.strokeStyle = "rgba(255,255,255,.12)"; c.lineWidth = 1;
  for (let b = 0; b <= beats; b += 4) { c.beginPath(); c.moveTo(x(b), H * .3); c.lineTo(x(b), H); c.stroke(); }
  const acc = getComputedStyle($("#stage")).getPropertyValue("--acc2").trim() || "#ffb347";
  const beat = st.beat;
  for (const n of p.notes) {
    const live = st.playing && beat >= n[0] && beat < n[0] + Math.max(.12, n[1]);
    c.fillStyle = live ? "#fff" : acc; c.globalAlpha = .45 + .55 * n[3];
    const hh = Math.max(2.5 * dpr, H * .045);
    c.beginPath(); c.roundRect ? c.roundRect(x(n[0]), y(n[2]) - hh / 2, Math.max(3 * dpr, x(n[0] + n[1]) - x(n[0]) - 1.5 * dpr), hh, 2 * dpr) : c.rect(x(n[0]), y(n[2]) - hh / 2, x(n[0] + n[1]) - x(n[0]), hh);
    c.fill();
  }
  c.globalAlpha = 1;
  if (st.playing) { c.fillStyle = "rgba(255,255,255,.85)"; c.fillRect(x(beat % beats) - dpr, H * .28, 2 * dpr, H * .72); }
}

// ---------------------------------------------------------------------------
// clavier jouable (3 octaves et demie), notes jouées allumées, notes de la gamme marquées
const KB_LO = 48, KB_HI = 96;
const keyEls = {};
function buildKeyboard() {
  const kb = $("#kb"); kb.innerHTML = "";
  const whites = []; for (let m = KB_LO; m <= KB_HI; m++) if (![1, 3, 6, 8, 10].includes(m % 12)) whites.push(m);
  const ww = kb.clientWidth / whites.length || 1048 / whites.length;
  whites.forEach((m, i) => { const d = document.createElement("div"); d.className = "wk"; d.style.left = (i * ww) + "px"; d.style.width = (ww - 2) + "px";
    if (m % 12 === 0) d.innerHTML = `<span>Do${Math.floor(m / 12) - 1}</span>`; kb.appendChild(d); keyEls[m] = d; d.dataset.m = m; });
  for (let m = KB_LO; m <= KB_HI; m++) if ([1, 3, 6, 8, 10].includes(m % 12)) {
    const wi = whites.indexOf(m - 1); const d = document.createElement("div"); d.className = "bk"; d.style.left = ((wi + 1) * ww - ww * .3) + "px"; d.style.width = (ww * .6) + "px";
    kb.appendChild(d); keyEls[m] = d; d.dataset.m = m; }
  let down = null;
  const play = m => { if (down === m) return; if (down !== null) { api("noteOff", down); keyEls[down].classList.remove("on"); } down = m; if (m !== null) { api("noteOn", m, .8); keyEls[m].classList.add("on"); KScope.note(m, .8); } };
  kb.addEventListener("pointerdown", e => { const k = e.target.closest("[data-m]"); if (!k) return; kb.setPointerCapture(e.pointerId); play(+k.dataset.m); });
  kb.addEventListener("pointermove", e => { if (down === null) return; const el = document.elementFromPoint(e.clientX, e.clientY); const k = el && el.closest && el.closest("[data-m]"); if (k) play(+k.dataset.m); });
  const up = () => play(null); kb.addEventListener("pointerup", up); kb.addEventListener("pointercancel", up);
}
function markScale(root, scale) {
  const iv = SCALE_IV[scale >= 0 ? scale : 0] || SCALE_IV[0];
  for (const m in keyEls) keyEls[m].classList.toggle("inScale", iv.includes(((m - root) % 12 + 12) % 12));
}
const lit = new Map();
function lightKey(m) { const k = keyEls[m]; if (!k) return; k.classList.add("on"); clearTimeout(lit.get(m)); lit.set(m, setTimeout(() => k.classList.remove("on"), 160)); }

// ---------------------------------------------------------------------------
// écrans des panneaux : forme d'onde (table + position), nuage de grains
function wavePoint(table, pos, x) {
  const s = Math.sin(2 * Math.PI * x), saw = 2 * (x - Math.floor(x + .5)), sq = x % 1 < .5 ? 1 : -1, tri = 1 - 4 * Math.abs(Math.round(x - .25) - (x - .25));
  switch (table) {
    case 0: return pos < .33 ? s * (1 - pos * 3) + tri * pos * 3 : pos < .66 ? tri * (2 - pos * 3) + saw * (pos * 3 - 1) : saw * (3 - pos * 3) + sq * (pos * 3 - 2) * .8;
    case 1: return x % 1 < .5 - .45 * pos ? .85 : -.85;
    case 2: return Math.sin(2 * Math.PI * x) * .5 + Math.sin(2 * Math.PI * x * (3 + pos * 6)) * .45 * Math.exp(-((x % 1) * 3));
    case 3: return s * .6 + Math.sin(2 * Math.PI * x * 3) * .25 + Math.sin(2 * Math.PI * x * (7 + 10 * pos)) * .15 * pos;
    case 4: return (s + Math.sin(4 * Math.PI * x) * .6 + Math.sin(6 * Math.PI * x) * .4 * (1 - pos) + Math.sin(16 * Math.PI * x) * .3 * pos) * .45;
    case 5: return Math.sin(2 * Math.PI * x + pos * 7 * Math.sin(4 * Math.PI * x)) * .9;
    case 6: return (saw * .4 + Math.sin(2 * Math.PI * x * 4) * .3 * (1 - pos) + Math.sin(2 * Math.PI * x * 6) * .3 * pos);
    default: { let v = 0; for (let h = 1; h < 12; h++) v += Math.sin(2 * Math.PI * x * h + h * h * (1 + pos)) * Math.abs(Math.sin(h * 2.3 + pos * 5)) / h; return v * .8; }
  }
}
function drawWave() {
  const cv = $("#waveCv"); if (!cv) return;
  const W = cv.width = cv.clientWidth * 2, H = cv.height = cv.clientHeight * 2, c = cv.getContext("2d");
  const acc = getComputedStyle($("#stage")).getPropertyValue("--acc2").trim();
  const t = Math.round(P("tableA")), pos = P("posA"), tb = Math.round(P("tableB")), pb = P("posB"), mix = P("mixB");
  c.lineWidth = 2.2; c.strokeStyle = acc; c.shadowColor = acc; c.shadowBlur = 8; c.beginPath();
  for (let i = 0; i <= W; i += 2) { const x = i / W * 2.5; const v = wavePoint(t, pos, x) * (1 - mix) + wavePoint(tb, pb, x) * mix; const y = H / 2 - v * H * .36; i ? c.lineTo(i, y) : c.moveTo(i, y); }
  c.stroke();
}
["tableA", "posA", "tableB", "posB", "mixB"].forEach(id => watch(id, () => requestAnimationFrame(drawWave)));
const grains = [];
function drawGrains(dt, energy) {
  const cv = $("#grainCv"); if (!cv || !cv.clientWidth) return;
  if (cv.width !== cv.clientWidth * 2) { cv.width = cv.clientWidth * 2; cv.height = cv.clientHeight * 2; }
  const W = cv.width, H = cv.height, c = cv.getContext("2d");
  c.fillStyle = "rgba(0,0,0,.25)"; c.fillRect(0, 0, W, H);
  if (P("granOn") < .5) return;
  const dens = P("granDensity"), spray = P("granSpray"), pos = P("granPos"), size = P("granSize");
  const n = dens * dt * (0.4 + energy * 2);
  for (let i = 0; i < n || (Math.random() < n - i); i++) grains.push({ x: (pos + (Math.random() - .5) * spray) * W, y: H * (.15 + .7 * Math.random()), a: 1, r: 2 + size / 40 });
  const acc = getComputedStyle($("#stage")).getPropertyValue("--acc2").trim();
  c.fillStyle = acc;
  for (const g of grains) { g.a -= dt * 2.5; c.globalAlpha = Math.max(0, g.a); c.beginPath(); c.arc(g.x, g.y, g.r * 2, 0, 7); c.fill(); }
  c.globalAlpha = 1;
  while (grains.length && grains[0].a <= 0) grains.shift();
  if (grains.length > 400) grains.splice(0, grains.length - 400);
}

// ---------------------------------------------------------------------------
// retour du moteur à 60 images/s
let lastHits = -1;
function onFb(f) {
  KScope.energy(f.e || 0);
  if (f.h !== lastHits) { if (lastHits >= 0) { KScope.note(f.n, f.vel); lightKey(f.n); } lastHits = f.h; }
  st.playing = f.s >= 0; st.beat = f.b || 0;
}
let lastT = performance.now(), energySm = 0;
function loop(now) {
  const dt = Math.min(.05, (now - lastT) / 1000); lastT = now;
  energySm += ((st.lastE || 0) - energySm) * .2;
  drawRoll(); drawGrains(dt, energySm);
  requestAnimationFrame(loop);
}

// ---------------------------------------------------------------------------
// layouts : un bouton (clic = suivant, clic droit / appui long = liste) ; images optionnelles
const ASSET_KEYS = ["fond", "logo", "lentille", "molette", "molette_grande", "generer_off", "generer_on", "bouton_off", "bouton_on", "voyant", "clavier"];
const assetCache = {};
function probe(name) {
  if (assetCache[name] !== undefined) return Promise.resolve(assetCache[name]);
  const tryExt = ext => new Promise(res => { const i = new Image(); i.onload = () => res("assets/" + name + "." + ext); i.onerror = () => res(null); i.src = "assets/" + name + "." + ext; });
  return tryExt("webp").then(r => r || tryExt("png")).then(r => (assetCache[name] = r));
}
async function applyAssets(layout) {
  const stg = $("#stage");
  const found = {};
  await Promise.all(ASSET_KEYS.map(async k => { found[k] = await probe(layout + "_" + k); }));
  if (st.layout !== layout) return;
  const set = (cls, img, src) => { stg.classList.toggle(cls, !!src); if (img && src) $(img).src = src; };
  set("hasFond", "#bgImg", found.fond); set("hasLogo", "#logoImg", found.logo); set("hasLentille", "#lensImg", found.lentille); set("hasClavier", "#kbImg", found.clavier);
  stg.classList.toggle("useGen", !!(found.generer_off && found.generer_on));
  if (found.generer_off) { stg.style.setProperty("--genOff", `url(${found.generer_off})`); stg.style.setProperty("--genOn", `url(${found.generer_on || found.generer_off})`); }
  stg.classList.toggle("useImg", !!(found.bouton_off && found.bouton_on));
  if (found.bouton_off) { stg.style.setProperty("--imgOff", `url(${found.bouton_off})`); stg.style.setProperty("--imgOn", `url(${found.bouton_on || found.bouton_off})`); }
  for (const id in knobs) knobs[id].useImage(knobs[id].size >= 56 ? (found.molette_grande || found.molette) : found.molette);
}
function setLayout(id, save = true) {
  if (!LAYOUTS.some(l => l[0] === id)) id = "retro";
  const stg = $("#stage");
  if (st.layout === id && stg.dataset.layout === id) return;
  st.layout = id;
  stg.classList.add("switching");
  setTimeout(() => {
    stg.dataset.layout = id;
    $("#layoutName").textContent = LAYOUTS.find(l => l[0] === id)[1];
    KScope.setMode(id);
    if (st.pattern) showPattern(st.pattern);
    applyAssets(id);
    requestAnimationFrame(() => { KScope.resize(); drawWave(); stg.classList.remove("switching"); });
  }, QS.has("gabarit") ? 0 : 200);
  if (save) api("setPref", "layout", id);
}
const lb = $("#layoutBtn");
lb.addEventListener("click", () => { const i = LAYOUTS.findIndex(l => l[0] === st.layout); setLayout(LAYOUTS[(i + 1) % LAYOUTS.length][0]); });
const layoutMenu = () => openMenu(lb, [{ title: "Layouts", items: LAYOUTS.map(l => ({ v: l[0], label: l[1] })) }], st.layout, v => setLayout(v), { width: 200 });
lb.addEventListener("contextmenu", e => { e.preventDefault(); layoutMenu(); });
let lpT = 0; lb.addEventListener("pointerdown", () => { lpT = setTimeout(() => { lpT = -1; layoutMenu(); }, 520); });
lb.addEventListener("pointerup", e => { if (lpT === -1) e.stopImmediatePropagation(); clearTimeout(lpT); });

// ---------------------------------------------------------------------------
// réglages et aide
$("#gearBtn").addEventListener("click", () => {
  const z = st.prefs.zoom || 100, vis = st.prefs.visual || "anime";
  openMenu($("#gearBtn"), [
    { title: "Taille de la fenêtre", cols: 3, items: [75, 100, 125, 150, 175, 200].map(v => ({ v: "z" + v, label: v + " %" })) },
    { title: "Animation", items: [["anime", "Animée (fluide)"], ["reduit", "Réduite (ordinateur lent)"], ["statique", "Fixe"]].map(([v, l]) => ({ v: "v" + v, label: l })) },
    { title: "Lecture", items: [{ v: "follow", label: (P("follow") > .5 ? "✓ " : "") + "Suivre le transport de l'hôte" }, { v: "midiOut", label: (P("midiOut") > .5 ? "✓ " : "") + "Envoyer les notes en MIDI" }] },
    { title: "Dossiers", items: [{ v: "fmidi", label: "Mélodies exportées (.mid)" }, { v: "fbanks", label: "Fonds installés" }, { v: "fassets", label: "Assets de layout (essai)" }] },
    { title: "Kaleidoscope " + VERSION, items: [{ v: "edit", label: "Éditer tout le son…" }] },
  ], z === 100 ? "z100" : "z" + z, v => {
    if (v[0] === "z") { st.prefs.zoom = +v.slice(1); api("setPref", "zoom", st.prefs.zoom); }
    else if (v[0] === "v") { st.prefs.visual = v.slice(1); api("setPref", "visual", st.prefs.visual); applyVisual(); }
    else if (v === "follow" || v === "midiOut") setParam(v, P(v) > .5 ? 0 : 1);
    else if (v[0] === "f") api("openFolder", v.slice(1));
    else if (v === "edit") openEditor();
  }, { width: 300 });
});
function applyVisual() { const v = st.prefs.visual || "anime"; KScope.setRunning(v !== "statique"); document.body.classList.toggle("reduce", v === "reduit"); }
$("#helpBtn").addEventListener("click", () => toast("GÉNÉRER (Espace) : nouvelle idée · D : déclinaison · Keys : tonique + gamme · Drop : déposez un son · J'aime / Garder / À refaire : Personal apprend · LAYOUT : change l'habillage sans rien perdre · glissez le rouleau vers votre logiciel", 7000));

// ---------------------------------------------------------------------------
// éditeur complet (tous les paramètres du son)
const GROUPS = [
  ["Oscillateurs", ["wtOn", "tableA", "posA", "tableB", "posB", "mixB", "semiB", "fineB", "unison", "detune", "spread", "sub", "noise", "wtEnv", "wtLfo"]],
  ["Filtre", ["fMode", "f24", "cutoff", "reso", "fEnv", "keytrack", "drive", "fA", "fD", "fS", "fR"]],
  ["Volume & LFO", ["aA", "aD", "aS", "aR", "lfoRate", "lfoVib", "lfoCut", "lfoAmp", "mono", "glideMs", "bendRange"]],
  ["Grains, fond, effets", ["granOn", "granSize", "granDensity", "granSpray", "granPitch", "granPos", "granMix", "smpOn", "smpLevel", "smpTune", "fxDrive", "chorus", "delay", "delayFb", "delayDiv", "reverb", "revSize", "width", "tilt", "levelDb"]],
];
const NAMES = { wtOn: "Oscillateurs", tableA: "Table A", posA: "Position A", tableB: "Table B", posB: "Position B", mixB: "Mélange B", semiB: "Demi-tons B", fineB: "Finesse B",
  unison: "Unisson", detune: "Désaccord", spread: "Largeur", sub: "Sub", noise: "Bruit", wtEnv: "Env > table", wtLfo: "LFO > table", fMode: "Type", f24: "24 dB",
  cutoff: "Coupure", reso: "Résonance", fEnv: "Enveloppe", keytrack: "Suivi clavier", drive: "Chaleur", fA: "Attaque", fD: "Déclin", fS: "Maintien", fR: "Relâche",
  aA: "Attaque", aD: "Déclin", aS: "Maintien", aR: "Relâche", lfoRate: "Vitesse LFO", lfoVib: "Vibrato", lfoCut: "LFO > filtre", lfoAmp: "Trémolo", mono: "Mono",
  glideMs: "Glissé", bendRange: "Pitch bend", granOn: "Grains", granSize: "Taille", granDensity: "Densité", granSpray: "Dispersion", granPitch: "Hauteur aléa.",
  granPos: "Position", granMix: "Mélange", smpOn: "Fond", smpLevel: "Niveau fond", smpTune: "Accord fond", fxDrive: "Saturation", chorus: "Chorus", delay: "Écho",
  delayFb: "Répétitions", delayDiv: "Division", reverb: "Espace", revSize: "Taille espace", width: "Largeur", tilt: "Couleur", levelDb: "Volume" };
const CHOICES = { tableA: TABLES, tableB: TABLES, fMode: ["Passe-bas", "Passe-bande", "Passe-haut"], delayDiv: ["1/16", "1/8", "1/8 p.", "1/4", "1/4 p.", "1/2"] };
const BOOLS = new Set(["wtOn", "f24", "mono", "granOn", "smpOn"]);
function openEditor() {
  closePops();
  const g = $("#eGrid"); g.innerHTML = "";
  for (const [title, ids] of GROUPS) {
    const box = document.createElement("div"); box.className = "eGrp"; box.innerHTML = `<h4>${title}</h4>`;
    for (const id of ids) {
      const row = document.createElement("div"); row.className = "eRow";
      const r = st.ranges[id] || [0, 1, 0, 1];
      if (BOOLS.has(id) || CHOICES[id]) {
        const opts = BOOLS.has(id) ? ["Non", "Oui"] : CHOICES[id];
        row.innerHTML = `<label>${NAMES[id]}</label><select style="grid-column:span 2;height:24px;border-radius:6px;background:rgba(255,255,255,.08);color:var(--darkInk);border:0">${opts.map((o, i) => `<option value="${i}">${o}</option>`).join("")}</select>`;
        const s = row.querySelector("select"); s.value = Math.round(P(id)); s.addEventListener("change", () => setParam(id, +s.value));
        watch(id, v => { s.value = Math.round(v); });
      } else {
        row.innerHTML = `<label>${NAMES[id]}</label><input type="range" min="0" max="1" step="0.001"><span></span>`;
        const i = row.querySelector("input"), sp = row.querySelector("span");
        const show = v => { i.value = toNorm(id, v); sp.textContent = id === "unison" ? Math.round(v) : fmt(id, v); };
        show(P(id)); watch(id, show);
        i.addEventListener("pointerdown", () => gesture(id, true)); i.addEventListener("pointerup", () => gesture(id, false));
        i.addEventListener("input", () => { let v = fromNorm(id, +i.value); if (id === "unison") v = Math.round(v); setParam(id, v); });
        i.addEventListener("dblclick", () => setParam(id, r[2]));
      }
      box.appendChild(row);
    }
    g.appendChild(box);
  }
  $("#veil").classList.add("show"); requestAnimationFrame(() => $("#editor").classList.add("show"));
}
$("#editBtn").addEventListener("click", openEditor);

// ---------------------------------------------------------------------------
// Drop : déposer un son, analyse, tonalité proposée
const dz = $("#dropZone");
["dragenter", "dragover"].forEach(ev => dz.addEventListener(ev, e => { e.preventDefault(); dz.classList.add("over"); }));
["dragleave", "drop"].forEach(ev => dz.addEventListener(ev, e => { e.preventDefault(); dz.classList.remove("over"); }));
dz.addEventListener("drop", e => { const f = e.dataTransfer.files && e.dataTransfer.files[0]; if (f) analyseFile(f); });
dz.addEventListener("click", () => $("#dropFile").click());
$("#dropFile").addEventListener("change", e => { const f = e.target.files[0]; if (f) analyseFile(f); e.target.value = ""; });
async function analyseFile(f) {
  $("#dropTxt").textContent = "Analyse de « " + f.name + " »…";
  try {
    const r = await DropAnalysis.analyse(await f.arrayBuffer());
    const txt = `${ROOTS[r.root]} ${r.minor ? "mineur" : "majeur"}${r.bpm ? " · " + r.bpm + " BPM" : ""} · confiance ${Math.round(r.confidence * 100)} %`;
    $("#dropTxt").textContent = f.name.slice(0, 28) + " → " + txt;
    showPattern(await api("setDrop", { root: r.root, scale: r.scale, bpm: r.tempoConfidence > .08 ? r.bpm : 0, confidence: r.confidence }));
    toast("Drop : " + txt + ". Mélodie construite autour. Corrigez la tonique ou la gamme en repassant sur Keys si besoin.", 4500);
  } catch (err) { $("#dropTxt").textContent = "Fichier illisible. Essayez un wav, mp3 ou flac."; }
}
watch("keyMode", v => $("#left").classList.toggle("dropMode", Math.round(v) === 1));

// ---------------------------------------------------------------------------
// aperçu sans plugin : backend simulé (mêmes appels, mêmes évènements)
const Mock = (() => {
  const ls = {}; let tick = 0, serial = 0, hist = [], cur = null, beat = 0, lastBeat = 0, liked = 0, disliked = 0;
  const emit = (ev, d) => (ls[ev] || []).forEach(f => f(d));
  const rnd = (a, b) => a + Math.random() * (b - a);
  function gen(declineFrom) {
    const style = Math.round(P("style")), mood = Math.round(P("mood")), root = declineFrom ? declineFrom.root : Math.round(P("root"));
    const scale = declineFrom ? declineFrom.scale : (Math.round(P("scale")) - 1 >= 0 ? Math.round(P("scale")) - 1 : [0, 1, 4, 9][Math.floor(Math.random() * 4)]);
    const bars = [1, 2, 4, 8][Math.round(P("bars"))] || 4, beats = bars * 4, iv = SCALE_IV[scale];
    let notes = [];
    if (declineFrom) notes = declineFrom.notes.map(n => Math.random() < .3 ? [n[0], n[1], n[2] + (Math.random() < .5 ? 2 : -1), n[3]] : n.slice());
    else { let deg = 7, t = 0; const acc = Math.round(P("genMode")) === 1;
      while (t < beats) { const len = [.5, .5, .25, 1, .75][Math.floor(Math.random() * 5)]; if (Math.random() > .15) {
        deg += Math.round(rnd(-2.4, 2.4)); deg = Math.max(0, Math.min(14, deg));
        const p = 60 + root + Math.floor(deg / iv.length) * 12 + iv[deg % iv.length] - 12 + 12 * Math.round(P("octave"));
        notes.push([t, len * .9, p, rnd(.55, 1)]); if (acc) { notes.push([t, len * .9, p - 3 - (deg % 2), .6]); notes.push([t, len * .9, p - 7, .55]); } }
        t += len; } }
    return { serial: ++serial, seed: Math.floor(Math.random() * 0xffffff).toString(16), style, mood, root, scale, bars, mode: Math.round(P("genMode")), beats, notes,
             feat: Array.from({ length: 10 }, () => Math.random()), liked, disliked, history: hist.length };
  }
  setInterval(() => {
    tick++;
    if (P("play") > .5 && cur) {
      const bpm = P("tempo") || 110; beat += bpm / 60 / 60;
      const L = cur.beats, b0 = lastBeat % L, b1 = beat % L;
      for (const n of cur.notes) if ((b0 <= b1 && n[0] >= b0 && n[0] < b1) || (b0 > b1 && (n[0] >= b0 || n[0] < b1))) { mock.hits++; mock.n = n[2]; mock.vel = n[3]; mock.e = Math.min(1, mock.e + .25 * n[3]); }
      lastBeat = beat;
    }
    mock.e *= .94;
    emit("fb", { e: mock.e, h: mock.hits, n: mock.n, vel: mock.vel, v: 0, l: mock.e, b: beat % (cur ? cur.beats : 16), s: P("play") > .5 ? 1 : -1, bpm: 110 });
  }, 1000 / 60);
  const mock = { hits: 0, n: 60, vel: .8, e: 0 };
  return {
    listen(ev, fn) { (ls[ev] = ls[ev] || []).push(fn); },
    async call(name, ...a) {
      switch (name) {
        case "getState": {
          const [styles, fac] = await Promise.all([fetch("styles.json").then(r => r.json()), fetch("presets/factory.json").then(r => r.json()).catch(() => ({ presets: [] }))]);
          const sIdx = id => styles.styles.findIndex(s => s.id === id), mIdx = id => styles.moods.findIndex(s => s.id === id);
          const presets = fac.presets.map(p => ({ id: p.id, name: p.name, cat: p.cat, fond: !!p.fond, bank: p.bank || "", style: sIdx(p.style), mood: mIdx(p.mood) }));
          const params = Object.assign({}, MOCK_DEFAULTS, (fac.presets[0] || {}).params || {}); for (const k in params) if (typeof params[k] === "boolean") params[k] = params[k] ? 1 : 0;
          return { params, ranges: MOCK_RANGES, presets, preset: 0, pattern: null, banks: { installed: [], total: 100, loaded: "" }, native: false, styles };
        }
        case "loadPreset": { const r = await fetch("presets/factory.json").then(r => r.json()); const p = r.presets[a[0]]; const params = {}; for (const k in p.params) params[k] = typeof p.params[k] === "boolean" ? +p.params[k] : p.params[k]; return { params, preset: a[0] }; }
        case "generate": if (cur) hist.push(cur); cur = gen(); return cur;
        case "decline": if (cur) hist.push(cur); cur = gen(cur); return cur;
        case "previous": if (hist.length) cur = hist.pop(); return cur;
        case "feedback": if (a[0] === "redo") { disliked++; cur = gen(); } else liked++; cur.liked = liked; cur.disliked = disliked; return cur;
        case "resetPersonal": liked = disliked = 0; return cur;
        case "setDrop": setParam("keyMode", 1, true); setParam("root", a[0].root, true); setParam("scale", a[0].scale + 1, true); cur = gen(); return cur;
        case "exportMidi": return "Kaleidoscope_apercu.mid";
        case "bankInfo": return { installed: [], total: 100 };
        default: return null;
      }
    }
  };
})();
const MOCK_RANGES = { posA: [0, 1, .3, 1], posB: [0, 1, .5, 1], mixB: [0, 1, .4, 1], semiB: [-24, 24, 0, 1], fineB: [-50, 50, 7, 1], unison: [1, 4, 2, 1], detune: [0, 1, .2, 1],
  spread: [0, 1, .6, 1], sub: [0, 1, .2, 1], noise: [0, 1, 0, 1], wtEnv: [-1, 1, 0, 1], wtLfo: [0, 1, 0, 1], cutoff: [0, 1, .7, 1], reso: [0, 1, .2, 1], fEnv: [-1, 1, .3, 1],
  keytrack: [0, 1, .5, 1], drive: [0, 1, .1, 1], aA: [.5, 8000, 5, .3], aD: [5, 10000, 600, .3], aS: [0, 1, .7, 1], aR: [5, 12000, 400, .3], fA: [.5, 8000, 2, .3], fD: [5, 10000, 500, .3],
  fS: [0, 1, .3, 1], fR: [5, 12000, 400, .3], lfoRate: [.02, 20, 4, .4], lfoVib: [0, 1, 0, 1], lfoCut: [0, 1, 0, 1], lfoAmp: [0, 1, 0, 1], granSize: [10, 400, 80, .5],
  granDensity: [2, 80, 30, .6], granSpray: [0, 1, .3, 1], granPitch: [0, 1, 0, 1], granPos: [0, 1, .3, 1], granMix: [0, 1, .5, 1], smpLevel: [0, 1.5, .8, 1], smpTune: [-24, 24, 0, 1],
  glideMs: [0, 1000, 0, .4], bendRange: [0, 24, 2, 1], fxDrive: [0, 1, 0, 1], chorus: [0, 1, 0, 1], delay: [0, 1, 0, 1], delayFb: [0, .92, .35, 1], reverb: [0, 1, .2, 1],
  revSize: [0, 1, .6, 1], width: [0, 1.5, .8, 1], tilt: [-1, 1, 0, 1], levelDb: [-30, 24, -6, 1], density: [-1, 1, 0, 1], variation: [0, 1, .5, 1], personal: [0, 1, .5, 1],
  humanize: [0, 1, .3, 1], gate: [.2, 1.5, 1, 1], tempo: [50, 200, 110, 1], octave: [-2, 2, 0, 1] };
const MOCK_DEFAULTS = { style: 11, mood: 4, genMode: 0, keyMode: 0, root: 9, scale: 0, bars: 2, density: 0, octave: 0, variation: .5, personal: .5, humanize: .3, gate: 1,
  play: 0, follow: 1, tempo: 110, evolve: 0, midiOut: 1 };

// ---------------------------------------------------------------------------
// démarrage
async function boot() {
  const s = await api("getState");
  st.ranges = s.ranges || {}; st.presets = s.presets || []; st.preset = s.preset || 0;
  st.styles = s.styles || await fetch("styles.json").then(r => r.json()).catch(() => null);
  st.banks = s.banks || { installed: [] };
  st.prefs = Object.assign({ layout: "retro", zoom: 100, visual: "anime" }, initPrefs);

  // sélecteurs
  const fams = st.styles.families;
  bindPick("#stylePick", "style", () => fams.map((f, fi) => ({ title: f, cols: 3, items: st.styles.styles.map((x, i) => ({ v: i, label: x.name, f: x.family })).filter(x => x.f === fi) })),
    i => st.styles.styles[i] ? st.styles.styles[i].name : "—", { width: 620 });
  bindPick("#moodPick", "mood", () => [{ cols: 2, items: st.styles.moods.map((m, i) => ({ v: i, label: m.name })) }], i => (st.styles.moods[i] || {}).name || "—", { width: 300 });
  bindPick("#rootPick", "root", () => [{ cols: 4, items: ROOTS.map((r, i) => ({ v: i, label: r })) }], i => ROOTS[i] || "Do", { width: 260 });
  bindPick("#scalePick", "scale", () => [{ cols: 2, items: SCALES.map((r, i) => ({ v: i, label: r })) }], i => SCALES[i] || "Auto", { width: 380 });
  bindPick("#barsPick", "bars", () => [{ items: BARS.map((b, i) => ({ v: i, label: b })) }], i => BARS[i] || "4 mesures", { width: 160 });
  bindPick("#octPick", "octave", () => [{ items: [-2, -1, 0, 1, 2].map(v => ({ v, label: "Octave " + (v > 0 ? "+" : "") + v })) }], i => "Octave " + (i > 0 ? "+" : "") + i, { width: 150 });
  bindPick("#evolvePick", "evolve", () => [{ items: EVOLVE.map((b, i) => ({ v: i, label: b })) }], i => EVOLVE[i] || EVOLVE[0], { width: 240 });
  bindPick("#tablePick", "tableA", () => [{ cols: 2, items: TABLES.map((b, i) => ({ v: i, label: b })) }], i => TABLES[i] || "Analogique", { width: 260 });
  $$(".seg[data-param]").forEach(bindSeg); $$(".pow[data-param]").forEach(bindPow);

  // molettes
  const kR = $("#knobsR");
  makeKnob(kR, "variation", "Variation", 58); makeKnob(kR, "personal", "Personal", 58); makeKnob(kR, "density", "Densité", 46); makeKnob(kR, "humanize", "Humain", 46);
  const kW = $("#knobsW"); [["posA", "Table"], ["cutoff", "Coupure"], ["reso", "Résonance"], ["fEnv", "Env"], ["drive", "Chaleur"]].forEach(([i, l]) => makeKnob(kW, i, l, 44));
  const kG = $("#knobsG"); [["granSize", "Taille"], ["granDensity", "Densité"], ["granSpray", "Texture"], ["granMix", "Mix"]].forEach(([i, l]) => makeKnob(kG, i, l, 44));
  const kF = $("#knobsF"); [["reverb", "Espace"], ["delay", "Écho"], ["chorus", "Chorus"], ["levelDb", "Volume"]].forEach(([i, l]) => makeKnob(kF, i, l, 44));
  watch("smpOn", refreshBank);

  // valeurs initiales
  for (const k in s.params) setParam(k, s.params[k], true);
  if (!NATIVE && QS.has("play")) setParam("play", 1, true);
  showPreset();
  buildKeyboard();
  KScope.init($("#kcanvas"));
  st.layout = "";
  setLayout(QS.get("layout") || QS.get("gabarit") || st.prefs.layout || "retro", false);
  applyVisual();
  if (s.pattern && s.pattern.notes) showPattern(s.pattern); else showPattern(await api("generate"));
  fit();

  on("params", o => { for (const k in o) setParam(k, o[k], true); });
  on("preset", i => { st.preset = i; showPreset(); });
  on("pattern", p => showPattern(p));
  on("banks", b => { st.banks = b; refreshBank(); if ($("#browser").classList.contains("show")) renderBrowser(); });
  on("fb", f => { st.lastE = f.e; onFb(f); });
  requestAnimationFrame(loop);
  if (QS.has("gabarit")) drawGabarit();
  window.KAL_TEST = { st, setLayout, doGenerate, KScope, openBrowser, openEditor, closePops, P };
}

// zones des assets (docs/gabarits) : affichées par-dessus l'interface, capturées à 2x
function drawGabarit() {
  document.body.classList.add("gabarit");
  const zones = [["logo", "#logo", "logo 760×150"], ["lentille / animation", "#lensWrap", "lentille 2200×1400 (centre transparent)"], ["GÉNÉRER", "#genBtn", "generer_off / _on 420×112"],
    ["bascule", "#segRole button", "bouton_off / _on 220×64"], ["molette", "#knobsW .knob svg", "molette 160×160"], ["grande molette", "#knobsR .knob svg", "molette_grande 240×240"],
    ["clavier", "#kbWrap", "clavier 2200×180"]];
  const sr = $("#stage").getBoundingClientRect(), s = sr.width / 1100;
  for (const [, sel, txt] of zones) { const el = $(sel); if (!el) continue; const r = el.getBoundingClientRect(); const d = document.createElement("div"); d.className = "gz";
    d.style.left = (r.left - sr.left) / s + "px"; d.style.top = (r.top - sr.top) / s + "px"; d.style.width = r.width / s + "px"; d.style.height = r.height / s + "px"; d.textContent = txt; $("#stage").appendChild(d); }
}
boot();
