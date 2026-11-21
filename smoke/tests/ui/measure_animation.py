#!/usr/bin/env python3
"""Mesure de la fluidité de l'animation (aperçu, navigateur) : i/s et temps de rendu par image.
Usage : python3 tests/ui/measure_animation.py build/preview/index.html [chrome]"""
import asyncio, statistics, sys
from playwright.async_api import async_playwright
URL = "file://" + sys.argv[1]; CHROME = sys.argv[2] if len(sys.argv) > 2 else None
FEED = """
async ({kind, mode, theme}) => {
  const T = window.SHK_TEST; T.setPref("visual", mode); T.setPref("theme", theme); T.perf.log.length = 0;
  const sleep = ms => new Promise(r => setTimeout(r, ms)); const dt = 1000 / 60; let h = T.fbk.h, t = 0;
  const send = (e, w, k) => T.feed({e, i: w, k: k === undefined ? (e > .01 ? 1 : 0) : k, h, v: e > .01 ? 1 : 0, l: e, vel: .9});
  if (kind === "note courte") { h++; for (t = 0; t < 800; t += dt) { send(t < 300 ? .8 * Math.exp(-t / 120) : 0, t < 500 ? .4 * Math.exp(-t / 200) : 0, t < 150 ? 1 : 0); await sleep(dt); } }
  if (kind === "tenue + effet") { h++; for (t = 0; t < 3000; t += dt) { if (t % 500 < dt) h++; send(.7, .6); await sleep(dt); } }
  if (kind === "silence") { for (t = 0; t < 3000; t += dt) { if (t < 50) send(0, 0, 0); await sleep(dt); } }
  if (kind === "accords + relâche") { for (t = 0; t < 4000; t += dt) { if (t % 800 < dt) h++; const on = t % 800 < 500; send(on ? .75 : .3 * Math.exp(-(t % 800 - 500) / 300), .8, on ? 3 : 0); await sleep(dt); } }
  const ms = T.perf.log.slice(); ms.sort((a, b) => a - b);
  return {frames: ms.length, s: t / 1000, mean: ms.reduce((a, b) => a + b, 0) / Math.max(1, ms.length), p95: ms[Math.floor(ms.length * .95)] || 0};
}"""
async def run(b, w, h, kind, mode, theme):
    pg = await b.new_page(viewport={"width": w, "height": h}); await pg.goto(URL); await pg.wait_for_timeout(700)
    await pg.evaluate("SHK_TEST.perf.log.length = 0"); await pg.wait_for_timeout(1300)   # fin de l'animation d'ouverture
    r = await pg.evaluate(FEED, {"kind": kind, "mode": mode, "theme": theme}); await pg.close(); return r
async def main():
    out = ["## Animation (navigateur sans carte graphique : ordre de grandeur)\n", "| Taille | Thème | Mode | Cas | i/s | Rendu moyen | Rendu 95 % |", "|---|---|---|---|---|---|---|"]
    async with async_playwright() as p:
        b = await (p.chromium.launch(executable_path=CHROME) if CHROME else p.chromium.launch())
        for (w, h, z) in [(500, 320, "50 %"), (1000, 640, "100 %"), (2000, 1280, "200 %")]:
            for theme in ["prune", "bleu"]:
                for mode in ["anime", "reduit", "statique"]:
                    for kind in ["note courte", "tenue + effet", "accords + relâche", "silence"]:
                        if (theme == "bleu" or z != "100 %") and (mode != "anime" or kind == "silence"): continue
                        r = await run(b, w, h, kind, mode, theme)
                        out.append(f"| {z} | {theme} | {mode} | {kind} | {r['frames'] / r['s']:.0f} | {r['mean']:.2f} ms | {r['p95']:.2f} ms |")
        res = await asyncio.gather(*[run(b, 1000, 640, "tenue + effet", "anime", "prune") for _ in range(5)])
        fps = [r["frames"] / r["s"] for r in res]
        out.append(f"\n5 instances ouvertes (100 %, tenue + effet) : {min(fps):.0f} à {max(fps):.0f} i/s, rendu moyen {statistics.mean(r['mean'] for r in res):.2f} ms.")
        await b.close()
    print("\n".join(out))
asyncio.run(main())
