"use strict";
// =============================================================================
// Room Labs Audio — bibliothèque d'interface commune (plugins de la suite)
// Calques du pack (face, cadres, sprites de commandes) + tout le dynamique en code :
// indicateurs, états, libellés, écrans, courbes, vumètres. Thèmes : vert (défaut), studio, pocket.
// =============================================================================
const RL = (() => {
  const NATIVE = !!(window.__JUCE__ && window.__JUCE__.backend);
  const $ = s => document.querySelector(s);
  const el = (t, c, h) => { const e = document.createElement(t); if (c) e.className = c; if (h != null) e.innerHTML = h; return e; };
  const call = NATIVE ? (() => { let n = 0; const pend = new Map();
    window.__JUCE__.backend.addEventListener("__juce__complete", ({ promiseId, result }) => { const f = pend.get(promiseId); if (f) { pend.delete(promiseId); f(result); } });
    return (name, ...params) => new Promise(res => { const id = n++; pend.set(id, res); window.__JUCE__.backend.emitEvent("__juce__invoke", { name, params, resultId: id }); });
  })() : null;
  const init0 = k => NATIVE ? ((window.__JUCE__.initialisationData[k] || [])[0]) : undefined;
  const st = { params: {}, ranges: {}, theme: "vert", zoom: 120, layout: null, mock: null, watchers: {}, fbHandlers: [] };
  const THEMES = [["vert", "Vert"], ["studio", "Studio"], ["pocket", "Pocket"]];
  const LOGO = '<svg viewBox="0 0 40 40" fill="none" stroke="var(--acc)" stroke-width="4" stroke-linejoin="round" stroke-linecap="round"><path d="M7 35V6h15a9 9 0 0 1 0 18H13"/><path d="M19 24l12 11"/><path d="M13 35V15"/></svg>';

  const watch = (id, f) => (st.watchers[id] = st.watchers[id] || []).push(f);
  const P = id => st.params[id] ?? 0;
  function set(id, v, fromHost) { st.params[id] = v; (st.watchers[id] || []).forEach(f => f(v)); if (!fromHost) api("setParam", id, v); }
  const api = (name, ...a) => NATIVE ? call(name, ...a) : (st.mock ? st.mock(name, ...a) : Promise.resolve(null));
  const plug = (name, ...a) => NATIVE ? call("call", name, ...a) : (st.mock ? st.mock("call", name, ...a) : Promise.resolve(null));
  const gesture = (id, b) => NATIVE && call("gesture", id, b);
  const R = id => st.ranges[id] || [0, 1, 0, 1];
  const toN = (id, v) => { const [a, b, , sk] = R(id); const x = Math.min(1, Math.max(0, (v - a) / (b - a))); return sk && sk !== 1 ? Math.pow(x, sk) : x; };
  const fromN = (id, n) => { const [a, b, , sk] = R(id); n = Math.min(1, Math.max(0, n)); return a + (b - a) * (sk && sk !== 1 ? Math.pow(n, 1 / sk) : n); };
  const A = name => `${name}`;
  let scale = 1;

  function fit() {
    const L = st.layout; if (!L) return;
    scale = Math.min(innerWidth / L.w, innerHeight / L.h);
    const s = $("#stage"); s.style.transform = `scale(${scale})`;
    s.style.left = Math.max(0, (innerWidth - L.w * scale) / 2) + "px"; s.style.top = Math.max(0, (innerHeight - L.h * scale) / 2) + "px";
    st.screens.forEach(sc => sc.resize());
  }
  addEventListener("resize", fit);

  // ---- messages, infobulles, menus
  let tT = 0;
  function toast(m, ms = 2400) { const t = $("#toast"); t.textContent = m; t.classList.add("show"); clearTimeout(tT); tT = setTimeout(() => t.classList.remove("show"), ms); }
  let tipT = 0;
  document.addEventListener("pointerover", e => {
    const t = e.target.closest && e.target.closest("[data-tip]"); const tip = $("#tip"); clearTimeout(tipT);
    if (!t) { tip.classList.remove("show"); return; }
    tipT = setTimeout(() => { const r = t.getBoundingClientRect(), sr = $("#stage").getBoundingClientRect();
      tip.textContent = t.dataset.tip; tip.style.left = Math.min(st.layout.w - 270, Math.max(4, (r.left - sr.left) / scale)) + "px";
      tip.style.top = Math.min(st.layout.h - 40, (r.bottom - sr.top) / scale + 6) + "px"; tip.classList.add("show"); }, 600);
  });
  document.addEventListener("pointerdown", () => { clearTimeout(tipT); $("#tip").classList.remove("show"); });
  function closeMenu() { $("#menu").classList.remove("show"); $("#veil").classList.remove("show"); }
  function menu(anchor, items, cur, pick, title) {
    const m = $("#menu"); m.innerHTML = title ? `<h4>${title}</h4>` : "";
    items.forEach((it, i) => { const d = el("div", "it" + (i === cur ? " on" : ""), it); d.onclick = () => { closeMenu(); pick(i); }; m.appendChild(d); });
    const r = anchor.getBoundingClientRect(), sr = $("#stage").getBoundingClientRect();
    m.style.left = Math.max(4, Math.min(st.layout.w - 200, (r.left - sr.left) / scale)) + "px";
    const y = (r.bottom - sr.top) / scale + 4; m.style.top = (y > st.layout.h * .6 ? Math.max(4, (r.top - sr.top) / scale - 8 - items.length * 30) : y) + "px";
    $("#veil").classList.add("show"); requestAnimationFrame(() => m.classList.add("show"));
  }

  // ---- commandes
  function fmt(id, v) {
    const f = (st.layout.formats || {})[id]; if (f) return f(v);
    const [a, b] = R(id); return Math.round((v - a) / (b - a) * 100) + " %";
  }
  function knob(o) {   // {id, label, x, y, size:1..4, d}
    const e = el("div", "el kn"); e.dataset.tip = o.tip || "";
    const body = el("div", "body"), img = el("img"); body.style.width = body.style.height = o.d + "px";
    const sw = o.d, c = sw / 2, rr = c * (1 - 14 / (st.ctl.knobs[(o.size || 1) - 1].size / 2)) * .98;
    body.innerHTML = `<svg viewBox="0 0 ${sw} ${sw}"><path class="trk" fill="none" stroke="var(--grid)" stroke-width="2.5" stroke-linecap="round"/><path class="arc" fill="none" stroke="var(--acc)" stroke-width="2.5" stroke-linecap="round" style="filter:drop-shadow(0 0 3px var(--glow))"/><line class="ind" stroke="var(--acc)" stroke-width="3" stroke-linecap="round"/></svg>`;
    body.prepend(img);
    const lbl = el("div", "lbl", o.label), v = el("div", "v");
    e.append(body, lbl, v);
    const arcR = rr + 5, pt = a => [c + arcR * Math.sin(a), c - arcR * Math.cos(a)];
    const arc = (a0, a1) => { const [x0, y0] = pt(a0), [x1, y1] = pt(a1); return `M${x0} ${y0}A${arcR} ${arcR} 0 ${Math.abs(a1 - a0) > Math.PI ? 1 : 0} 1 ${x1} ${y1}`; };
    const A0 = -0.75 * Math.PI, SPAN = 1.5 * Math.PI;
    body.querySelector(".trk").setAttribute("d", arc(A0, A0 + SPAN));
    const bip = R(o.id)[0] < 0 && R(o.id)[1] > 0;
    const render = val => {
      const n = toN(o.id, val), a = A0 + SPAN * n, mid = A0 + SPAN * (bip ? toN(o.id, 0) : 0);
      body.querySelector(".arc").setAttribute("d", Math.abs(a - mid) < .01 ? "" : arc(Math.min(a, mid), Math.max(a, mid)));
      const ind = body.querySelector(".ind"), r1 = rr * .35, r2 = rr * .82;
      ind.setAttribute("x1", c + r1 * Math.sin(a)); ind.setAttribute("y1", c - r1 * Math.cos(a)); ind.setAttribute("x2", c + r2 * Math.sin(a)); ind.setAttribute("y2", c - r2 * Math.cos(a));
      v.textContent = fmt(o.id, val);
    };
    watch(o.id, render);
    let y0 = 0, n0 = 0;
    e.addEventListener("pointerdown", ev => { if (ev.button) return; ev.preventDefault(); e.setPointerCapture(ev.pointerId); y0 = ev.clientY; n0 = toN(o.id, P(o.id)); gesture(o.id, true);
      const mv = m => set(o.id, fromN(o.id, n0 + (y0 - m.clientY) * (m.shiftKey ? .001 : .005) / Math.max(.5, scale)));
      const up = () => { e.removeEventListener("pointermove", mv); e.removeEventListener("pointerup", up); gesture(o.id, false); };
      e.addEventListener("pointermove", mv); e.addEventListener("pointerup", up); });
    e.addEventListener("dblclick", () => set(o.id, R(o.id)[2]));
    e.addEventListener("wheel", ev => { ev.preventDefault(); set(o.id, fromN(o.id, toN(o.id, P(o.id)) - Math.sign(ev.deltaY) * (ev.shiftKey ? .004 : .02))); }, { passive: false });
    const k = { e, o, theme() { img.src = A(`${st.theme}_knob${o.size || 1}.webp`); }, render };
    place(e, o.x - o.d / 2 - 20, o.y - o.d / 2, o.d + 40); e.style.width = (o.d + 40) + "px";
    st.parts.push(k); return k;
  }
  function key(o) {   // {id?, label, x, y, w, h, value?, toggle?, onClick?, choice?}
    const e = el("div", "el ky" + (o.choice ? " sel" : "")); e.dataset.tip = o.tip || "";
    const off = el("img", "off"), on = el("img", "on"), s = el("span", "", o.label);
    e.append(off, on, s); place(e, o.x, o.y, o.w, o.h);
    const k = { e, o, theme() { off.src = A(`${st.theme}_key${o.offSprite || 1}.webp`); on.src = A(`${st.theme}_key${o.onSprite || 2}n.webp`); } };
    if (o.id && o.choice) {
      const names = () => R(o.id)[4] || [];
      watch(o.id, v => { s.textContent = (o.prefix || "") + (names()[Math.round(v)] || ""); e.classList.toggle("on", o.litWhen ? o.litWhen(v) : true); });
      e.onclick = () => menu(e, names(), Math.round(P(o.id)), i => set(o.id, i), o.menuTitle);
    } else if (o.id) {
      const val = o.value ?? 1;
      watch(o.id, v => e.classList.toggle("on", o.toggle !== false && o.value === undefined ? v > .5 : Math.round(v) === val));
      e.onclick = () => o.value === undefined ? set(o.id, P(o.id) > .5 ? 0 : 1) : set(o.id, val);
    }
    if (o.onClick) e.addEventListener("click", () => o.onClick(k));
    st.parts.push(k); return k;
  }
  function led(o) {   // {x, y, d, on: fb => bool}
    const e = el("img", "el led dim"); place(e, o.x - o.d / 2, o.y - o.d / 2, o.d, o.d);
    const k = { e, o, theme() { e.src = A(`${st.theme}_led${o.sprite || 4}.webp`); }, fb(f) { e.classList.toggle("dim", !o.on(f)); } };
    st.parts.push(k); return k;
  }
  function label(o) { const e = el("div", "el tlbl", o.text); place(e, o.x, o.y, o.w || 200, 16); if (o.align) e.style.textAlign = o.align; return e; }
  // écran dynamique : canvas dans un cadre (index de cadre de la face ou rectangle)
  function screen(o) {   // {frame | rect, title, draw(ctx, w, h, fb, dpr), pad}
    const r = o.rect || st.meta.frames[o.frame], pad = o.pad ?? 3;
    const e = el("div", "scr"); place(e, r[0] + pad, r[1] + pad, r[2] - 2 * pad, r[3] - 2 * pad);
    if (o.radius != null) e.style.borderRadius = o.radius + "px";
    const cv = el("canvas"); e.appendChild(cv);
    if (o.title) e.appendChild(el("div", "ttl", o.title));
    const sc = { e, o, cv, last: null,
      resize() { const d = Math.min(3, (devicePixelRatio || 1) * scale); cv.width = Math.round((r[2] - 2 * pad) * d); cv.height = Math.round((r[3] - 2 * pad) * d); sc.d = d; sc.draw(sc.last); },
      draw(f) { sc.last = f; const c = cv.getContext("2d"); c.setTransform(1, 0, 0, 1, 0, 0); c.clearRect(0, 0, cv.width, cv.height); c.scale(sc.d, sc.d); o.draw(c, r[2] - 2 * pad, r[3] - 2 * pad, f || {}, sc); } };
    st.screens.push(sc); return sc;
  }
  function place(e, x, y, w, h) { e.style.left = x + "px"; e.style.top = y + "px"; if (w != null) e.style.width = w + "px"; if (h != null) e.style.height = h + "px"; $("#stage").appendChild(e); }
  const css = n => getComputedStyle($("#stage")).getPropertyValue(n).trim();

  function setTheme(t, save) {
    st.theme = t; $("#stage").dataset.theme = t; document.documentElement.dataset.theme = t;
    $("#face").src = A(`${t}_face.webp`); const cad = $("#cadres"); if (st.meta.frames.length) cad.src = A(`${t}_cadres.webp`); else cad.style.display = "none";
    st.parts.forEach(p => p.theme && p.theme());
    $("#thmName").textContent = THEMES.find(x => x[0] === t)[1];
    st.screens.forEach(s => s.draw(s.last));
    if (save) api("setPref", "theme", t);
  }

  async function start(layout) {
    st.layout = layout; st.parts = []; st.screens = [];
    const meta = await fetch("meta.json").then(r => r.json());
    st.meta = meta.products[layout.product]; st.ctl = meta.controls;
    layout.w = st.meta.size[0]; layout.h = st.meta.size[1];
    document.body.innerHTML = `<div id="stage" data-theme="vert" style="width:${layout.w}px;height:${layout.h}px"><img id="face" class="face"><img id="cadres" class="cadres"><div id="veil"></div><div id="menu" class="pop"></div><div id="toast"></div><div id="tip"></div></div>`;
    $("#veil").onclick = closeMenu;
    const s = NATIVE ? await call("getState") : await layout.mockState();
    st.ranges = s.ranges || {}; Object.assign(st.params, s.params || {});
    if (!NATIVE) st.mock = layout.mock;
    const h = layout.header;
    const hd = el("div", "hdr", `<div class="logo">${LOGO}<div class="brand">ROOM LABS<br>AUDIO</div></div><div class="sep"></div><div class="name">${layout.title}</div>`);
    place(hd, h.x, h.y); hd.style.height = "34px";
    const hb = el("div", "hbtns");
    const thm = el("button", "ib thm", `<i></i><span id="thmName"></span>`); thm.dataset.tip = "Thème : Vert, Studio ou Pocket (le son et les réglages ne changent pas)";
    thm.onclick = () => { const i = THEMES.findIndex(x => x[0] === st.theme); setTheme(THEMES[(i + 1) % 3][0], true); };
    const info = el("button", "ib", "i"); info.dataset.tip = "Aide"; info.onclick = () => toast(layout.help, 7000);
    const gear = el("button", "ib", '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="3.2"/><path d="M12 2v3M12 19v3M2 12h3M19 12h3M4.9 4.9l2.1 2.1M17 17l2.1 2.1M4.9 19.1 7 17M17 7l2.1-2.1"/></svg>');
    gear.dataset.tip = "Taille de la fenêtre";
    gear.onclick = () => menu(gear, ["80 %", "100 %", "120 %", "150 %", "180 %"], -1, i => api("setPref", "zoom", [80, 100, 120, 150, 180][i]), "TAILLE");
    hb.append(thm, info, gear); place(hb, h.bx, h.y + 3);
    layout.build(RL);
    const prefs = init0("prefs") || {};
    setTheme(new URLSearchParams(location.search).get("theme") || prefs.theme || "vert", false);
    for (const k in st.params) set(k, st.params[k], true);
    fit();
    const onFb = f => { st.screens.forEach(sc => sc.draw(f)); st.parts.forEach(p => p.fb && p.fb(f)); st.fbHandlers.forEach(fn => fn(f)); };
    if (NATIVE) {
      window.__JUCE__.backend.addEventListener("params", o => { for (const k in o) set(k, o[k], true); });
      window.__JUCE__.backend.addEventListener("fb", onFb);
    } else setInterval(() => onFb(layout.mockFb && layout.mockFb()), 33);
    window.RL_TEST = { st, setTheme, P, set };
  }
  return { start, knob, key, led, label, screen, set, P, watch, api, plug, toast, menu, css, onFb: f => st.fbHandlers.push(f), st, NATIVE };
})();
