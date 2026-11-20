#!/bin/bash
# Fabrique ShakeIt-<version>.dmg contenant l'installeur .pkg
# Usage : installer/macos/build_dmg.sh <version> <dossier Release>
set -euo pipefail
VERSION="$1"; REL="$2"
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
WORK="$(mktemp -d)"
PKGROOT="$WORK/root"
CONTENT="$PKGROOT/Library/Application Support/Room Labs/Shake It"
mkdir -p "$PKGROOT/Library/Audio/Plug-Ins/VST3" "$PKGROOT/Library/Audio/Plug-Ins/Components" "$PKGROOT/Applications" \
         "$CONTENT/Presets/Usine" "$CONTENT/Samples" "$CONTENT/Documentation" "$CONTENT/Licenses"
cp -R "$REL/VST3/ShakeIt.vst3" "$PKGROOT/Library/Audio/Plug-Ins/VST3/"
cp -R "$REL/AU/ShakeIt.component" "$PKGROOT/Library/Audio/Plug-Ins/Components/"
cp -R "$REL/Standalone/ShakeIt.app" "$PKGROOT/Applications/"
cp "$ROOT/presets/factory.json" "$CONTENT/Presets/Usine/"
cp "$ROOT/docs/LISEZ-MOI.txt" "$CONTENT/Documentation/"
cp "$ROOT/docs/LICENCES.txt" "$CONTENT/Licenses/"

# signature « ad hoc » (obligatoire sur Apple Silicon ; pas de compte Apple Developer)
for b in "$PKGROOT/Library/Audio/Plug-Ins/VST3/ShakeIt.vst3" "$PKGROOT/Library/Audio/Plug-Ins/Components/ShakeIt.component" "$PKGROOT/Applications/ShakeIt.app"; do
  codesign --force --deep --sign - "$b"
done

mkdir -p "$WORK/dmg"
pkgbuild --root "$PKGROOT" --identifier com.roomlabs.shakeit --version "$VERSION" \
         --install-location / --scripts "$ROOT/installer/macos/scripts" "$WORK/ShakeIt-core.pkg"
cat > "$WORK/distribution.xml" <<XML
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2">
  <title>Shake It ${VERSION} — Room Labs</title>
  <options customize="never" require-scripts="false" hostArchitectures="arm64,x86_64"/>
  <domains enable_localSystem="true"/>
  <choices-outline><line choice="core"/></choices-outline>
  <choice id="core" title="Shake It"><pkg-ref id="com.roomlabs.shakeit"/></choice>
  <pkg-ref id="com.roomlabs.shakeit" version="${VERSION}">ShakeIt-core.pkg</pkg-ref>
</installer-gui-script>
XML
productbuild --distribution "$WORK/distribution.xml" --package-path "$WORK" "$WORK/dmg/Installer Shake It.pkg"

cat > "$WORK/dmg/LISEZ-MOI.txt" <<TXT
SHAKE IT ${VERSION} — by Room Labs © 2026

1. Clic droit (ou Ctrl + clic) sur « Installer Shake It.pkg » > Ouvrir > Ouvrir.
   (Le Mac affiche un avertissement car l'installeur n'est pas encore signé par Apple.)
   Si le bouton « Ouvrir » n'apparaît pas : Réglages Système > Confidentialité et sécurité
   > tout en bas « Ouvrir quand même ».
2. Suivre l'installation (mot de passe du Mac demandé).
3. Dans Logic Pro : le plugin est dans Instruments AU > Room Labs > Shake It.
   Dans Ableton Live / FL Studio Mac : VST3 > Shake It (relancer la recherche des plugins).

Fichiers installés :
- /Library/Audio/Plug-Ins/Components/ShakeIt.component (AU)
- /Library/Audio/Plug-Ins/VST3/ShakeIt.vst3
- /Applications/ShakeIt.app (version autonome)
- /Library/Application Support/Room Labs/Shake It (presets, documentation)
TXT

OUT="$ROOT/ShakeIt-macOS-${VERSION}.dmg"
rm -f "$OUT"
hdiutil create -volname "Shake It ${VERSION}" -srcfolder "$WORK/dmg" -ov -format UDZO "$OUT"
echo "DMG : $OUT"
