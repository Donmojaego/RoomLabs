#!/bin/bash
# Fabrique AfricanRiddim-<version>.dmg contenant l'installeur .pkg
# Usage : installer/macos/build_dmg.sh <version> <dossier Release>
set -euo pipefail
VERSION="$1"; REL="$2"
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
WORK="$(mktemp -d)"
PKGROOT="$WORK/root"
CONTENT="$PKGROOT/Library/Application Support/Room Labs/African Riddim"
mkdir -p "$PKGROOT/Library/Audio/Plug-Ins/VST3" "$PKGROOT/Library/Audio/Plug-Ins/Components" "$PKGROOT/Applications" \
         "$CONTENT/Documentation" "$CONTENT/Licenses"
cp -R "$REL/VST3/African Riddim.vst3" "$PKGROOT/Library/Audio/Plug-Ins/VST3/"
cp -R "$REL/AU/African Riddim.component" "$PKGROOT/Library/Audio/Plug-Ins/Components/"
cp -R "$REL/Standalone/African Riddim.app" "$PKGROOT/Applications/"
cp "$ROOT/docs/LISEZ-MOI.txt" "$CONTENT/Documentation/"
cp "$ROOT/docs/LICENCES.txt" "$CONTENT/Licenses/"

# signature « ad hoc » (obligatoire sur Apple Silicon ; pas de compte Apple Developer)
for b in "$PKGROOT/Library/Audio/Plug-Ins/VST3/African Riddim.vst3" "$PKGROOT/Library/Audio/Plug-Ins/Components/African Riddim.component" "$PKGROOT/Applications/African Riddim.app"; do
  codesign --force --deep --sign - "$b"
done

mkdir -p "$WORK/dmg"
pkgbuild --root "$PKGROOT" --identifier com.roomlabs.africanriddim --version "$VERSION" \
         --install-location / --scripts "$ROOT/installer/macos/scripts" "$WORK/AfricanRiddim-core.pkg"
cat > "$WORK/distribution.xml" <<XML
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2">
  <title>African Riddim ${VERSION} — Room Labs</title>
  <options customize="never" require-scripts="false" hostArchitectures="arm64,x86_64"/>
  <domains enable_localSystem="true"/>
  <choices-outline><line choice="core"/></choices-outline>
  <choice id="core" title="African Riddim"><pkg-ref id="com.roomlabs.africanriddim"/></choice>
  <pkg-ref id="com.roomlabs.africanriddim" version="${VERSION}">AfricanRiddim-core.pkg</pkg-ref>
</installer-gui-script>
XML
productbuild --distribution "$WORK/distribution.xml" --package-path "$WORK" "$WORK/dmg/Installer African Riddim.pkg"

cat > "$WORK/dmg/LISEZ-MOI.txt" <<TXT
AFRICAN RIDDIM ${VERSION} — by Room Labs © 2026

1. Clic droit (ou Ctrl + clic) sur « Installer African Riddim.pkg » > Ouvrir > Ouvrir.
   (Le Mac affiche un avertissement car l'installeur n'est pas encore signé par Apple.)
   Si le bouton « Ouvrir » n'apparaît pas : Réglages Système > Confidentialité et sécurité
   > tout en bas « Ouvrir quand même ».
2. Suivre l'installation (mot de passe du Mac demandé).
3. Dans Logic Pro : le plugin est dans Instruments AU > Room Labs Audio > African Riddim.
   Dans Ableton Live / FL Studio Mac : VST3 > African Riddim (relancer la recherche des plugins).

Fichiers installés :
- /Library/Audio/Plug-Ins/Components/African Riddim.component (AU)
- /Library/Audio/Plug-Ins/VST3/African Riddim.vst3
- /Applications/African Riddim.app (version autonome)
- /Library/Application Support/Room Labs/African Riddim (documentation)
TXT

OUT="$ROOT/AfricanRiddim-macOS-${VERSION}.dmg"
rm -f "$OUT"
hdiutil create -volname "African Riddim ${VERSION}" -srcfolder "$WORK/dmg" -ov -format UDZO "$OUT"
echo "DMG : $OUT"
