#!/usr/bin/env python3
"""Kaleidoscope — fabrique les 100 fonds à partir des dépôts CC0 (VCSL, VSCO-2-CE).

Usage : python tools/build_banks.py <dossier_des_depots> <sortie> [--only id1,id2] [--list]
  <dossier_des_depots> contient vcsl/ et vsco-2-ce/ (clones, même partiels)
  <sortie>/Banques/<id>/instrument.json + <id>_<note>[_v].flac

Pour chaque instrument :
  - note lue dans le nom du fichier, octave vérifiée par détection de hauteur (les conventions varient) ;
  - une seule prise (rr1), 1 ou 2 couches de vélocité (la plus douce et la plus forte) ;
  - au plus une note par tierce mineure (zones de clavier calculées au milieu des racines) ;
  - instruments tenus : boucle fondue enchaînée cuite dans le fichier (pas de clic), longueur ≤ 4,5 s ;
  - instruments frappés : silence final coupé, fondu de 30 ms, longueur ≤ 4,5 s (pianos 6 s) ;
  - mono si les deux canaux sont quasi identiques ; volume harmonisé (crête médiane à -6 dBFS).
Dépendances : numpy, soundfile.
"""
import json, os, re, sys
import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import banks  # noqa: E402

NOTE_RE = re.compile(r"(?:^|[_\s-])([A-Ga-g])(#|b|s)?(-?\d)(?=$|[_\s.-])")
VEL_RE = re.compile(r"(?:^|_)(?:vl?|v)(\d)(?:_|$)|(?:^|_)(ppp|pp|p|mp|mf|f|ff|fff|soft|loud|medium)(?:\d?)(?:_|$)", re.I)
RR_RE = re.compile(r"(?:^|_)(?:rr|RR|r)0*(\d+)(?:_|$)")
PC = {"C": 0, "D": 2, "E": 4, "F": 5, "G": 7, "A": 9, "B": 11}
DYN = {"ppp": 1, "pp": 2, "p": 3, "soft": 3, "mp": 4, "medium": 5, "mf": 5, "f": 6, "loud": 7, "ff": 7, "fff": 8}


def parse(fname):
    stem = os.path.splitext(os.path.basename(fname))[0]
    m = None
    for m in NOTE_RE.finditer(stem):
        pass                                   # dernière occurrence (les préfixes comme "B1_f_1" restent bons)
    if not m: return None
    acc = {"#": 1, "s": 1, "b": -1}.get(m.group(2) or "", 0)
    note = 12 * (int(m.group(3)) + 1) + PC[m.group(1).upper()] + acc     # convention C4 = 60 ; corrigée plus tard
    rest = stem[:m.start()] + "_" + stem[m.end():]
    vel = 0
    for v in VEL_RE.finditer(rest):
        vel = int(v.group(1)) if v.group(1) else DYN.get(v.group(2).lower(), 0)
    rr = RR_RE.search(rest)
    rr = int(rr.group(1)) if rr else (int(re.search(r"_(\d+)$", stem).group(1)) if re.search(r"_(\d+)$", stem) and not re.search(r"_v\d$", stem) else 1)
    return dict(note=note, vel=vel, rr=rr)


def list_files(repo_dir, folder):
    d = os.path.join(repo_dir, folder)
    return sorted(os.path.join(d, f) for f in os.listdir(d) if f.lower().endswith((".wav", ".flac"))) if os.path.isdir(d) else []


def choose(files, layers):
    """une prise par note, couches de vélocité extrêmes, au plus une note par tierce mineure"""
    by = {}
    for f in files:
        p = parse(f)
        if p: by.setdefault(p["note"], []).append((p, f))
    notes = {}
    for n, lst in by.items():
        minrr = min(p["rr"] for p, _ in lst)
        lst = [(p, f) for p, f in lst if p["rr"] == minrr]
        vels = sorted({p["vel"] for p, _ in lst})
        keep = [vels[-1]] if layers == 1 else ([vels[0], vels[-1]] if len(vels) > 1 else vels)
        if layers == 1 and len(vels) > 2: keep = [vels[-2]]       # avant-dernière couche : timbre plein sans dureté
        notes[n] = [(v, next(f for p, f in lst if p["vel"] == v)) for v in keep]
    roots = sorted(notes)
    kept, last = [], -99
    for i, n in enumerate(roots):
        if len(roots) <= 26 or n - last >= 3 or i == len(roots) - 1:
            kept.append(n); last = n
    return [(n, notes[n]) for n in kept]


