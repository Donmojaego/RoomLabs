"use strict";
// =============================================================================
// Kaleidoscope — visualisation procédurale (WebGL2, repli Canvas 2D).
// Une seule animation pour les 4 layouts ; le layout choisit la projection :
//   0 rosace (Rétro), 1 tunnel profond (Télescope), 2 tunnel spectral (Holographique), 3 éclats de verre (Laiton & Verre).
// Réactions : énergie du son, chaque note (onde colorée selon la hauteur), graine et traits de la mélodie
// (nombre de miroirs, densité des éclats, vitesse), palette du style, humeur.
// =============================================================================
const KScope = (() => {
  const VS = `#version 300 es
in vec2 p; void main(){ gl_Position = vec4(p, 0., 1.); }`;
  const FS = `#version 300 es
precision highp float;
uniform vec2 res; uniform float t, energy, seg, mode, seed, dens, spin, warm, flash, depth, quality;
uniform vec3 c0, c1, c2, c3;
uniform vec4 waves[6];          // x: âge (s), y: teinte, z: force, w: angle
out vec4 o;
mat2 rot(float a){ float c = cos(a), s = sin(a); return mat2(c, -s, s, c); }
float h21(vec2 p){ p = fract(p * vec2(123.34, 456.21)); p += dot(p, p + 45.32); return fract(p.x * p.y); }
vec3 hue(float h){ return clamp(abs(mod(h * 6. + vec3(0., 4., 2.), 6.) - 3.) - 1., 0., 1.); }
vec3 pal(float h){ h = fract(h) * 4.; return h < 1. ? mix(c0, c1, h) : h < 2. ? mix(c1, c2, h - 1.) : h < 3. ? mix(c2, c3, h - 2.) : mix(c3, c0, h - 3.); }
// éclats triangulaires en couches, bord lumineux (verre teinté)
vec3 shards(vec2 q, float k){
  vec3 col = vec3(0.); float lead = 1.;
  for (int i = 0; i < 3; i++){
    float fi = float(i);
    vec2 g = q * (1.5 + fi * 1.3 + dens * 1.3) + vec2(seed * 7.1 + fi * 3.7, t * (0.03 + 0.02 * fi) * (1. + spin));
    g = rot(0.35 * fi + seed) * g;
    vec2 id = floor(g), f = fract(g);
    float tri = step(f.x, f.y);
    float h = h21(id + tri * 17.3 + fi * 31.);
    float e = min(min(f.x, f.y), min(1. - f.x, 1. - f.y)); e = min(e, abs(f.x - f.y) * .7071);
    float l = smoothstep(0.012, 0.045, e);
    float tw = .7 + .3 * sin(t * (0.5 + h * 1.4) + h * 40. + energy * 3.);
    vec3 c = pal(h + fi * .27 + k) * (0.75 + 0.55 * h) * tw;
    c += vec3(1.) * pow(smoothstep(0.5, 0.0, abs(e - .16)), 6.) * .12;          // reflet dans le verre
    float cover = i == 0 ? 1. : step(.42 + .1 * fi, h);                          // couches partielles : profondeur
    col = mix(col, mix(col, c, .78), cover);
    lead *= mix(1., mix(.12, 1., l), cover);
  }
  return col * lead;
}
void main(){
  vec2 uv = (gl_FragCoord.xy - .5 * res) / min(res.x, res.y);
  float r = length(uv), a = atan(uv.y, uv.x);
  float n = seg, sa = 6.2831853 / n;
  float pulse = 1. + 0.06 * energy + 0.05 * flash;
  vec3 col;
  if (mode < .5 || mode > 2.5) {               // rosace / éclats : repli miroir
    float aa = mod(a + t * 0.05 * (1. + spin), sa); aa = abs(aa - sa * .5);
    vec2 q = vec2(cos(aa), sin(aa)) * r / pulse;
    if (mode > 2.5) q = rot(0.4) * q * 1.3 + vec2(0., .1 * sin(t * .2));
    col = shards(q * 3.1, r * .35);
    col *= smoothstep(1.08, .25, r) * 1.35;
    col += pal(r * 1.5 - t * .05) * .25 * smoothstep(.12, 0., r) * (0.6 + energy);
  } else {                                      // tunnels : profondeur = 1/r
    float z = -log(r + .002) * 1.1 + t * (0.25 + 0.7 * energy) * depth;     // log-polaire : anneaux réguliers vers le fond
    float aa = mod(a + t * .04 * (1. + spin), sa); aa = abs(aa - sa * .5);
    vec2 q = vec2(aa / sa * 2.2, z);
    col = shards(q, z * .02);
    float fog = smoothstep(0., .3, r);
    if (mode > 1.5) {                           // holographique : lignes spectrales, cyan/violet
      float ring = abs(fract(z * .5) - .5);
      col += hue(z * .05 + a * .08 + t * .02) * smoothstep(.04, 0., ring) * (.35 + .8 * energy);
      col *= vec3(.75, .9, 1.15);
    }
    col = col * fog + pal(t * .03) * (1. - fog) * (.6 + 1.2 * energy) * .8;   // lumière au fond du tube
    col += vec3(1., .95, .9) * smoothstep(.05, 0., r) * (.25 + .9 * energy);
  }
  // ondes : une par note jouée, colorée par la hauteur
  for (int i = 0; i < 6; i++){
    vec4 w = waves[i];
    if (w.z <= 0.) continue;
    float rad = w.x * (mode > .5 && mode < 2.5 ? .45 : .7);
    float d = abs(r - rad);
    float ang = .55 + .45 * cos(a * n - w.w);
    col += hue(w.y) * w.z * exp(-d * 28.) * exp(-w.x * 1.6) * (0.6 + .6 * ang);
  }
  col = mix(col, col * vec3(1.08, .96, .82), warm);
  col *= .85 + .5 * energy + .35 * flash;
  col = col / (1. + col * .55);                 // tonemap doux
  col = pow(col, vec3(.92));
  o = vec4(col, 1.);
}`;

  let gl, prog, U = {}, canvas, ctx2d, raf = 0, last = 0, start = performance.now(), running = true, scale = 1;
  const S = { energy: 0, eTarget: 0, flash: 0, seg: 8, segT: 8, seed: .3, dens: .4, spin: 0, warm: .2, depth: 1, mode: 0, modeT: 0,
              pal: [[.9, .4, .1], [.95, .75, .2], [.2, .5, .9], [.7, .2, .6]], palT: null, waves: [], fps: 60, frames: 0, slow: 0, quality: 1 };

  function compile(type, src) {
    const s = gl.createShader(type); gl.shaderSource(s, src); gl.compileShader(s);
    if (!gl.getShaderParameter(s, gl.COMPILE_STATUS)) throw new Error(gl.getShaderInfoLog(s));
    return s;
  }
  function init(cv) {
    canvas = cv;
    try {
      gl = cv.getContext("webgl2", { antialias: false, premultipliedAlpha: false, powerPreference: "high-performance" });
      if (!gl) throw new Error("webgl2");
      prog = gl.createProgram();
      gl.attachShader(prog, compile(gl.VERTEX_SHADER, VS)); gl.attachShader(prog, compile(gl.FRAGMENT_SHADER, FS));
      gl.linkProgram(prog);
      if (!gl.getProgramParameter(prog, gl.LINK_STATUS)) throw new Error(gl.getProgramInfoLog(prog));
      gl.useProgram(prog);
      const b = gl.createBuffer(); gl.bindBuffer(gl.ARRAY_BUFFER, b);
      gl.bufferData(gl.ARRAY_BUFFER, new Float32Array([-1, -1, 3, -1, -1, 3]), gl.STATIC_DRAW);
      const loc = gl.getAttribLocation(prog, "p"); gl.enableVertexAttribArray(loc); gl.vertexAttribPointer(loc, 2, gl.FLOAT, false, 0, 0);
      for (const k of ["res", "t", "energy", "seg", "mode", "seed", "dens", "spin", "warm", "flash", "depth", "quality", "c0", "c1", "c2", "c3", "waves"])
        U[k] = gl.getUniformLocation(prog, k);
    } catch (e) { gl = null; ctx2d = cv.getContext("2d"); console.warn("Kaleidoscope : repli Canvas 2D", e); }
    resize();
    if (!raf) raf = requestAnimationFrame(frame);
  }
  function resize() {
    if (!canvas) return;
    const r = canvas.getBoundingClientRect();
    const dpr = Math.min(window.devicePixelRatio || 1, 2) * scale;
    const w = Math.max(32, Math.round(r.width * dpr)), h = Math.max(32, Math.round(r.height * dpr));
    if (canvas.width !== w || canvas.height !== h) { canvas.width = w; canvas.height = h; }
  }
  const lerp = (a, b, k) => a + (b - a) * k;

  function frame(now) {
    raf = requestAnimationFrame(frame);
    const dt = Math.min(0.05, (now - (last || now)) / 1000); last = now;
    if (!running) return;
    // qualité adaptative : si l'image ralentit, on baisse la résolution (fluidité d'abord)
    S.frames++; S.fps = lerp(S.fps, dt > 0 ? 1 / dt : 60, 0.05);
    if (S.frames % 90 === 0) {
      if (S.fps < 45 && scale > .5) { scale = Math.max(.5, scale - .15); resize(); }
      else if (S.fps > 58 && scale < 1) { scale = Math.min(1, scale + .1); resize(); }
    }
    const k = 1 - Math.exp(-dt * 8);
    S.energy = lerp(S.energy, S.eTarget, S.eTarget > S.energy ? 1 - Math.exp(-dt * 30) : k * .5);
    S.flash *= Math.exp(-dt * 5);
    S.seg = lerp(S.seg, S.segT, 1 - Math.exp(-dt * 2));
    S.mode = lerp(S.mode, S.modeT, 1 - Math.exp(-dt * 3));
    if (S.palT) for (let i = 0; i < 4; i++) for (let j = 0; j < 3; j++) S.pal[i][j] = lerp(S.pal[i][j], S.palT[i][j], 1 - Math.exp(-dt * 2));
    for (const w of S.waves) w.age += dt;
    S.waves = S.waves.filter(w => w.age < 2.5);
    const t = (now - start) / 1000;
    if (gl) draw(t); else draw2d(t);
  }
  function draw(t) {
    gl.viewport(0, 0, canvas.width, canvas.height);
    gl.uniform2f(U.res, canvas.width, canvas.height);
    gl.uniform1f(U.t, t); gl.uniform1f(U.energy, S.energy); gl.uniform1f(U.seg, Math.round(S.seg * 2) / 2);
    gl.uniform1f(U.mode, Math.round(S.mode)); gl.uniform1f(U.seed, S.seed); gl.uniform1f(U.dens, S.dens); gl.uniform1f(U.spin, S.spin);
    gl.uniform1f(U.warm, S.warm); gl.uniform1f(U.flash, S.flash); gl.uniform1f(U.depth, S.depth); gl.uniform1f(U.quality, scale);
    gl.uniform3fv(U.c0, S.pal[0]); gl.uniform3fv(U.c1, S.pal[1]); gl.uniform3fv(U.c2, S.pal[2]); gl.uniform3fv(U.c3, S.pal[3]);
    const w = new Float32Array(24);
    S.waves.slice(-6).forEach((v, i) => { w[i * 4] = v.age; w[i * 4 + 1] = v.hue; w[i * 4 + 2] = v.amp; w[i * 4 + 3] = v.ang; });
    gl.uniform4fv(U.waves, w);
    gl.drawArrays(gl.TRIANGLES, 0, 3);
  }
  function draw2d(t) {                         // repli : rosace dessinée en 2D
    const c = ctx2d, W = canvas.width, H = canvas.height, R = Math.min(W, H) * .5, n = Math.round(S.seg);
    c.fillStyle = "#000"; c.fillRect(0, 0, W, H);
    c.save(); c.translate(W / 2, H / 2);
    for (let i = 0; i < n * 2; i++) {
      c.save(); c.rotate(i * Math.PI / n + t * .05); if (i % 2) c.scale(1, -1);
      for (let j = 0; j < 7; j++) {
        const p = S.pal[(i + j) % 4], a = .35 + .4 * Math.sin(t + j + S.seed * 9);
        c.fillStyle = `rgba(${p[0] * 255 | 0},${p[1] * 255 | 0},${p[2] * 255 | 0},${a * (0.6 + S.energy)})`;
        c.beginPath(); c.moveTo(R * j / 8, 0); c.lineTo(R * (j + 1) / 8, R * .12 * (1 + Math.sin(t * .7 + j))); c.lineTo(R * (j + 1.4) / 8, 0); c.fill();
      }
      c.restore();
    }
    c.restore();
  }

  // ---- interface publique
  const hsl = (h, s, l) => { const k = n => (n + h * 12) % 12, a = s * Math.min(l, 1 - l);
    const f = n => l - a * Math.max(-1, Math.min(k(n) - 3, Math.min(9 - k(n), 1))); return [f(0), f(8), f(4)]; };
  return {
    init, resize,
    setRunning(v) { running = v; },
    setMode(layout) { S.modeT = { retro: 0, telescope: 1, holo: 2, labo: 3 }[layout] ?? 0; S.depth = layout === "holo" ? 1.3 : 1; S.warm = layout === "retro" ? .35 : layout === "telescope" ? .15 : 0; },
    // palette : style (0..n), humeur, tonique — couleurs différentes à chaque univers musical
    setPalette(paletteIdx, mood, root) {
      const base = [[.04, .09, .58, .14], [.62, .78, .92, .52], [.0, .08, .16, .55], [.12, .5, .58, .9], [.8, .9, .02, .45], [.55, .72, .85, .95],
                    [.02, .12, .48, .6], [.33, .45, .58, .1], [.9, .98, .12, .7]][((paletteIdx | 0) % 9 + 9) % 9];
      const dark = { triste: .82, sombre: .7, mysterieuse: .85, planante: 1, joyeuse: 1.12, energique: 1.1, romantique: 1, epique: 1.05, luxueuse: 1, nostalgique: .95 }[mood] ?? 1;
      const sat = { triste: .45, sombre: .55, planante: .6, nostalgique: .55, luxueuse: .7 }[mood] ?? .85;
      const shift = (root || 0) / 12 * .08;
      S.palT = base.map((h, i) => hsl((h + shift) % 1, sat, Math.min(.72, (.42 + .08 * (i % 2)) * dark)));
      if (mood === "luxueuse") S.palT[1] = [.95, .78, .35];
    },
    // traits de la mélodie (Features) : densité, intervalle moyen, sauts, registre, syncope, silences, ambitus, tension, triolets, montée
    setPattern(info) {
      const f = info.feat || [];
      S.seed = parseInt(String(info.seed || "1").slice(-6), 16) / 0xffffff || .3;
      S.segT = 6 + 2 * Math.round(Math.min(3, Math.max(0, (f[7] || .3) * 4)));   // tension -> 6, 8, 10, 12 miroirs
      S.dens = Math.min(1, (f[0] || .4));
      S.spin = (f[4] || 0) * 1.5 + (f[9] - .5 || 0);
      S.flash = 1;
    },
    energy(e) { S.eTarget = Math.min(1, e * 1.6); },
    note(pitch, vel) {
      S.waves.push({ age: 0, hue: ((pitch % 12) / 12 + .03) % 1, amp: .35 + .65 * (vel || .7), ang: (pitch % 12) / 12 * 6.283 });
      S.flash = Math.min(1, S.flash + .25 * (vel || .7));
    },
    stats() { return { fps: S.fps, scale, webgl: !!gl }; },
  };
})();
