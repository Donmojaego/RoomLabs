#!/usr/bin/env python3
"""Génère l'installeur Windows de la suite à partir de manifest.json (aucune réécriture à la main
pour ajouter un plugin : on l'ajoute au manifeste et à CMakeLists.txt).

  python tools/make_installer.py <version> <dossier_build_cmake> <sortie.iss>

Le plugin VST3 de chaque entrée est cherché dans <build>/<target>_artefacts/Release/VST3/<name>.vst3.
Les données utilisateur (%APPDATA%\\Room Labs\\Suite : thème, taille, préférences) ne sont jamais touchées,
ni à l'installation, ni à la mise à jour, ni à la désinstallation.
"""
import json, os, sys

ver, build, out = sys.argv[1], sys.argv[2], sys.argv[3]
here = os.path.dirname(os.path.abspath(__file__))
m = json.load(open(os.path.join(here, "..", "manifest.json"), encoding="utf-8"))
build = os.path.abspath(build)

L = []
w = L.append
w("; Fichier généré par tools/make_installer.py depuis manifest.json — ne pas modifier à la main")
w("[Setup]")
w(f"AppId={m['appId'].replace('{', '{{')}")
w(f"AppName={m['suite']} Plugins")
w(f"AppVersion={ver}")
w(f"AppVerName={m['suite']} Plugins {ver}")
w(f"AppPublisher={m['publisher']}")
w(f"AppPublisherURL={m['url']}")
w(f"DefaultDirName={{commonpf64}}\\Room Labs Audio")
w("DisableProgramGroupPage=yes")
w("DisableDirPage=yes")
w("ArchitecturesAllowed=x64compatible")
w("ArchitecturesInstallIn64BitMode=x64compatible")
w("PrivilegesRequired=admin")
w("OutputDir=output")
w(f"OutputBaseFilename=RoomLabsAudio_Plugins_{ver}_Win_x64")
w("Compression=lzma2/max")
w("SolidCompression=yes")
w("WizardStyle=modern")
w("UninstallDisplayName=Room Labs Audio Plugins")
w("")
w("[Languages]")
w('Name: "fr"; MessagesFile: "compiler:Languages\\French.isl"')
w('Name: "en"; MessagesFile: "compiler:Default.isl"')
w("")
w("[Types]")
w('Name: "full"; Description: "Tous les plugins"')
w('Name: "custom"; Description: "Choisir les plugins"; Flags: iscustom')
w("")
w("[Components]")
for p in m["plugins"]:
    w(f'Name: "{p["id"]}"; Description: "{p["name"]} — {p["description"]}"; Types: full custom')
w("")
w("[Files]")
for p in m["plugins"]:
    src = os.path.join(build, f"{p['target']}_artefacts", "Release", "VST3", f"{p['name']}.vst3")
    if not os.path.isdir(src):
        sys.exit(f"VST3 introuvable pour {p['name']} : {src}")
    w(f'Source: "{src}\\*"; DestDir: "{{commoncf64}}\\VST3\\Room Labs Audio\\{p["name"]}.vst3"; Components: {p["id"]}; Flags: ignoreversion recursesubdirs createallsubdirs')
w(f'Source: "{os.path.join(here, "..", "docs", "LISEZ-MOI.txt")}"; DestDir: "{{app}}"; Flags: ignoreversion isreadme')
w(f'Source: "{os.path.abspath(os.path.join(here, "..", "RAPPORT-TESTS.md"))}"; DestDir: "{{app}}"; Flags: ignoreversion skipifsourcedoesntexist')
w("")
w("[InstallDelete]")
w("; un plugin décoché lors d'une mise à jour est retiré proprement (jamais les préférences)")
for p in m["plugins"]:
    w(f'Type: filesandordirs; Name: "{{commoncf64}}\\VST3\\Room Labs Audio\\{p["name"]}.vst3"; Components: not {p["id"]}')
w("")
w("[UninstallDelete]")
w('Type: dirifempty; Name: "{commoncf64}\\VST3\\Room Labs Audio"')
open(out, "w", encoding="utf-8-sig").write("\n".join(L) + "\n")
print(f"{out} : {len(m['plugins'])} plugin(s)")
