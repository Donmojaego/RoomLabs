#!/usr/bin/env python3
"""Mesure de l'animation dans un navigateur (aperçu, sans plugin).

Simule les messages « motion » du moteur à 60 Hz pour quatre cas (note courte,
808 longue, glide, silence), à 50/100/200 % et avec 5 instances ouvertes.
Usage : python3 tests/ui/measure_animation.py chemin/vers/preview/index.html [chrome]
"""
import asyncio, json, math, sys, statistics
from playwright.async_api import async_playwright

URL = "file://" + sys.argv[1]
CHROME = sys.argv[2] if len(sys.argv) > 2 else None

FEED = """
async ({kind, mode}) => {
  const T = window.SHK_TEST; T.setPref("visual", mode); window.__poseLog = []; T.perf.log.length = 0;
  const sleep = ms => new Promise(r => setTimeout(r, ms));
  let o = T.motion.o, t = 0; const dt = 1000/60;
  const send = (e, g, n) => T.feed({e, o, g, n, l: Math.min(1, e)});
  const frames0 = T.perf.log.length;
  if (kind === "courte") { o++; for (t = 0; t < 600; t += dt) { const e = t < 10 ? t/10*.9 : .9*Math.exp(-(t-10)/120); send(e, 0, t < 150 ? 29 : -1); await sleep(dt); } }
  if (kind === "808 longue") { o++; for (t = 0; t < 3000; t += dt) { const e = .95*Math.exp(-t/1100); send(e, 0, 29); await sleep(dt); } }
  if (kind === "glide") { o++; for (t = 0; t < 2000; t += dt) { const ph = t % 1000; const g = ph < 400 ? 1 - ph/400 : 0; send(.75, g, 29 + (t >= 1000 ? 12 : 0)); await sleep(dt); } }
  if (kind === "silence") { for (t = 0; t < 4500; t += dt) { if (t < 50) send(0, 0, -1); await sleep(dt); } }
  const ms = T.perf.log.slice(frames0); const poses = [...new Set(window.__poseLog)];
  return {frames: ms.length, seconds: t/1000, mean: ms.reduce((a,b)=>a+b,0)/Math.max(1,ms.length), p95: ms.sort((a,b)=>a-b)[Math.floor(ms.length*.95)] || 0, poses, changes: window.__poseLog.length};
}
"""

async def run_page(browser, w, h, kind, mode):
    pg = await browser.new_page(viewport={"width": w, "height": h})
    await pg.goto(URL); await pg.wait_for_timeout(600)
    r = await pg.evaluate(FEED, {"kind": kind, "mode": mode})
    await pg.close(); return r

async def main():
    out = ["## Animation (navigateur sans carte graphique, mesures de l'aperçu)\n",
           "| Taille | Mode | Cas | i/s | Rendu moyen | Rendu 95 % | Poses vues | Changements |", "|---|---|---|---|---|---|---|---|"]
    async with async_playwright() as p:
        b = await p.chromium.launch(executable_path=CHROME) if CHROME else await p.chromium.launch()
        for (w, h, z) in [(500, 320, "50 %"), (1000, 640, "100 %"), (2000, 1280, "200 %")]:
            for mode in ["anime", "reduit", "statique"]:
                for kind in ["courte", "808 longue", "glide", "silence"]:
                    if mode != "anime" and z != "100 %" and kind != "808 longue": continue
                    r = await run_page(b, w, h, kind, mode)
                    names = {"0": "neutre", "1": "impact", "2": "rebond"}
                    fps = r["frames"] / max(.001, r["seconds"])
                    out.append(f"| {z} | {mode} | {kind} | {fps:.0f} | {r['mean']:.2f} ms | {r['p95']:.2f} ms | {', '.join(names[str(x)] for x in r['poses']) or '—'} | {r['changes']} |")
        # 5 instances en même temps
        pages = []
        tasks = [run_page(b, 1000, 640, "808 longue", "anime") for _ in range(5)]
        res = await asyncio.gather(*tasks)
        fps = [r["frames"] / r["seconds"] for r in res]
        out.append(f"\n5 instances ouvertes en même temps (100 %, 808 longue) : {min(fps):.0f} à {max(fps):.0f} i/s, rendu moyen {statistics.mean(r['mean'] for r in res):.2f} ms.")
        await b.close()
    print("\n".join(out))

asyncio.run(main())
