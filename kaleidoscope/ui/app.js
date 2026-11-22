"use strict";
// =============================================================================
// Kaleidoscope — interface. Pont avec le plugin (JUCE) ; sans plugin : aperçu autonome (mélodies simulées).
// 4 habillages peints (assets/<layout>_*.webp, découpés par tools/prepare_assets.py) ; textes écrits par le code.
// =============================================================================
const NATIVE = !!(window.__JUCE__ && window.__JUCE__.backend);
const $ = s => document.querySelector(s), $$ = s => Array.from(document.querySelectorAll(s));
const QS = new URLSearchParams(location.search);
const W0 = 1000, H0 = 1083;
const ROOTS = ["Do", "Do#", "Ré", "Mib", "Mi", "Fa", "Fa#", "Sol", "Lab", "La", "Sib", "Si"];
const SCALE_SHORT = ["auto", "majeur", "mineur", "min. harm.", "min. mél.", "dorien", "phrygien", "lydien", "mixolyd.", "locrien", "penta min.", "penta maj.", "blues", "phryg. dom.", "double harm.", "hirajoshi", "in sen", "bhairav", "kafi", "par tons", "hongroise"];
const SCALES = ["Auto", "Majeur", "Mineur naturel", "Mineur harmonique", "Mineur mélodique", "Dorien", "Phrygien", "Lydien", "Mixolydien", "Locrien",
  "Penta mineure", "Penta majeure", "Blues", "Phrygien dominant", "Double harmonique", "Hirajoshi", "In sen", "Bhairav", "Kafi", "Par tons", "Hongroise mineure"];
const SCALE_IV = [[0,2,4,5,7,9,11],[0,2,3,5,7,8,10],[0,2,3,5,7,8,11],[0,2,3,5,7,9,11],[0,2,3,5,7,9,10],[0,1,3,5,7,8,10],[0,2,4,6,7,9,11],[0,2,4,5,7,9,10],
  [0,1,3,5,6,8,10],[0,3,5,7,10],[0,2,4,7,9],[0,3,5,6,7,10],[0,1,4,5,7,8,10],[0,1,4,5,7,8,11],[0,2,3,7,8],[0,1,5,7,10],[0,1,4,5,7,8,11],[0,2,3,5,7,9,10],[0,2,4,6,8,10],[0,2,3,6,7,8,11]];
const TABLES = ["Analogique", "Impulsion", "Formants", "Verre", "Orgue", "Numérique", "Chœur", "Spectral"];
const BARS = ["1 mesure", "2 mesures", "4 mesures", "8 mesures"];
const EVOLVE = ["Infini : arrêt", "Infini : chaque boucle", "Infini : toutes les 2", "Infini : toutes les 4", "Infini : toutes les 8"];
// ordre du bouton LAYOUT (cahier des charges) : Rétro 5 (défaut) → Labo minimal 3 → Holographique 4 → Télescope
const LAYOUTS = [["retro", "Rétro"], ["labo", "Labo"], ["holo", "Holographique"], ["telescope", "Télescope"]];

// ---------------------------------------------------------------------------
// placement des groupes pour chaque châssis (unités de la scène 1000 x 1083)
const GEOM = {
  retro: { logo: [26, 70, 240, 64], preset: [370, 50, 260, 88], master: [752, 70, 232, 92],
    style: [42, 262, 167, 140], mode: [42, 406, 167, 136], key: [42, 548, 167, 136],
    gen: [786, 262, 167, 172], ton: [786, 440, 167, 120], perso: [786, 566, 167, 120],
    engA: [54, 764, 420, 170], engB: [524, 764, 420, 170], misc: [200, 972, 600, 56], foot: [40, 958, 150, 92] },
  labo: { logo: [44, 98, 280, 62], preset: [380, 2, 240, 78], master: [716, 88, 254, 86], ton: [18, 6, 352, 70],
    style: [36, 200, 222, 110], mode: [36, 342, 222, 140], key: [36, 510, 222, 128],
    rounds: [740, 202, 226, 108], gen: [742, 344, 222, 138], perso: [740, 512, 226, 124],
    engA: [50, 716, 436, 242], engB: [522, 716, 432, 242], misc: [250, 1012, 720, 60], foot: [24, 1008, 210, 72] },
  holo: { logo: [26, 32, 320, 70], preset: [372, 24, 256, 102], master: [788, 4, 180, 124],
    style: [46, 258, 208, 124], mode: [42, 396, 190, 120], key: [42, 520, 190, 120],
    gen: [744, 258, 212, 124], rounds: [770, 396, 190, 96], ton: [770, 500, 190, 140], perso: [250, 1014, 720, 46],
    engA: [56, 698, 424, 190], engB: [522, 698, 424, 190], misc: [250, 956, 720, 46], foot: [24, 944, 210, 84] },
  telescope: { logo: [22, 22, 300, 70], preset: [370, 6, 224, 104], master: [806, 4, 184, 118],
    style: [602, 258, 188, 122], mode: [602, 388, 186, 126], key: [604, 520, 182, 118],
    engA: [150, 814, 330, 112, true], engB: [482, 814, 322, 112, true], togs: [834, 572, 160, 124],
    foot: [8, 954, 84, 122], gen: [96, 950, 170, 126], rounds: [268, 960, 210, 110], ton: [482, 958, 152, 116], perso: [638, 958, 172, 116], misc: [814, 958, 180, 116] },
};
// fenêtres de texte et zones cliquables des sélecteurs peints (fractions de l'image)
const SELG = {
  retro: { preset: { win: [.2, .3, .6, .4], arL: [.03, .26, .15, .48], arR: [.82, .26, .15, .48] }, menu: { win: [.11, .27, .63, .46] } },
  labo: { preset: { win: [.17, .3, .66, .4], arL: [.04, .3, .12, .4], arR: [.84, .3, .12, .4] }, menu: { win: [.09, .28, .66, .44] } },
  holo: { preset: { win: [.19, .45, .62, .38], tab: [.3, .02, .4, .2], arL: [.04, .42, .14, .46], arR: [.82, .42, .14, .46] },
          menu: { win: [.19, .45, .62, .38], tab: [.3, .02, .4, .2], arL: [.04, .42, .14, .46], arR: [.82, .42, .14, .46] } },
  telescope: { preset: { win: [.27, .37, .46, .34], cover: 1, tab: [.35, .05, .3, .17], arL: [.1, .33, .14, .42], arR: [.76, .33, .14, .42] },
               menu: { win: [.15, .4, .7, .3], cover: 1, tab: [.35, .05, .3, .17] } },
};
const PILLWIN = { retro: [.2, .3, .6, .4], labo: [.24, .3, .52, .4], holo: [.2, .3, .6, .4], telescope: [.2, .32, .6, .36] };
const TABBG = { holo: "#0b1236", telescope: "#2b1a0f" };

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
const st = { params: {}, ranges: {}, presets: [], preset: 0, styles: null, pattern: null, banks: { installed: [] }, layout: "", prefs: {}, beat: 0, playing: false, meta: {}, favs: new Set() };
const watchers = {};
const watch = (id, fn) => (watchers[id] = watchers[id] || []).push(fn);
function toNorm(id, v) { const r = st.ranges[id]; if (!r) return v; const [a, b, , sk] = r; const x = Math.min(1, Math.max(0, (v - a) / (b - a))); return sk && sk !== 1 ? Math.pow(x, sk) : x; }
function fromNorm(id, n) { const r = st.ranges[id]; if (!r) return n; const [a, b, , sk] = r; n = Math.min(1, Math.max(0, n)); return a + (b - a) * (sk && sk !== 1 ? Math.pow(n, 1 / sk) : n); }
function setParam(id, v, fromHost) { st.params[id] = v; (watchers[id] || []).forEach(f => f(v)); if (!fromHost) api("setParam", id, v); }
const P = id => st.params[id] ?? 0;
const gesture = (id, b) => NATIVE && call("gesture", id, b);
const A = name => `assets/${st.layout}_${name}.webp`;
const M = () => st.meta[st.layout] || {};
const ar = key => { const m = M()[key]; return m ? m[0] / m[1] : 2.5; };
const el = (tag, cls, html) => { const e = document.createElement(tag); if (cls) e.className = cls; if (html != null) e.innerHTML = html; return e; };
const box = (e, x, y, w, h) => { e.style.left = x + "px"; e.style.top = y + "px"; e.style.width = w + "px"; e.style.height = h + "px"; };
const place = (e, r, W, H) => box(e, r[0] * W, r[1] * H, r[2] * W, r[3] * H);

