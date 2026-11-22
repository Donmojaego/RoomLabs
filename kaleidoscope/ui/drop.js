"use strict";
// =============================================================================
// Kaleidoscope — mode Drop : analyse d'un son déposé (tonalité, mode, tempo), entièrement dans l'interface.
//   chroma (12 classes de hauteur, pondéré), profils de Krumhansl-Schmuckler (24 tonalités),
//   tempo par autocorrélation de l'enveloppe d'attaques (60-180 BPM, préférence 80-160).
// Résultat proposé à l'utilisateur, qui peut corriger la tonique et la gamme avant de générer.
// =============================================================================
const DropAnalysis = (() => {
  const MAJ = [6.35, 2.23, 3.48, 2.33, 4.38, 4.09, 2.52, 5.19, 2.39, 3.66, 2.29, 2.88];
  const MIN = [6.33, 2.68, 3.52, 5.38, 2.60, 3.53, 2.54, 4.75, 3.98, 2.69, 3.34, 3.17];
  function fft(re, im) {
    const n = re.length;
    for (let i = 1, j = 0; i < n; i++) { let bit = n >> 1; for (; j & bit; bit >>= 1) j ^= bit; j ^= bit; if (i < j) { [re[i], re[j]] = [re[j], re[i]]; [im[i], im[j]] = [im[j], im[i]]; } }
    for (let len = 2; len <= n; len <<= 1) {
      const ang = -2 * Math.PI / len, wr = Math.cos(ang), wi = Math.sin(ang);
      for (let i = 0; i < n; i += len) {
        let cr = 1, ci = 0;
        for (let j = 0; j < len / 2; j++) {
          const a = i + j, b = a + len / 2, xr = re[b] * cr - im[b] * ci, xi = re[b] * ci + im[b] * cr;
          re[b] = re[a] - xr; im[b] = im[a] - xi; re[a] += xr; im[a] += xi;
          const t = cr * wr - ci * wi; ci = cr * wi + ci * wr; cr = t;
        }
      }
    }
  }
  const corr = (a, b) => { const ma = a.reduce((s, x) => s + x, 0) / 12, mb = b.reduce((s, x) => s + x, 0) / 12;
    let n = 0, da = 0, db = 0; for (let i = 0; i < 12; i++) { n += (a[i] - ma) * (b[i] - mb); da += (a[i] - ma) ** 2; db += (b[i] - mb) ** 2; } return n / Math.sqrt(da * db + 1e-12); };

  async function analyse(arrayBuffer) {
    const AC = window.AudioContext || window.webkitAudioContext;
    const ac = new AC();
    let buf;
    try { buf = await ac.decodeAudioData(arrayBuffer.slice(0)); } finally { try { ac.close(); } catch (e) {} }
    const sr = buf.sampleRate, len = Math.min(buf.length, Math.floor(sr * 45));        // 45 s suffisent
    const x = new Float32Array(len);
    for (let c = 0; c < buf.numberOfChannels; c++) { const d = buf.getChannelData(c); for (let i = 0; i < len; i++) x[i] += d[i] / buf.numberOfChannels; }
    const N = 8192, hop = 2048, chroma = new Float64Array(12), win = new Float32Array(N);
    for (let i = 0; i < N; i++) win[i] = .5 - .5 * Math.cos(2 * Math.PI * i / N);
    const onset = []; let prev = null;
    const re = new Float64Array(N), im = new Float64Array(N);
    const binPc = new Int8Array(N / 2), binW = new Float32Array(N / 2);
    for (let k = 1; k < N / 2; k++) {
      const hz = k * sr / N;
      if (hz < 55 || hz > 2200) { binPc[k] = -1; continue; }
      const midi = 69 + 12 * Math.log2(hz / 440); const near = Math.round(midi);
      binPc[k] = ((near % 12) + 12) % 12; binW[k] = Math.max(0, 1 - 2 * Math.abs(midi - near)) * (hz < 110 ? .6 : 1);   // poids : proche d'une note juste
    }
    for (let s = 0; s + N <= len; s += hop) {
      for (let i = 0; i < N; i++) { re[i] = x[s + i] * win[i]; im[i] = 0; }
      fft(re, im);
      const mag = new Float32Array(N / 2);
      let flux = 0;
      for (let k = 1; k < N / 2; k++) {
        const m = Math.sqrt(re[k] * re[k] + im[k] * im[k]); mag[k] = m;
        if (binPc[k] >= 0) chroma[binPc[k]] += Math.sqrt(m) * binW[k];
        if (prev && k < 1200) flux += Math.max(0, Math.log1p(m) - Math.log1p(prev[k]));
      }
      onset.push(flux); prev = mag;
    }
    // tonalité : meilleure corrélation sur 24 profils
    let best = { r: -2, root: 0, minor: false };
    for (let root = 0; root < 12; root++) {
      const rot = Array.from({ length: 12 }, (_, i) => chroma[(i + root) % 12]);
      const a = corr(rot, MAJ), b = corr(rot, MIN);
      if (a > best.r) best = { r: a, root, minor: false };
      if (b > best.r) best = { r: b, root, minor: true };
    }
    // tempo : autocorrélation de l'enveloppe d'attaques (une trame = hop / sr secondes)
    let bpm = 0, tconf = 0;
    if (onset.length > 32) {
      const m = onset.reduce((a, b) => a + b, 0) / onset.length, o = onset.map(v => Math.max(0, v - m));
      const fps = sr / hop; let bestS = 0;
      for (let b = 60; b <= 180; b += 0.5) {
        const lag = fps * 60 / b, L = Math.floor(lag);
        if (L < 2 || L * 2 >= o.length) continue;
        const fr = lag - L; let sc = 0;
        for (let i = 0; i + L + 1 < o.length; i++) sc += o[i] * (o[i + L] * (1 - fr) + o[i + L + 1] * fr);
        sc *= Math.exp(-0.5 * Math.pow(Math.log2(b / 118) / 0.9, 2));     // préférence douce autour de 118 BPM
        if (sc > bestS) { bestS = sc; bpm = b; }
      }
      const e = o.reduce((a, v) => a + v * v, 0); tconf = e > 0 ? Math.min(1, bestS / e) : 0;
    }
    const total = chroma.reduce((a, b) => a + b, 0) || 1;
    return { root: best.root, scale: best.minor ? 1 : 0, minor: best.minor, confidence: Math.max(0, best.r), bpm: Math.round(bpm), tempoConfidence: tconf,
             chroma: Array.from(chroma, v => v / total), seconds: len / sr };
  }
  return { analyse };
})();
