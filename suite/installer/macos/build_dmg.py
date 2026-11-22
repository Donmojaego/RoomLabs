#!/usr/bin/env python3
"""DMG macOS de la suite, piloté par manifest.json : RoomLabsAudio_Plugins_<version>_macOS.dmg
   (installeur .pkg : VST3 + Audio Unit de chaque plugin, signature ad hoc).
   python3 installer/macos/build_dmg.py <version> <dossier build>"""
import json, os, shutil, subprocess, sys, tempfile
ver, build = sys.argv[1], os.path.abspath(sys.argv[2])
root = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
m = json.load(open(os.path.join(root, "manifest.json"), encoding="utf-8"))
work = tempfile.mkdtemp(); pk = os.path.join(work, "root")
vst = os.path.join(pk, "Library/Audio/Plug-Ins/VST3/Room Labs Audio"); au = os.path.join(pk, "Library/Audio/Plug-Ins/Components")
doc = os.path.join(pk, "Library/Application Support/Room Labs/Suite/Documentation")
for d in (vst, au, doc): os.makedirs(d, exist_ok=True)
run = lambda *a: subprocess.run(a, check=True)
for p in m["plugins"]:
    rel = os.path.join(build, f"{p['target']}_artefacts", "Release")
    for src, dst in ((os.path.join(rel, "VST3", p["name"] + ".vst3"), vst), (os.path.join(rel, "AU", p["name"] + ".component"), au)):
        out = os.path.join(dst, os.path.basename(src)); shutil.copytree(src, out, symlinks=True)
        run("codesign", "--force", "--deep", "--sign", "-", out)
shutil.copy(os.path.join(root, "docs", "LISEZ-MOI.txt"), doc)
os.makedirs(os.path.join(work, "dmg"))
run("pkgbuild", "--root", pk, "--identifier", "com.roomlabs.suite", "--version", ver, "--install-location", "/",
    "--scripts", os.path.join(root, "installer/macos/scripts"), os.path.join(work, "suite-core.pkg"))
open(os.path.join(work, "distribution.xml"), "w").write(f"""<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2">
  <title>Room Labs Audio Plugins {ver}</title>
  <options customize="never" require-scripts="false" hostArchitectures="arm64,x86_64"/>
  <domains enable_localSystem="true"/>
  <choices-outline><line choice="core"/></choices-outline>
  <choice id="core" title="Room Labs Audio Plugins"><pkg-ref id="com.roomlabs.suite"/></choice>
  <pkg-ref id="com.roomlabs.suite" version="{ver}">suite-core.pkg</pkg-ref>
</installer-gui-script>
""")
run("productbuild", "--distribution", os.path.join(work, "distribution.xml"), "--package-path", work,
    os.path.join(work, "dmg", "Installer Room Labs Audio.pkg"))
names = "\n".join(f"- {p['name']} : {p['description']}" for p in m["plugins"])
open(os.path.join(work, "dmg", "LISEZ-MOI.txt"), "w").write(f"""ROOM LABS AUDIO — PLUGINS {ver} (macOS 11+, Apple Silicon et Intel)

1. Clic droit (ou Ctrl + clic) sur « Installer Room Labs Audio.pkg » > Ouvrir > Ouvrir.
   (Avertissement normal : l'installeur n'est pas encore signé par Apple.)
   Sinon : Réglages Système > Confidentialité et sécurité > « Ouvrir quand même ».
2. Suivre l'installation (mot de passe du Mac demandé).
3. Logic Pro : Effets Audio Units > Room Labs Audio. Ableton / FL Studio : VST3 > Room Labs Audio.

Plugins :
{names}

Vos préférences (thème, taille) sont conservées lors des mises à jour.
""")
out = os.path.join(root, f"RoomLabsAudio_Plugins_{ver}_macOS.dmg")
if os.path.exists(out): os.remove(out)
run("hdiutil", "create", "-volname", f"Room Labs Audio {ver}", "-srcfolder", os.path.join(work, "dmg"), "-ov", "-format", "UDZO", out)
print("DMG :", out)