// ---------------------------------------------------------------------------
// mise à l'échelle (fenêtre redimensionnable, proportions fixes)
function fit() {
  const s = Math.min(innerWidth / W0, innerHeight / H0);
  const stg = $("#stage");
  stg.style.transform = `scale(${s})`;
  stg.style.left = Math.max(0, (innerWidth - W0 * s) / 2) + "px"; stg.style.top = Math.max(0, (innerHeight - H0 * s) / 2) + "px";
  KScope.resize(); drawRoll(); drawWave();
}
addEventListener("resize", fit);
const stageScale = () => $("#stage").getBoundingClientRect().width / W0;

// infobulles et messages
let tipT = 0;
document.addEventListener("pointerover", e => {
  const t = e.target.closest("[data-tip]"); const tip = $("#tip");
  clearTimeout(tipT);
  if (!t) { tip.classList.remove("show"); return; }
  tipT = setTimeout(() => {
    tip.textContent = t.dataset.tip;
    const r = t.getBoundingClientRect(), sr = $("#stage").getBoundingClientRect(), s = stageScale();
    let x = (r.left - sr.left) / s, y = (r.bottom - sr.top) / s + 8;
    if (y > H0 - 60) y = (r.top - sr.top) / s - 46;
    tip.style.left = Math.min(W0 - 290, Math.max(6, x)) + "px"; tip.style.top = y + "px"; tip.classList.add("show");
  }, 550);
});
document.addEventListener("pointerdown", () => { clearTimeout(tipT); $("#tip").classList.remove("show"); });
let toastT = 0;
function toast(msg, ms = 2600) { const t = $("#toast"); t.textContent = msg; t.classList.add("show"); clearTimeout(toastT); toastT = setTimeout(() => t.classList.remove("show"), ms); }

// ---------------------------------------------------------------------------
// menus déroulants
function closePops() { $$(".pop").forEach(p => p.classList.remove("show")); $("#veil").classList.remove("show"); }
$("#veil").addEventListener("pointerdown", closePops);
$$("[data-close]").forEach(b => b.addEventListener("click", closePops));
document.addEventListener("keydown", e => { if (e.key === "Escape") closePops(); });
function openMenu(anchor, groups, current, pick, opts = {}) {
  closePops();
  const m = $("#menu"); m.innerHTML = "";
  for (const g of groups) {
    if (g.title) m.appendChild(el("h4", "", g.title));
    const grid = el("div", "grid"); grid.style.gridTemplateColumns = `repeat(${g.cols || 1},1fr)`;
    for (const it of g.items) {
      const cur = g.current !== undefined ? g.current : current;
      const d = el("div", "it" + (it.v === cur ? " on" : ""), it.label + (it.sub ? `<small>${it.sub}</small>` : ""));
      d.addEventListener("click", () => { if (!g.keepOpen) closePops(); (g.pick || pick)(it.v); });
      grid.appendChild(d);
    }
    m.appendChild(grid);
  }
  const sr = $("#stage").getBoundingClientRect(), s = stageScale(), r = anchor.getBoundingClientRect();
  const w = opts.width || Math.max(200, r.width / s);
  m.style.width = w + "px";
  let x = (r.left - sr.left) / s + (r.width / s - w) / 2, yb = (r.bottom - sr.top) / s + 6, yt = (r.top - sr.top) / s - 6;
  x = Math.min(W0 - w - 8, Math.max(8, x));
  m.style.left = x + "px";
  if (yb < H0 * .58) { m.style.top = yb + "px"; m.style.bottom = "auto"; m.style.maxHeight = (H0 - yb - 12) + "px"; }
  else { m.style.top = "auto"; m.style.bottom = (H0 - yt) + "px"; m.style.maxHeight = (yt - 12) + "px"; }
  $("#veil").classList.add("show"); requestAnimationFrame(() => m.classList.add("show"));
  const cur = m.querySelector(".it.on"); if (cur) cur.scrollIntoView({ block: "nearest" });
}
// changer de style, d'humeur, de gamme… relance une proposition (on entend tout de suite le résultat)
const REGEN = new Set(["style", "mood", "genMode", "root", "scale", "bars", "octave"]);
let regenT = 0;
function afterChoice(id) { if (REGEN.has(id)) { clearTimeout(regenT); regenT = setTimeout(() => doGenerate(true), 60); } }
function choose(id, v) { setParam(id, v); afterChoice(id); }

// ---------------------------------------------------------------------------
// composants peints
const comps = [];     // chacun : { relayout() } appelé au changement de layout
function fmt(id, v) {
  const r = st.ranges[id] || [0, 1];
  if (id === "levelDb" || id === "master") return (v > 0 ? "+" : "") + v.toFixed(1) + " dB";
  if (["aA", "aD", "aR", "fA", "fD", "fR", "glideMs", "granSize"].includes(id)) return v >= 1000 ? (v / 1000).toFixed(2) + " s" : Math.round(v) + " ms";
  if (id === "lfoRate") return v.toFixed(2) + " Hz";
  if (id === "granDensity") return Math.round(v) + " /s";
  if (id === "tempo") return Math.round(v) + " BPM";
  if (id === "semiB" || id === "smpTune" || id === "bendRange") return (v > 0 ? "+" : "") + Math.round(v) + " dt";
  if (id === "fineB") return (v > 0 ? "+" : "") + Math.round(v) + " ct";
  if (r[0] < 0) return (v > 0 ? "+" : "") + Math.round(v / Math.max(Math.abs(r[0]), r[1]) * 100) + " %";
  return Math.round((v - r[0]) / (r[1] - r[0]) * 100) + " %";
}
function dragValue(target, id, axis, onMove) {
  let p0 = 0, n0 = 0;
  target.addEventListener("pointerdown", e => {
    if (e.button !== 0) return;
    e.preventDefault(); target.setPointerCapture(e.pointerId); target.classList.add("drag");
    p0 = axis === "x" ? e.clientX : e.clientY; n0 = toNorm(id, P(id)); gesture(id, true);
    const mv = ev => { const d = axis === "x" ? ev.clientX - p0 : p0 - ev.clientY; setParam(id, fromNorm(id, n0 + d * (ev.shiftKey ? .0012 : .0055) / Math.max(.5, stageScale()))); };
    const up = () => { target.classList.remove("drag"); target.removeEventListener("pointermove", mv); target.removeEventListener("pointerup", up); target.removeEventListener("pointercancel", up); gesture(id, false); };
    target.addEventListener("pointermove", mv); target.addEventListener("pointerup", up); target.addEventListener("pointercancel", up);
  });
  target.addEventListener("dblclick", () => { const r = st.ranges[id]; if (r) { gesture(id, true); setParam(id, r[2]); gesture(id, false); } });
  target.addEventListener("wheel", e => { e.preventDefault(); setParam(id, fromNorm(id, toNorm(id, P(id)) - Math.sign(e.deltaY) * (e.shiftKey ? .005 : .025))); }, { passive: false });
}