def f0(x, sr):
    """hauteur par autocorrélation normalisée sur 0,2 s après l'attaque (en Hz, 0 si indéterminé)"""
    m = x.mean(axis=1) if x.ndim > 1 else x
    a = int(np.argmax(np.abs(m) > 0.1 * np.max(np.abs(m)) + 1e-9)) + int(0.08 * sr)
    seg = m[a:a + int(0.2 * sr)]
    if len(seg) < 2048: return 0.0
    seg = seg - seg.mean()
    n = len(seg); spec = np.fft.rfft(seg, 2 * n); ac = np.fft.irfft(spec * np.conj(spec))[:n]
    ac /= ac[0] + 1e-12
    lo, hi = int(sr / 2500), int(sr / 25)
    k = lo + int(np.argmax(ac[lo:hi]))
    return sr / k if ac[k] > 0.4 else 0.0


def octave_shift(samples):
    """décalage d'octave (en notes MIDI) qui accorde le mieux les noms de fichiers avec la hauteur mesurée"""
    votes = []
    for n, x, sr in samples:
        hz = f0(x, sr)
        if hz <= 0: continue
        midi = 69 + 12 * np.log2(hz / 440.0)
        d = midi - n
        votes.append(int(round(d / 12.0)) * 12)
    if not votes: return 12
    vals, counts = np.unique(votes, return_counts=True)
    best, share = int(vals[np.argmax(counts)]), counts.max() / len(votes)
    # ces bibliothèques nomment surtout les notes avec do3 = 60 (+12) ; une détection isolée ou extrême
    # (cloches inharmoniques, pizzicati trop courts) retombe sur cette convention
    return best if best in (0, 12, 24) and share >= 0.6 else 12


# instruments dont les noms de fichiers ne suivent pas la convention (vérifié à l'oreille / par la détection)
OCTAVE_OVERRIDE = {"glock-tubulaire": 48}


def bake_loop(x, sr, max_len):
    """boucle sur la partie stable : fin à 85 % (≤ max_len), départ ~1,2 s avant, calé sur l'autocorrélation, fondu de 120 ms"""
    n = min(len(x), int(max_len * sr))
    end = int(n * 0.92)
    start_guess = max(int(0.35 * n), end - int(1.2 * sr))
    xf = int(0.12 * sr)
    if start_guess < xf + 1 or end - start_guess < 2 * xf: return x[:n], None
    m = x.mean(axis=1) if x.ndim > 1 else x
    ref = m[end - 2048:end]
    best, bs = -1e9, start_guess
    for s in range(start_guess - 2000, start_guess + 2000, 1):
        if s - 2048 < xf: continue
        c = float(np.dot(ref, m[s - 2048:s]))
        if c > best: best, bs = c, s
    start = bs
    y = x[:end].copy()
    w = np.linspace(0, 1, xf) if x.ndim == 1 else np.linspace(0, 1, xf)[:, None]
    # les dernières xf frames avant la fin glissent vers celles avant le départ : la jonction est continue
    y[end - xf:end] = y[end - xf:end] * (1 - w) + x[start - xf:start] * w
    return y, (start, end)


def trim(x, sr, max_len):
    m = np.abs(x).max(axis=1) if x.ndim > 1 else np.abs(x)
    pk = m.max() + 1e-12
    above = np.where(m > pk * 10 ** (-60 / 20))[0]
    n = min(len(x), int(max_len * sr), (above[-1] + int(0.05 * sr)) if len(above) else len(x))
    y = x[:n].copy()
    f = min(n, int(0.03 * sr))
    w = np.linspace(1, 0, f) if y.ndim == 1 else np.linspace(1, 0, f)[:, None]
    y[n - f:] *= w
    return y


