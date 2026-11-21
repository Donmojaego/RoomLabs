#!/usr/bin/env python3
"""Mesure de la fluidité de l'animation (aperçu, navigateur) : i/s et temps de rendu par image.
Usage : python3 tests/ui/measure_animation.py build/preview/index.html [chrome]"""
import asyncio, statistics, sys
from playwright.async_api import async_playwright
URL = "file://" + sys.argv[1]; CHROME = sys.argv[2] if len(sys.argv) > 2 else None
FEED = """
async ({kind, mode}) => {
  const T = window.SHK_TEST; T.setPref("visual", mode); T.perf.log.length = 0;
  const sleep = ms => new Promise(r => setTimeout(r, ms)); const dt = 1000 / 60; let h = T.fbk.h, tk = T.fbk.tk, tt = 1, t = 0;
  const send = (e, k) => T.feed({e, h, note: 60 + (h * 7) % 24, vel: .9, k, v: k, t: tt, tk, w: T.fbk.w, b: 0});
  if (kind === "note courte") { h++; for (t = 0; t < 1500; t += dt) { send(t < 400 ? .7 * Math.exp(-t / 150) : 0, t < 150 ? 1 : 0); await sleep(dt); } }
  if (kind === "mélodie + tic-tac") { for (t = 0; t < 3000; t += dt) { if (t % 250 < dt) h++; if (t % 250 < dt) tk++; tt = Math.max(.3, tt - .002); send(.6, 1); await sleep(dt); } }
  if (kind === "silence") { for (t = 0; t < 3000; t += dt) { if (t < 50) send(0, 0); await sleep(dt); } }
  const ms = T.perf.log.slice(); ms.sort((a, b) => a - b);
  return {frames: ms.length, s: t / 1000, mean: ms.reduce((a, b) => a + b, 0) / Math.max(1, ms.length), p95: ms[Math.floor(ms.length * .95)] || 0};
}"""
async def run(b, w, h, kind, mode):
    pg = await b.new_page(viewport={"width": w, "height": h}); await pg.goto(URL); await pg.wait_for_timeout(2500)
    r = await pg.evaluate(FEED, {"kind": kind, "mode": mode}); await pg.close(); return r
async def main():
    out = ["## Animation (navigateur sans carte graphique : ordre de grandeur)\n", "| Taille | Mode | Cas | i/s | Rendu moyen | Rendu 95 % |", "|---|---|---|---|---|---|"]
    async with async_playwright() as p:
        b = await (p.chromium.launch(executable_path=CHROME) if CHROME else p.chromium.launch())
        for (w, h, z) in [(480, 320, "50 %"), (960, 640, "100 %"), (1920, 1280, "200 %")]:
            for mode in ["anime", "reduit", "statique"]:
                for kind in ["note courte", "mélodie + tic-tac", "silence"]:
                    if z != "100 %" and (mode != "anime" or kind == "silence"): continue
                    r = await run(b, w, h, kind, mode)
                    out.append(f"| {z} | {mode} | {kind} | {r['frames'] / r['s']:.0f} | {r['mean']:.2f} ms | {r['p95']:.2f} ms |")
        res = await asyncio.gather(*[run(b, 960, 640, "mélodie + tic-tac", "anime") for _ in range(5)])
        fps = [r["frames"] / r["s"] for r in res]
        out.append(f"\n5 instances ouvertes (100 %, mélodie + tic-tac) : {min(fps):.0f} à {max(fps):.0f} i/s, rendu moyen {statistics.mean(r['mean'] for r in res):.2f} ms.")
        await b.close()
    print("\n".join(out))
asyncio.run(main())