// sélecteur peint : fenêtre de texte, flèches, ouverture d'un menu
function makeSel(kind, label, textFn, menuFn, stepFn, tip) {
  const root = el("div", "ia sel"); if (tip) root.dataset.tip = tip;
  const bg = el("img", "bg"); root.appendChild(bg);
  const win = el("div", "win", `<span class="lab"></span><span class="val"></span>`); root.appendChild(win);
  const tab = el("div", "tab"); root.appendChild(tab);
  const arL = el("div", "hot"), arR = el("div", "hot"); root.appendChild(arL); root.appendChild(arR);
  const c = {
    el: root, kind, w: 0, h: 0,
    update() { win.querySelector(".val").textContent = textFn(); const g = (SELG[st.layout] || {})[kind] || {};
      win.querySelector(".lab").textContent = (g.tab || c.w * (g.win || [0, 0, 1])[2] < 190) ? "" : label; tab.textContent = g.tab ? label : ""; },
    size(w, h) {
      const k = kind === "preset" ? "sel_preset" : (c.alt ? "sel_menu2" : "sel_menu");
      const a = ar(k); let ww = w, hh = w / a; if (hh > h) { hh = h; ww = h * a; }
      c.w = ww; c.h = hh; root.style.width = ww + "px"; root.style.height = hh + "px"; bg.src = A(k);
      const g = (SELG[st.layout] || {})[kind] || SELG.retro[kind];
      place(win, g.win, ww, hh); win.classList.toggle("cover", !!g.cover);
      win.style.fontSize = Math.max(12, Math.min(15, hh * g.win[3] * .5)) + "px"; c.w = ww;
      if (g.tab) { place(tab, g.tab, ww, hh); tab.style.display = "flex"; tab.style.background = TABBG[st.layout] || "transparent"; tab.style.color = "var(--winInk)"; tab.style.fontSize = Math.max(11, hh * g.tab[3] * .55) + "px"; }
      else tab.style.display = "none";
      [arL, arR].forEach((a2, i) => { const r = g[i ? "arR" : "arL"]; if (r && stepFn) { place(a2, r, ww, hh); a2.style.display = "block"; } else a2.style.display = "none"; });
      c.update();
    }
  };
  root.addEventListener("click", e => { if (e.target === arL || e.target === arR) return; menuFn(root); });
  arL.addEventListener("click", e => { e.stopPropagation(); stepFn(-1); });
  arR.addEventListener("click", e => { e.stopPropagation(); stepFn(1); });
  comps.push(c);
  return c;
}
// pilule à deux états (Solo / Accomp, Keys / Drop)
function makePill(text, id, v, tip) {
  const root = el("div", "ia pill"); root.dataset.tip = tip || "";
  const off = el("img", "bg off"), onI = el("img", "bg on"); root.appendChild(off); root.appendChild(onI);
  const win = el("div", "win", text); root.appendChild(win);
  const c = { el: root, size(w, h) { const a = ar("pill"); let ww = w, hh = w / a; if (hh > h) { hh = h; ww = h * a; }
      box(root, 0, 0, ww, hh); root.style.position = "relative"; root.style.left = root.style.top = "";
      off.src = A("btn_off"); onI.src = A("btn_on"); place(win, PILLWIN[st.layout], ww, hh);
      win.style.fontSize = Math.max(12, Math.min(19, hh * .27 * (text.length > 5 ? .8 : 1))) + "px"; } };
  root.addEventListener("click", () => choose(id, v));
  watch(id, x => root.classList.toggle("on", Math.round(x) === v));
  comps.push(c); return c;
}
// bouton peint (générer, déclinaison, personal, layout) avec légende
function makeRound(key, caption, tip) {
  const root = el("div", "rb"), ia = el("div", "ia"), img = el("img", "bg"); ia.appendChild(img); root.appendChild(ia);
  const cap = el("div", "cap-lbl", caption); root.appendChild(cap); root.dataset.tip = tip || "";
  const c = { el: root, ia, size(w, h) { const a = ar(key); let ww = w, hh = w / a; const hb = h - (caption ? 16 : 0); if (hh > hb) { hh = hb; ww = hb * a; }
      ia.style.width = ww + "px"; ia.style.height = hh + "px"; img.src = A(key); cap.style.fontSize = (ww > 100 ? 13 : 12) + "px"; } };
  comps.push(c); return c;
}
// molette : anneau fixe + chapeau tourné (repère vers le bas sur l'image)
function makeKnob(id, label) {
  const root = el("div", "kn"), ring = el("div", "ring"), rimg = el("img"), cimg = el("img", "capi");
  ring.appendChild(rimg); ring.appendChild(cimg); root.appendChild(ring);
  root.appendChild(el("div", "k-lbl", label)); const vt = el("div", "k-val"); root.appendChild(vt);
  const render = v => { const n = toNorm(id, v); cimg.style.transform = `rotate(${(-135 + 270 * n + 180).toFixed(1)}deg)`; vt.textContent = fmt(id, v); };
  const c = { el: root, size(d) { const k = M().knob || { w: 1, h: 1, cx: .5, cy: .5, r: .3 }; const hh = d * k.h / k.w;
      ring.style.width = d + "px"; ring.style.height = hh + "px"; box(rimg, 0, 0, d, hh); rimg.src = A("knob");
      const cd = 2 * k.r * d; box(cimg, k.cx * d - cd / 2, k.cy * hh - cd / 2, cd, cd); cimg.src = A("cap"); render(P(id)); } };
  watch(id, render); dragValue(root, id, "y");
  comps.push(c); return c;
}
// interrupteur de moteur
function makeTog(id, label, tip) {
  const root = el("div", "tg"), ia = el("div", "ia"), off = el("img", "bg off"), onI = el("img", "bg on");
  ia.appendChild(off); ia.appendChild(onI); root.appendChild(ia); root.appendChild(el("span", "", label)); root.dataset.tip = tip || "";
  root.addEventListener("click", () => setParam(id, P(id) > .5 ? 0 : 1));
  watch(id, v => root.classList.toggle("on", v > .5));
  const c = { el: root, size(h) { const a = ar("tog"); ia.style.height = h + "px"; ia.style.width = h * a + "px"; off.src = A("tog_off"); onI.src = A("tog_on"); } };
  comps.push(c); return c;
}
const tb = (html, tip, fn) => { const b = el("button", "tb", html); if (tip) b.dataset.tip = tip; b.addEventListener("click", fn); return b; };
const ICON = {
  heart: '<svg viewBox="0 0 24 24" fill="currentColor"><path d="M12 21s-8-5.3-8-11a4.5 4.5 0 0 1 8-2.8A4.5 4.5 0 0 1 20 10c0 5.7-8 11-8 11z"/></svg>',
  star: '<svg viewBox="0 0 24 24" fill="currentColor"><path d="m12 2 3 7h7l-5.5 4.5L18.5 21 12 16.8 5.5 21l2-7.5L2 9h7z"/></svg>',
  x: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.6"><path d="M18 6 6 18M6 6l12 12"/></svg>',
  reset: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2"><path d="M3 12a9 9 0 1 0 3-6.7L3 8"/><path d="M3 3v5h5"/></svg>',
  lock: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2"><rect x="5" y="11" width="14" height="10" rx="2"/><path d="M8 11V7a4 4 0 0 1 8 0v4"/></svg>',
  back: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2"><path d="M9 14 4 9l5-5"/><path d="M4 9h10a6 6 0 0 1 0 12h-3"/></svg>',
  play: '<svg viewBox="0 0 24 24" fill="currentColor"><path d="M7 4v16l13-8z"/></svg>',
  stop: '<svg viewBox="0 0 24 24" fill="currentColor"><rect x="6" y="5" width="4.5" height="14" rx="1"/><rect x="13.5" y="5" width="4.5" height="14" rx="1"/></svg>',
  midi: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2"><path d="M12 3v12m0 0-4-4m4 4 4-4"/><path d="M4 17v3h16v-3"/></svg>',
  gear: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="3.2"/><path d="M12 2v3M12 19v3M2 12h3M19 12h3M4.9 4.9l2.1 2.1M17 17l2.1 2.1M4.9 19.1 7 17M17 7l2.1-2.1"/></svg>',
};