def build(entry, repos, out):
    import soundfile as sf
    files = list_files(os.path.join(repos, entry["repo"]), entry["folder"])
    picks = choose(files, entry["layers"])
    if len(picks) < 3: raise RuntimeError(f"{entry['id']} : trop peu de notes ({len(picks)})")
    loaded = []
    for n, lst in picks:
        for v, f in lst:
            x, sr = sf.read(f, dtype="float32", always_2d=True)
            if x.shape[1] > 2: x = x[:, :2]
            loaded.append((n, v, x, sr, f))
    shift = OCTAVE_OVERRIDE.get(entry["id"], octave_shift([(n, x, sr) for n, v, x, sr, f in loaded]))
    peaks = [float(np.abs(x).max()) for *_, x, sr, f in [(a, b, c, d, e) for a, b, c, d, e in loaded]]
    gain = 0.5 / (float(np.median(peaks)) + 1e-9)
    d = os.path.join(out, "Banques", entry["id"]); os.makedirs(d, exist_ok=True)
    roots = sorted({n + shift for n, *_ in loaded})
    regions = []
    vel_of = {}
    for n, v, x, sr, f in loaded:
        vel_of.setdefault(n, []).append(v)
    for n, v, x, sr, f in loaded:
        root = n + shift
        if x.shape[1] == 2 and np.corrcoef(x[:, 0], x[:, 1])[0, 1] > 0.985: x = x.mean(axis=1, keepdims=True)
        x = x * gain
        loop = None
        if entry["sustain"]:
            y, loop = bake_loop(x[:, 0] if x.shape[1] == 1 else x, sr, 4.5)
            x = y.reshape(len(y), -1)
        else:
            x = trim(x, sr, 6.0 if entry["cat"] == "Pianos" else 4.5)
        x = np.clip(x, -1, 1)
        i = roots.index(root)
        lo = 0 if i == 0 else (roots[i - 1] + root) // 2 + 1
        hi = 127 if i == len(roots) - 1 else (root + roots[i + 1]) // 2
        vs = sorted(set(vel_of[n]))
        vlo, vhi = (0, 127) if len(vs) == 1 else ((0, 79) if v == vs[0] else (80, 127))
        name = f"{entry['id']}_{root}" + ("" if len(vs) == 1 else f"_v{1 if v == vs[0] else 2}") + ".flac"
        sf.write(os.path.join(d, name), x.squeeze() if x.shape[1] == 1 else x, sr, subtype="PCM_16")
        r = dict(file=name, root=root, lo=lo, hi=hi, vlo=vlo, vhi=vhi)
        if loop: r.update(loop=True, loopStart=int(loop[0]), loopEnd=int(loop[1]))
        regions.append(r)
    meta = {k: entry[k] for k in ("id", "name", "cat", "source", "license", "sustain", "style", "mood", "version")}
    meta.update(regions=regions, octaveShift=shift, origin=f"{banks.REPOS[entry['repo']]} / {entry['folder']}")
    json.dump(meta, open(os.path.join(d, "instrument.json"), "w", encoding="utf-8"), ensure_ascii=False, indent=1)
    return len(regions), shift


def main():
    if len(sys.argv) < 3: print(__doc__); return 1
    repos, out = sys.argv[1], sys.argv[2]
    only = None
    if "--only" in sys.argv: only = set(sys.argv[sys.argv.index("--only") + 1].split(","))
    rows = [e for e in banks.entries() if not only or e["id"] in only]
    if "--list" in sys.argv:
        for e in rows:
            picks = choose(list_files(os.path.join(repos, e["repo"]), e["folder"]), e["layers"])
            print(f"{e['id']:26s} {len(picks):3d} notes  {[n for n, _ in picks][:6]}…")
        return 0
    bad = []
    for e in rows:
        try:
            n, s = build(e, repos, out)
            print(f"{e['id']:26s} {n:3d} régions  octave {s:+d}")
        except Exception as ex:   # un instrument raté ne bloque pas les autres, mais fait échouer la tâche
            print("ÉCHEC", e["id"], ex); bad.append(e["id"])
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
