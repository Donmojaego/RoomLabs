; Installeur Windows de Smoke (Inno Setup 6)
; Compilé automatiquement par GitHub Actions :
;   iscc /DAppVersion=0.1.0 /DBuildDir=..\..\build\Smoke_artefacts\Release Smoke.iss

#ifndef AppVersion
  #define AppVersion "0.1.0"
#endif
#ifndef BuildDir
  #define BuildDir "..\..\build\Smoke_artefacts\Release"
#endif

[Setup]
AppId={{95EC04EC-D071-4B78-883D-25E5379034D8}
AppName=Smoke
AppVersion={#AppVersion}
AppPublisher=Room Labs
AppPublisherURL=https://7throomacademy.fr
AppCopyright=© 2026 Room Labs
DefaultDirName={commonpf64}\Smoke
DisableDirPage=yes
DefaultGroupName=Room Labs\Smoke
DisableProgramGroupPage=yes
UninstallFilesDir={app}\Uninstall
OutputDir=output
OutputBaseFilename=Smoke-Setup-{#AppVersion}
Compression=lzma2/max
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
WizardStyle=modern
SetupLogging=yes

[Languages]
Name: "fr"; MessagesFile: "compiler:Languages\French.isl"

[Types]
Name: "complet"; Description: "Installation complète"
Name: "perso"; Description: "Installation personnalisée"; Flags: iscustom

[Components]
Name: "vst3"; Description: "Plugin VST3 (FL Studio, Ableton Live…)"; Types: complet perso; Flags: fixed
Name: "standalone"; Description: "Application autonome (sans DAW)"; Types: complet perso
Name: "contenu"; Description: "Presets, documentation et licences"; Types: complet perso; Flags: fixed

[Dirs]
Name: "{app}\Samples"
Name: "{app}\Presets"

[Files]
; Le plugin va dans le dossier VST3 standard, scanné automatiquement par FL Studio
Source: "{#BuildDir}\VST3\Smoke.vst3\*"; DestDir: "{commonpf64}\Common Files\VST3\Smoke.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#BuildDir}\Standalone\Smoke.exe"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion
Source: "..\..\presets\factory.json"; DestDir: "{app}\Presets"; Components: contenu; Flags: ignoreversion
Source: "..\..\docs\LISEZ-MOI.txt"; DestDir: "{app}\Documentation"; Components: contenu; Flags: ignoreversion isreadme
Source: "..\..\docs\LICENCES.txt"; DestDir: "{app}\Licenses"; Components: contenu; Flags: ignoreversion

[Icons]
Name: "{group}\Smoke (autonome)"; Filename: "{app}\Smoke.exe"; Components: standalone
Name: "{group}\Désinstaller Smoke"; Filename: "{uninstallexe}"

[UninstallDelete]
Type: filesandordirs; Name: "{commonpf64}\Common Files\VST3\Smoke.vst3"

[Code]
// L'interface utilise WebView2, déjà présent sur Windows 10/11 à jour.
function WebView2Installed(): Boolean;
var v: String;
begin
  Result := RegQueryStringValue(HKLM, 'SOFTWARE\WOW6432Node\Microsoft\EdgeUpdate\Clients\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}', 'pv', v) and (v <> '') and (v <> '0.0.0.0');
  if not Result then
    Result := RegQueryStringValue(HKCU, 'Software\Microsoft\EdgeUpdate\Clients\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}', 'pv', v) and (v <> '') and (v <> '0.0.0.0');
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if (CurStep = ssPostInstall) and (not WebView2Installed()) then
    MsgBox('Smoke est installé.' + #13#10 + #13#10 +
           'Le composant Microsoft WebView2 (utilisé pour afficher l''interface) n''a pas été détecté. ' +
           'Il est normalement déjà présent sur Windows 10 et 11 à jour. ' +
           'Si l''interface du plugin reste blanche, installez « Microsoft Edge WebView2 Runtime » depuis le site de Microsoft.',
           mbInformation, MB_OK);
end;