// ---------------------------------------------------------------------------
// construction des groupes (une fois) ; tailles recalculées à chaque layout
const UI = {};
function build() {
  const S = st.styles;
  UI.preset = makeSel("preset", "Preset", () => { const p = st.presets[st.preset]; return p ? p.name : "—"; }, openBrowser, d => loadPreset(st.preset + d),
    "Preset : flèches pour défiler, clic pour le navigateur (155 natifs + 125 fonds)");
  $("#gPreset").appendChild(UI.preset.el);
  const styleMenu = a => openMenu(a, S.families.map((f, fi) => ({ title: f, cols: 3, items: S.styles.map((x, i) => ({ v: i, label: x.name, f: x.family })).filter(x => x.f === fi) })),
    Math.round(P("style")), v => choose("style", v), { width: 640 });
  UI.style = makeSel("menu", "Style", () => (S.styles[Math.round(P("style"))] || {}).name || "—", styleMenu,
    d => choose("style", (Math.round(P("style")) + d + S.styles.length) % S.styles.length), "59 styles en 9 familles");
  UI.mood = makeSel("menu", "Humeur", () => (S.moods[Math.round(P("mood"))] || {}).name || "—",
    a => openMenu(a, [{ cols: 2, items: S.moods.map((m, i) => ({ v: i, label: m.name })) }], Math.round(P("mood")), v => choose("mood", v), { width: 320 }),
    d => choose("mood", (Math.round(P("mood")) + d + S.moods.length) % S.moods.length), "Humeur");
  UI.mood.alt = true;
  $("#gStyle").append(UI.style.el, UI.mood.el);
  UI.solo = makePill("Solo", "genMode", 0, "Solo : une mélodie, une note à la fois");
  UI.acc = makePill("Accomp", "genMode", 1, "Accompagnement : accords et voicings pour soutenir la voix");
  $("#gMode").append(UI.solo.el, UI.acc.el);
  UI.keys = makePill("Keys", "keyMode", 0, "Keys : vous choisissez la tonalité et la gamme");
  UI.drop = makePill("Drop", "keyMode", 1, "Drop : déposez un son, tonalité et tempo détectés, mélodie construite autour");
  $("#gKey").append(UI.keys.el, UI.drop.el);
  const tonMenu = a => openMenu(a, [
      { title: "Tonique", cols: 4, items: ROOTS.map((r, i) => ({ v: i, label: r })), current: Math.round(P("root")), pick: v => choose("root", v) },
      { title: "Gamme", cols: 2, items: SCALES.map((r, i) => ({ v: i, label: r })), current: Math.round(P("scale")), pick: v => choose("scale", v) }], null, null, { width: 400 });
  UI.ton = makeSel("menu", "Ton", () => ROOTS[Math.round(P("root"))] + " " + SCALE_SHORT[Math.round(P("scale"))], tonMenu,
    d => choose("root", (Math.round(P("root")) + d + 12) % 12), "Tonalité et gamme (Keys) — la mélodie reste dans la gamme");
  const lenMenu = a => openMenu(a, [
      { title: "Longueur", cols: 2, items: BARS.map((b, i) => ({ v: i, label: b })), current: Math.round(P("bars")), pick: v => choose("bars", v) },
      { title: "Octave", cols: 5, items: [-2, -1, 0, 1, 2].map(v => ({ v, label: (v > 0 ? "+" : "") + v })), current: Math.round(P("octave")), pick: v => choose("octave", v) }], null, null, { width: 300 });
  UI.len = makeSel("menu", "Boucle", () => { const o = Math.round(P("octave")); return [1, 2, 4, 8][Math.round(P("bars"))] + (Math.round(P("bars")) ? " mesures" : " mesure") + (o ? " · " + (o > 0 ? "+" : "") + o + " oct" : ""); }, lenMenu,
    d => choose("bars", Math.min(3, Math.max(0, Math.round(P("bars")) + d))), "Longueur de la boucle et octave");
  UI.len.alt = true;
  const ton = $("#gTon"); const inner = el("div"); inner.id = "tonInner"; inner.style.cssText = "position:relative;display:flex;flex-direction:column;align-items:center;gap:6px";
  const r1 = el("div"); r1.id = "tonRow1"; r1.appendChild(UI.ton.el);
  const dz = el("div", "", `<b>Déposez un son</b><span id="dropTxt">ou cliquez pour choisir (wav, mp3, flac…)</span>`); dz.id = "dropZone";
  inner.append(r1, UI.len.el, dz); ton.appendChild(inner);

  UI.gen = makeRound("generer", "Générer", "Nouvelle mélodie dans le style choisi (Espace) — la lecture démarre");
  UI.gen.ia.id = "genBtn";
  UI.decl = makeRound("decl", "Déclinaison", "Variation liée à la mélodie de référence (touche D). Clic droit : la mélodie actuelle devient la référence");
  UI.perso = makeRound("perso", "Personal", "Personal : les propositions suivent vos goûts (J'aime / Garder / À refaire)");
  const rr = el("div", "row"); rr.id = "roundsRow"; rr.style.gap = "10px"; rr.append(UI.decl.el, UI.perso.el); UI.rr = rr;
  $("#gGen").append(UI.gen.el, rr);
  const gR = el("div", "grp light"); gR.id = "gRounds"; $("#skin").appendChild(gR);
  const gT = el("div", "grp dark"); gT.id = "gTogs"; $("#skin").appendChild(gT);
  UI.gen.ia.addEventListener("click", () => doGenerate(false));
  UI.decl.ia.addEventListener("click", async () => showPattern(await api("decline")));
  UI.decl.ia.addEventListener("contextmenu", async e => { e.preventDefault(); showPattern(await api("lockRef")); toast("Cette mélodie devient la référence des déclinaisons."); });
  UI.perso.ia.addEventListener("click", () => { const onNow = P("personal") > .01; if (onNow) st.lastPerso = P("personal"); setParam("personal", onNow ? 0 : (st.lastPerso || .5));
    toast(onNow ? "Personal désactivé : propositions neutres." : "Personal activé : les propositions suivent vos goûts."); });
  watch("personal", v => UI.perso.el.classList.toggle("on", v > .01));

  const fb = async k => { showPattern(await api("feedback", k));
    toast(k === "like" ? "Noté : Personal retient ce que vous aimez." : k === "keep" ? "Gardé : compte double dans Personal." : "Compris : Personal évitera ce genre de mélodie. Nouvelle proposition."); };
  const p1 = el("div", "row"), p2 = el("div", "row");
  p1.append(tb(ICON.heart + "J'aime", "J'aime : Personal apprend vos goûts", () => fb("like")), tb(ICON.star + "Garder", "Garder : compte double", () => fb("keep")));
  p2.append(tb(ICON.x + "À refaire", "À refaire : Personal évite ce genre de mélodie et en propose une autre", () => fb("redo")),
            tb(ICON.reset, "Remettre Personal à zéro", async () => { showPattern(await api("resetPersonal")); toast("Personal remis à zéro."); }));
  const p3 = el("div", "row");
  UI.lock = tb(ICON.lock + "Référence", "La mélodie actuelle devient la référence des déclinaisons", async () => { showPattern(await api("lockRef")); toast("Référence verrouillée : les déclinaisons partent de cette mélodie."); });
  p3.append(UI.lock);
  $("#gPerso").append(p1, p2, p3);
  [p1, p2, p3].forEach(r => { r.style.gap = "6px"; r.style.width = "100%"; });

  // moteurs
  UI.tWt = makeTog("wtOn", "Wavetable", "Moteur wavetable / soustractif");
  UI.tGr = makeTog("granOn", "Grains", "Moteur granulaire (textures)");
  UI.tFo = makeTog("smpOn", "Fond", "Couche échantillonnée (fond chargé)");
  UI.kA = [["posA", "Table"], ["cutoff", "Coupure"], ["reso", "Résonance"], ["granMix", "Grains"]].map(([i, l]) => makeKnob(i, l));
  UI.kB = [["variation", "Variation"], ["density", "Densité"], ["reverb", "Espace"], ["delay", "Écho"]].map(([i, l]) => makeKnob(i, l));
  UI.scope = el("div", "scope", '<canvas id="waveCv"></canvas>'); UI.scope.dataset.tip = "Forme d'onde — clic : choisir la table";
  UI.scope.addEventListener("click", () => openMenu(UI.scope, [{ cols: 2, items: TABLES.map((t, i) => ({ v: i, label: t })) }], Math.round(P("tableA")), v => setParam("tableA", v), { width: 280 }));
  UI.gscope = el("div", "scope", '<canvas id="grainCv"></canvas>');
  UI.bank = el("div", "bankLine", '<b id="bankName">Aucun fond</b><br><span id="bankSub"></span>'); UI.bank.dataset.tip = "Fonds : instruments échantillonnés — clic pour les parcourir";
  UI.bank.addEventListener("click", () => { bTab = "fond"; openBrowser(); });
  watch("smpOn", refreshBank);

  // divers
  UI.play = tb(ICON.play + "Lecture", "Lecture / arrêt (suit le transport de votre logiciel quand il joue)", () => setParam("play", P("play") > .5 ? 0 : 1));
  watch("play", v => { UI.play.classList.toggle("on", v > .5); UI.play.innerHTML = (v > .5 ? ICON.stop + "Arrêt" : ICON.play + "Lecture"); });
  UI.back = tb(ICON.back + "Retour", "Revenir à la proposition précédente", async () => showPattern(await api("previous")));
  UI.evolve = tb("", "Infini : la mélodie évolue toute seule pendant la lecture", () => openMenu(UI.evolve, [{ items: EVOLVE.map((b, i) => ({ v: i, label: b })) }], Math.round(P("evolve")), v => setParam("evolve", v), { width: 240 }));
  watch("evolve", v => { UI.evolve.textContent = EVOLVE[Math.round(v)] || EVOLVE[0]; UI.evolve.classList.toggle("on", v > .5); });
  UI.midi = tb(ICON.midi + "MIDI", "Exporter la mélodie en .mid (Documents/Room Labs/Kaleidoscope/MIDI) — ou glissez le rouleau du hublot", async () => {
    const f = await api("exportMidi"); toast(f ? "Mélodie exportée : " + String(f).split(/[\\/]/).pop() : "Export impossible (aucune mélodie)", 3600); });
  $("#gMisc").append(UI.play, UI.back, UI.evolve, UI.midi);

  UI.layout = makeRound("layout", "Layout", "LAYOUT : Rétro → Labo → Holographique → Télescope. Clic droit : la liste. Rien d'autre ne change.");
  UI.layout.ia.addEventListener("click", () => { const i = LAYOUTS.findIndex(l => l[0] === st.layout); setLayout(LAYOUTS[(i + 1) % LAYOUTS.length][0]); });
  UI.layout.ia.addEventListener("contextmenu", e => { e.preventDefault(); openMenu(UI.layout.ia, [{ title: "Layouts", items: LAYOUTS.map(l => ({ v: l[0], label: l[1] })) }], st.layout, v => setLayout(v), { width: 210 }); });
  UI.gear = tb(ICON.gear, "Réglages : taille, animation, dossiers, édition du son", openGear);
  UI.help = tb("?", "Aide rapide", () => toast("GÉNÉRER (Espace) : nouvelle idée · DÉCLINAISON (D) : variante de la référence (clic droit : nouvelle référence) · Keys : tonalité + gamme · Drop : déposez un son · J'aime / Garder / À refaire : Personal apprend · LAYOUT : change l'habillage sans rien perdre · glissez le rouleau du hublot vers votre logiciel", 8000));
  const fs = el("div", "row"); fs.style.gap = "6px"; fs.append(UI.gear, UI.help);
  $("#gFoot").append(UI.layout.el, fs);
  buildMaster();
}

