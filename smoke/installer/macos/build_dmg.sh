#!/bin/bash
# Fabrique Smoke-<version>.dmg contenant l'installeur .pkg
# Usage : installer/macos/build_dmg.sh <version> <dossier Release>
set -euo pipefail
VERSION="$1"; REL="$2"
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
WORK="$(mktemp -d)"
PKGROOT="$WORK/root"
CONTENT="$PKGROOT/Library/Application Support/Room Labs/Smoke"
mkdir -p "$PKGROOT/Library/Audio/Plug-Ins/VST3" "$PKGROOT/Library/Audio/Plug-Ins/Components" "$PKGROOT/Applications" \
         "$CONTENT/Presets/Usine" "$CONTENT/Samples" "$CONTENT/Documentation" "$CONTENT/Licenses"
cp -R "$REL/VST3/Smoke.vst3" "$PKGROOT/Library/Audio/Plug-Ins/VST3/"
cp -R "$REL/AU/Smoke.component" "$PKGROOT/Library/Audio/Plug-Ins/Components/"
cp -R "$REL/Standalone/Smoke.app" "$PKGROOT/Applications/"
cp "$ROOT/presets/factory.json" "$CONTENT/Presets/Usine/"
cp "$ROOT/docs/LISEZ-MOI.txt" "$CONTENT/Documentation/"
cp "$ROOT/docs/LICENCES.txt" "$CONTENT/Licenses/"

# signature « ad hoc » (obligatoire sur Apple Silicon ; pas de compte Apple Developer)
for b in "$PKGROOT/Library/Audio/Plug-Ins/VST3/Smoke.vst3" "$PKGROOT/Library/Audio/Plug-Ins/Components/Smoke.component" "$PKGROOT/Applications/Smoke.app"; do
  codesign --force --deep --sign - "$b"
done

mkdir -p "$WORK/dmg"
pkgbuild --root "$PKGROOT" --identifier com.roomlabs.smoke --version "$VERSION" \
         --install-location / --scripts "$ROOT/installer/macos/scripts" "$WORK/Smoke-core.pkg"
cat > "$WORK/distribution.xml" <<XML
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2">
  <title>Smoke ${VERSION} — Room Labs</title>
  <options customize="never" require-scripts="false" hostArchitectures="arm64,x86_64"/>
  <domains enable_localSystem="true"/>
  <choices-outline><line choice="core"/></choices-outline>
  <choice id="core" title="Smoke"><pkg-ref id="com.roomlabs.smoke"/></choice>
  <pkg-ref id="com.roomlabs.smoke" version="${VERSION}">Smoke-core.pkg</pkg-ref>
</installer-gui-script>
XML
productbuild --distribution "$WORK/distribution.xml" --package-path "$WORK" "$WORK/dmg/Installer Smoke.pkg"

cat > "$WORK/dmg/LISEZ-MOI.txt" <<TXT
SMOKE ${VERSION} — by Room Labs © 2026

1. Clic droit (ou Ctrl + clic) sur « Installer Smoke.pkg » > Ouvrir > Ouvrir.
   (Le Mac affiche un avertissement car l'installeur n'est pas encore signé par Apple.)
   Si le bouton « Ouvrir » n'apparaît pas : Réglages Système > Confidentialité et sécurité
   > tout en bas « Ouvrir quand même ».
2. Suivre l'installation (mot de passe du Mac demandé).
3. Dans Logic Pro : le plugin est dans Instruments AU > Room Labs > Smoke.
   Dans Ableton Live / FL Studio Mac : VST3 > Smoke (relancer la recherche des plugins).

Fichiers installés :
- /Library/Audio/Plug-Ins/Components/Smoke.component (AU)
- /Library/Audio/Plug-Ins/VST3/Smoke.vst3
- /Applications/Smoke.app (version autonome)
- /Library/Application Support/Room Labs/Smoke (presets, documentation)
TXT

OUT="$ROOT/Smoke-macOS-${VERSION}.dmg"
rm -f "$OUT"
hdiutil create -volname "Smoke ${VERSION}" -srcfolder "$WORK/dmg" -ov -format UDZO "$OUT"
echo "DMG : $OUT"