// MASTER : glissière (rétro, labo) ou cadran (holo, télescope)
function buildMaster() {
  const w = el("div", "ia"); w.id = "masterWrap"; w.dataset.tip = "MASTER : volume de sortie (glisser ; double clic = -4 dB). Le limiteur évite l'écrêtage.";
  const bg = el("img", "bg"), th = el("img", "thumb"), cap = el("img", "mcap"), lbl = el("div"), val = el("div", "mval");
  lbl.id = "masterLbl"; lbl.textContent = "MASTER";
  w.append(bg, th, cap, lbl, val); $("#gMaster").appendChild(w);
  const render = () => {
    const v = P("master"), n = toNorm("master", v), m = M(); val.textContent = fmt("master", v);
    if (m.slider) { const W = w.clientWidth, H = w.clientHeight, s = m.slider, d = s.thumb * W; box(th, (s.x0 + (s.x1 - s.x0) * n) * W - d / 2, s.cy * H - d / 2, d, d); }
    if (m.dial) cap.style.transform = `rotate(${(-135 + 270 * n).toFixed(1)}deg)`;
  };
  UI.master = { el: w, render, size(gw, gh) {
    const m = M(), dial = !!m.dial, a = ar("master"); let ww = gw, hh = gw / a; const hAvail = gh - (dial ? 0 : 18); if (hh > hAvail) { hh = hAvail; ww = hh * a; }
    w.classList.toggle("dial", dial); box(w, 0, dial ? 0 : 18, ww, hh); w.style.position = "relative"; w.style.left = ""; bg.src = A("master");
    th.style.display = dial ? "none" : "block"; cap.style.display = dial ? "block" : "none"; lbl.style.display = dial ? "none" : "block";
    if (dial) { const d = m.dial, cd = 2 * d.r * ww; box(cap, d.cx * ww - cd / 2, d.cy * hh - cd / 2, cd, cd); cap.src = A("mcap");
      if (st.layout === "telescope") { box(val, .37 * ww, .855 * hh, .26 * ww, .1 * hh); val.classList.add("cover"); } else { box(val, .3 * ww, .86 * hh, .4 * ww, .12 * hh); val.classList.remove("cover"); } }
    else { th.src = A("thumb"); box(lbl, 8, -18, 100, 16); box(val, ww - 110, -18, 100, 16); val.style.justifyContent = "flex-end"; val.classList.remove("cover"); }
    if (!dial) val.style.justifyContent = "flex-end"; else val.style.justifyContent = "center";
    requestAnimationFrame(render);
  } };
  watch("master", render);
  dragValue(w, "master", "x");
  w.addEventListener("pointerdown", () => { if (M().dial) {} });
}

// tailles et positions pour le layout courant
function layoutAll() {
  const g = GEOM[st.layout], m = M();
  const pos = (id, r) => { const e = $(id); box(e, r[0], r[1], r[2], r[3]); return { w: r[2], h: r[3], compact: !!r[4] }; };
  pos("#gLogo", g.logo);
  let r = pos("#gPreset", g.preset); UI.preset.size(r.w, r.h);
  r = pos("#gMaster", g.master); UI.master.size(r.w, r.h);
  r = pos("#gStyle", g.style); { const h = (r.h - 6) / 2; UI.style.size(r.w, h); UI.mood.size(r.w, h); $("#gStyle").style.gap = "6px"; }
  r = pos("#gMode", g.mode); { const h = (r.h - 6) / 2; UI.solo.size(r.w, h); UI.acc.size(r.w, h); $("#gMode").style.gap = "6px"; }
  r = pos("#gKey", g.key); { const h = (r.h - 6) / 2; UI.keys.size(r.w, h); UI.drop.size(r.w, h); $("#gKey").style.gap = "6px"; }
  r = pos("#gTon", g.ton); { const h = (r.h - 6) / 2; UI.ton.size(r.w, h); UI.len.size(r.w, h); const dz = $("#dropZone"); dz.style.height = UI.ton.h + "px"; }
  r = pos("#gGen", g.gen);
  { const G = $("#gGen"), sep = !!g.rounds, wide = !sep && r.w / r.h > 1.6;
    G.style.flexDirection = wide ? "row" : "column"; G.style.gap = wide ? "8px" : "4px";
    const capW = Math.min(104, (g.rounds ? g.rounds[2] : (r.w / r.h > 1.6 ? 208 : r.w)) / 2);    // largeur d'une légende (DÉCLINAISON)
    if (sep) {
      UI.gen.size(r.w, r.h); const R = $("#gRounds"); box(R, ...g.rounds); R.style.display = "flex"; R.appendChild(UI.rr);
      const d = Math.min(g.rounds[3] - 20, 64); UI.decl.size(d, d + 16); UI.perso.size(d, d + 16); UI.rr.style.gap = Math.max(6, (g.rounds[2] - 2 * capW)) + "px";
    } else {
      $("#gRounds").style.display = "none"; G.appendChild(UI.rr);
      if (wide) { UI.gen.size(r.w - 2 * capW - 8, r.h); const d = Math.min(r.h - 20, 60); UI.decl.size(d, d + 16); UI.perso.size(d, d + 16); UI.rr.style.gap = "0px"; }
      else { const hp = r.h * .55; UI.gen.size(r.w, hp); const d = Math.min(r.h - hp - 22, 60); UI.decl.size(d, d + 16); UI.perso.size(d, d + 16); UI.rr.style.gap = Math.max(0, r.w - 2 * capW) + "px"; }
    }
    [UI.decl.el, UI.perso.el].forEach(e => e.style.width = capW + "px"); }
  r = pos("#gPerso", g.perso);
  { const G = $("#gPerso"), wide = r.w / r.h > 4; G.style.gap = "6px"; G.style.flexDirection = wide ? "row" : "column";
    G.querySelectorAll(".row").forEach(rw => { rw.style.width = wide ? "auto" : "100%"; rw.style.flex = wide ? "1 1 auto" : "none"; });
    const bh = wide ? Math.min(36, r.h) : Math.min(32, (r.h - 12) / 3); G.querySelectorAll(".tb").forEach(b => b.style.height = bh + "px"); }
  // tonalité : en ligne si la zone est large (barre du haut du Labo)
  { const G = $("#gTon"), inner = $("#tonInner"), wide = r && g.ton[2] / g.ton[3] > 2.6; inner.style.flexDirection = wide ? "row" : "column";
    if (wide) { UI.ton.size(g.ton[2] / 2 - 4, g.ton[3]); UI.len.size(g.ton[2] / 2 - 4, g.ton[3]); $("#dropZone").style.width = UI.ton.w + "px"; $("#dropZone").style.right = "auto"; }
    else $("#dropZone").style.width = ""; }
  // interrupteurs à part (Télescope)
  { const T = $("#gTogs"); if (g.togs) { box(T, ...g.togs); T.style.display = "flex"; T.style.flexDirection = "column"; T.style.alignItems = "flex-start"; T.style.gap = "6px"; } else T.style.display = "none"; }
  $("#gTogs").innerHTML = "";
  for (const [id, togs, knobsL, extra] of [["#gEngA", [UI.tWt, UI.tGr], UI.kA, UI.scope], ["#gEngB", [UI.tFo], UI.kB, UI.bank]]) {
    r = pos(id, g[id === "#gEngA" ? "engA" : "engB"]);
    const G = $(id); G.innerHTML = ""; G.style.gap = "6px";
    if (r.compact) {
      G.style.flexDirection = "row";
      const T = $("#gTogs"); togs.forEach(t => { t.size(28); T.appendChild(t.el); }); if (id === "#gEngB") { UI.bank.style.maxWidth = "160px"; T.appendChild(UI.bank); }
      const kr = el("div", "row"); kr.style.cssText = "gap:0;width:100%;justify-content:space-around";
      const d = Math.min(r.w / knobsL.length - 22, r.h - 34);
      knobsL.forEach(k => { k.size(d); k.el.style.width = (r.w / knobsL.length) + "px"; kr.appendChild(k.el); }); G.append(kr);
    } else {
      G.style.flexDirection = "column";
      const head = el("div", "row"); head.style.cssText = "gap:14px;width:100%;justify-content:flex-start;padding:0 6px";
      const th = Math.min(34, r.h * .18); togs.forEach(t => { t.size(th); head.appendChild(t.el); });
      if (id === "#gEngB") { UI.bank.style.maxWidth = (r.w - 150) + "px"; head.appendChild(UI.bank); }
      G.appendChild(head);
      const hasScope = r.h > 200; const sh = hasScope ? Math.min(50, r.h * .2) : 0;
      if (hasScope) { const sc = id === "#gEngA" ? UI.scope : UI.gscope; sc.style.width = (r.w - 24) + "px"; sc.style.height = sh + "px"; G.appendChild(sc); }
      const kr = el("div", "row"); kr.style.cssText = "gap:4px;width:100%;justify-content:space-around";
      const d = Math.min(r.w / knobsL.length - 22, r.h - th - sh - 48);
      knobsL.forEach(k => { k.size(Math.max(40, d)); k.el.style.width = ""; kr.appendChild(k.el); }); G.appendChild(kr);
    }
  }
  r = pos("#gMisc", g.misc); { const G = $("#gMisc"); const col = r.w / r.h < 2.5; G.style.flexDirection = col ? "column" : "row"; G.style.flexWrap = "nowrap"; G.style.gap = "6px";
    G.querySelectorAll(".tb").forEach(b => { b.style.height = (col ? Math.min(30, (r.h - 18) / 4) : 34) + "px"; b.style.width = col ? "100%" : ""; b.style.flex = col ? "none" : "1 1 auto"; }); }
  r = pos("#gFoot", g.foot); { const G = $("#gFoot"); const col = r.w / r.h < 1.3; G.style.flexDirection = col ? "column" : "row"; G.style.gap = "8px";
    const d = Math.min(col ? r.w - 8 : r.h - 20, 58); UI.layout.size(d, d + 16); [UI.gear, UI.help].forEach(b => { b.style.height = "30px"; b.style.width = "38px"; b.style.flex = "none"; }); }
  // hublot : kaléidoscope sous le châssis, cadre par-dessus, rouleau et infos
  const [hx, hy, hr] = m.hole || [500, 440, 230];
  box($("#kWrap"), hx - hr - 6, hy - hr - 6, 2 * hr + 12, 2 * hr + 12);
  const cad = $("#cadre");
  if (m.cadre) { const cw = 2 * (hr + 3) / m.cadre.innerRatio; box(cad, hx - cw / 2, hy - cw / 2, cw, cw); cad.src = A("cadre"); cad.style.display = "block"; } else cad.style.display = "none";
  const rw = hr * 1.5; box($("#roll"), hx - rw / 2, hy + hr * .5, rw, hr * .42); $("#roll").style.borderRadius = `0 0 ${rw / 2}px ${rw / 2}px`;
  const li = $("#lensInfo"); box(li, hx - hr, hy - hr * .78, 2 * hr, 18);
  const rb = $("#refBadge"); rb.style.left = hx + "px"; rb.style.top = (hy - hr * .64) + "px";
  $("#chassis").src = A("chassis");
  KScope.resize(); drawWave(); drawRoll(); UI.master.render();
  comps.forEach(c => c.update && c.update());
}

// ---------------------------------------------------------------------------
// layouts
function setLayout(id, save = true) {
  if (!GEOM[id]) id = "retro";
  if (st.layout === id) return;
  const stg = $("#stage"), first = !st.layout;
  stg.classList.add("switching");
  setTimeout(() => {
    st.layout = id; stg.dataset.layout = id;
    KScope.setMode(id);
    layoutAll();
    if (st.pattern) showPattern(st.pattern);
    requestAnimationFrame(() => stg.classList.remove("switching"));
  }, first || QS.has("gabarit") ? 0 : 260);
  if (save) { api("setPref", "layout", id); toast("Layout : " + LAYOUTS.find(l => l[0] === id)[1], 1200); }
}

// ---------------------------------------------------------------------------
// presets et navigateur (favoris, écoute)
function showPreset() { UI.preset.update(); refreshBank(); }
async function loadPreset(i) {
  i = (i + st.presets.length) % st.presets.length;
  const r = await api("loadPreset", i);
  st.preset = i;
  if (r && r.params) for (const k in r.params) setParam(k, r.params[k], true);
  showPreset();
  const p = st.presets[i];
  if (p && p.fond && !st.banks.installed.includes(p.bank)) toast("« " + p.name + " » : fond à installer (installeur des fonds Kaleidoscope). En attendant, le moteur joue sans échantillon.", 4200);
}
let bTab = "all", bCat = "";
function openBrowser() {
  closePops(); $$("#browser .tabs button").forEach(x => x.classList.toggle("on", x.dataset.tab === bTab)); renderBrowser();
  $("#veil").classList.add("show"); requestAnimationFrame(() => $("#browser").classList.add("show"));
  setTimeout(() => $("#bSearch").focus(), 50);
}
$$("#browser .tabs button").forEach(b => b.addEventListener("click", () => { bTab = b.dataset.tab; bCat = ""; $$("#browser .tabs button").forEach(x => x.classList.toggle("on", x === b)); renderBrowser(); }));
$("#bSearch").addEventListener("input", renderBrowser);
$("#bOpenBanks").addEventListener("click", () => api("openFolder", "banks"));
function saveFavs() { api("setPref", "favs", [...st.favs].join(",")); }
async function audition(i) {
  await loadPreset(i);
  const pat = st.pattern; const root = 60 + (pat ? pat.root % 12 : 0);
  [0, 4, 7].forEach((iv, k) => setTimeout(() => { api("noteOn", root + iv, .75); setTimeout(() => api("noteOff", root + iv), 520); }, k * 170));
}
function renderBrowser() {
  const q = $("#bSearch").value.trim().toLowerCase();
  const styleName = i => st.styles && i >= 0 ? st.styles.styles[i].name : "";
  const list = st.presets.map((p, i) => ({ ...p, i })).filter(p => bTab === "all" || (bTab === "fav" ? st.favs.has(p.id) : (bTab === "fond") === !!p.fond));
  const cats = [...new Set(list.map(p => p.cat))];
  const cEl = $("#bCats"); cEl.innerHTML = "";
  const mk = (label, v, n) => { const d = el("div", "it" + (bCat === v ? " on" : ""), label + `<small>${n}</small>`); d.addEventListener("click", () => { bCat = v; renderBrowser(); }); cEl.appendChild(d); };
  mk("Toutes", "", list.length);
  cats.forEach(c => mk(c, c, list.filter(p => p.cat === c).length));
  const shown = list.filter(p => (!bCat || p.cat === bCat) && (!q || (p.name + " " + p.cat + " " + styleName(p.style)).toLowerCase().includes(q)));
  const L = $("#bList"); L.innerHTML = "";
  for (const p of shown) {
    const miss = p.fond && !st.banks.installed.includes(p.bank);
    const d = el("div", "it" + (p.i === st.preset ? " on" : "") + (miss ? " missing" : ""));
    const pl = el("button", "mini", "&#9654;"); pl.title = "Écouter";
    const fv = el("button", "mini fav" + (st.favs.has(p.id) ? " on" : ""), "&#9733;"); fv.title = "Favori";
    const tx = el("div", "tx", `<b>${p.name}</b><small>${p.fond ? "Fond · " : ""}${p.cat}${p.style >= 0 ? " · " + styleName(p.style) : ""}${miss ? " · à installer" : ""}</small>`);
    d.append(pl, tx, fv);
    pl.addEventListener("click", e => { e.stopPropagation(); audition(p.i); $$("#bList .it").forEach(x => x.classList.remove("on")); d.classList.add("on"); });
    fv.addEventListener("click", e => { e.stopPropagation(); st.favs.has(p.id) ? st.favs.delete(p.id) : st.favs.add(p.id); fv.classList.toggle("on"); saveFavs(); });
    d.addEventListener("click", () => { loadPreset(p.i); $$("#bList .it").forEach(x => x.classList.remove("on")); d.classList.add("on"); });
    d.addEventListener("dblclick", closePops);
    L.appendChild(d);
  }
  const nat = st.presets.filter(p => !p.fond).length, fon = st.presets.length - nat;
  $("#bCount").textContent = `${shown.length} affichés · ${nat} presets natifs + ${fon} fonds = ${st.presets.length} · ${st.banks.installed.length} fonds installés sur ${st.banks.total || fon} · ${st.favs.size} favoris`;
}
function refreshBank() {
  const b = st.banks, n = $("#bankName"), s = $("#bankSub"); if (!n) return;
  const loadedName = b.loaded ? (st.presets.find(x => x.bank === b.loaded) || {}).name : "";
  if (b.loading) { n.textContent = "Chargement…"; s.textContent = (st.presets.find(x => x.bank === b.wanted) || {}).name || ""; }
  else if (b.error) { n.textContent = "Fond non installé"; s.textContent = "Installeur des fonds"; }
  else if (loadedName) { n.textContent = loadedName; s.textContent = P("smpOn") > .5 ? "fond actif" : "couche éteinte"; }
  else { n.textContent = "Aucun fond"; s.textContent = `${b.installed.length} / ${b.total || 100} installés`; }
}

// ---------------------------------------------------------------------------
// génération
async function doGenerate(quiet) {
  const g = $("#genBtn"); g.classList.remove("flash"); void g.offsetWidth; g.classList.add("flash");
  showPattern(await api("generate"));
  if (!quiet && P("play") < .5) setParam("play", 1);
}
$("#roll").addEventListener("pointerdown", e => {
  const x0 = e.clientX, y0 = e.clientY;
  const mv = ev => { if (Math.hypot(ev.clientX - x0, ev.clientY - y0) > 8) { cleanup(); if (NATIVE) call("dragMidi"); else toast("Glisser-déposer disponible dans le plugin"); } };
  const cleanup = () => { removeEventListener("pointermove", mv); removeEventListener("pointerup", cleanup); };
  addEventListener("pointermove", mv); addEventListener("pointerup", cleanup);
});
document.addEventListener("keydown", e => {
  if (e.target.tagName === "INPUT") return;
  if (e.code === "Space") { e.preventDefault(); doGenerate(false); }
  else if (e.key === "d" || e.key === "D") api("decline").then(showPattern);
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
  $("#refBadge").classList.toggle("show", !!p.isRef);
  drawRoll();
}

// ---------------------------------------------------------------------------
// réglages
function openGear() {
  const z = st.prefs.zoom || 80;
  openMenu(UI.gear, [
    { title: "Taille de la fenêtre", cols: 3, items: [60, 70, 80, 90, 100, 120].map(v => ({ v: "z" + v, label: v + " %" })) },
    { title: "Animation", items: [["anime", "Animée (fluide)"], ["reduit", "Réduite (ordinateur lent)"], ["statique", "Fixe"]].map(([v, l]) => ({ v: "v" + v, label: l })) },
    { title: "Lecture", items: [{ v: "follow", label: (P("follow") > .5 ? "✓ " : "") + "Suivre le transport de l'hôte" }, { v: "midiOut", label: (P("midiOut") > .5 ? "✓ " : "") + "Envoyer les notes en MIDI" }] },
    { title: "Dossiers", items: [{ v: "fmidi", label: "Mélodies exportées (.mid)" }, { v: "fbanks", label: "Fonds installés" }] },
    { title: "Kaleidoscope " + VERSION, items: [{ v: "edit", label: "Éditer tout le son…" }] },
  ], "z" + z, v => {
    if (v[0] === "z") { st.prefs.zoom = +v.slice(1); api("setPref", "zoom", st.prefs.zoom); }
    else if (v[0] === "v") { st.prefs.visual = v.slice(1); api("setPref", "visual", st.prefs.visual); applyVisual(); }
    else if (v === "follow" || v === "midiOut") setParam(v, P(v) > .5 ? 0 : 1);
    else if (v[0] === "f") api("openFolder", v.slice(1));
    else if (v === "edit") openEditor();
  }, { width: 300 });
}
function applyVisual() { const v = st.prefs.visual || "anime"; KScope.setRunning(v !== "statique"); KScope.setQuality && KScope.setQuality(v === "reduit" ? .6 : 1); }

// ---------------------------------------------------------------------------
// retour du moteur
let lastHits = -1;
function onFb(f) {
  KScope.energy(f.e || 0);
  if (f.h !== lastHits) { if (lastHits >= 0) KScope.note(f.n, f.vel); lastHits = f.h; }
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
// démarrage
async function boot() {
  st.meta = await fetch("assets/meta.json").then(r => r.json()).catch(() => ({}));
  const s = await api("getState");
  st.ranges = s.ranges || {}; st.presets = s.presets || []; st.preset = s.preset || 0;
  st.styles = s.styles || await fetch("styles.json").then(r => r.json()).catch(() => null);
  st.banks = s.banks || { installed: [] };
  st.prefs = Object.assign({ layout: "retro", zoom: 80, visual: "anime", favs: "" }, initPrefs);
  st.favs = new Set(String(st.prefs.favs || "").split(",").filter(Boolean));
  build();
  for (const k in s.params) setParam(k, s.params[k], true);
  if (st.params.master === undefined) setParam("master", -4, true);
  if (!NATIVE && QS.has("play")) setParam("play", 1, true);
  KScope.init($("#kcanvas"));
  setLayout(QS.get("layout") || QS.get("gabarit") || st.prefs.layout || "retro", false);
  showPreset();
  applyVisual();
  if (s.pattern && s.pattern.notes) showPattern(s.pattern); else showPattern(await api("generate"));
  fit();
  watch("keyMode", v => $("#gTon").classList.toggle("dropMode", Math.round(v) === 1));
  $("#gTon").classList.toggle("dropMode", Math.round(P("keyMode")) === 1);
  bindDrop();
  on("params", o => { for (const k in o) setParam(k, o[k], true); });
  on("preset", i => { st.preset = i; showPreset(); });
  on("pattern", p => showPattern(p));
  on("banks", b => { st.banks = b; refreshBank(); if ($("#browser").classList.contains("show")) renderBrowser(); });
  on("fb", f => { st.lastE = f.e; onFb(f); });
  requestAnimationFrame(loop);
  if (QS.has("gabarit")) setTimeout(drawGabarit, 400);
  window.KAL_TEST = { st, setLayout, doGenerate, KScope, openBrowser, openEditor, closePops, P, UI };
}

// gabarits (docs/gabarits) : zones de chaque élément, avec le fichier et la taille utilisés
function drawGabarit() {
  document.body.classList.add("gabarit");
  const sr = $("#stage").getBoundingClientRect(), s = stageScale();
  const zone = (e, txt) => { if (!e) return; const r = e.getBoundingClientRect(); if (!r.width) return; const d = el("div", "gz", txt);
    box(d, (r.left - sr.left) / s, (r.top - sr.top) / s, r.width / s, r.height / s); $("#stage").appendChild(d); };
  const L = st.layout;
  zone($("#kWrap"), "ouverture : kaléidoscope (shader)"); zone($("#cadre"), `${L}_cadre`);
  zone(UI.preset.el, "sel_preset"); zone(UI.style.el, "sel_menu"); zone(UI.mood.el, "sel_menu2"); zone(UI.ton.el, "sel_menu"); zone(UI.len.el, "sel_menu2");
  zone(UI.solo.el, "btn_on / btn_off"); zone(UI.drop.el, "btn_on / btn_off");
  zone(UI.gen.ia, "generer"); zone(UI.decl.ia, "decl"); zone(UI.perso.ia, "perso"); zone(UI.layout.ia, "layout");
  zone(UI.kA[0].el.querySelector(".ring"), "knob + cap"); zone(UI.tWt.el.querySelector(".ia"), "tog_on / tog_off");
  zone($("#masterWrap"), "master"); zone($("#gPerso"), "boutons texte (code)"); zone($("#gMisc"), "boutons texte (code)"); zone($("#gLogo"), "logo (texte)");
}

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


function drawRoll() {
  const cv = $("#rollCv"), p = st.pattern; if (!cv) return;
  const r = cv.getBoundingClientRect(), dpr = Math.min(2, devicePixelRatio || 1) * ($("#stage").getBoundingClientRect().width / W0 || 1) / ((r.width / cv.clientWidth) || 1);
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
// Drop : déposer un son (sur la zone Drop ou n'importe où sur le plugin), analyse, tonalité proposée et corrigeable
function bindDrop() {
  const dz = $("#dropZone");
  ["dragenter", "dragover"].forEach(ev => document.addEventListener(ev, e => { e.preventDefault(); dz.classList.add("over"); }));
  ["dragleave", "drop"].forEach(ev => document.addEventListener(ev, e => { e.preventDefault(); dz.classList.remove("over"); }));
  document.addEventListener("drop", e => { const f = e.dataTransfer && e.dataTransfer.files && e.dataTransfer.files[0]; if (f && /audio|wav|mp3|flac|ogg|aif/i.test(f.type + f.name)) analyseFile(f); });
  dz.addEventListener("click", () => $("#dropFile").click());
  $("#dropFile").addEventListener("change", e => { const f = e.target.files[0]; if (f) analyseFile(f); e.target.value = ""; });
}
async function analyseFile(f) {
  const dt = $("#dropTxt"); dt.textContent = "Analyse de « " + f.name.slice(0, 24) + " »…";
  try {
    const r = await DropAnalysis.analyse(await f.arrayBuffer());
    const txt = `${ROOTS[r.root]} ${r.minor ? "mineur" : "majeur"}${r.bpm ? " · " + r.bpm + " BPM" : ""} (confiance ${Math.round(r.confidence * 100)} %)`;
    showPattern(await api("setDrop", { root: r.root, scale: r.scale, bpm: r.tempoConfidence > .08 ? r.bpm : 0, confidence: r.confidence }));
    $("#gTon").classList.add("analysed");
    dt.textContent = "Nouveau son : déposez-le ici";
    toast("Drop : " + txt + ". Mélodie construite autour. Tonalité modifiable avec le sélecteur Ton.", 5000);
  } catch (err) { dt.textContent = "Fichier illisible. Essayez un wav, mp3 ou flac."; }
}

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
  revSize: [0, 1, .6, 1], width: [0, 1.5, .8, 1], tilt: [-1, 1, 0, 1], levelDb: [-30, 24, -6, 1], master: [-36, 12, -4, 1], density: [-1, 1, 0, 1], variation: [0, 1, .5, 1], personal: [0, 1, .5, 1],
  humanize: [0, 1, .3, 1], gate: [.2, 1.5, 1, 1], tempo: [50, 200, 110, 1], octave: [-2, 2, 0, 1] };
const MOCK_DEFAULTS = { style: 11, mood: 4, genMode: 0, keyMode: 0, root: 9, scale: 0, bars: 2, density: 0, octave: 0, variation: .5, personal: .5, humanize: .3, gate: 1,
  play: 0, follow: 1, tempo: 110, evolve: 0, midiOut: 1, master: -4 };

boot();
