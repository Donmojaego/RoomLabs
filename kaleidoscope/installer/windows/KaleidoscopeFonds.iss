; Installeur des 100 fonds de Kaleidoscope (instruments échantillonnés CC0, VCSL et VSCO 2 CE)
; Compilé par GitHub Actions (workflow « Kaleidoscope - Fonds ») :
;   iscc /DBankVersion=0.1.0 /DBankDir=..\..\dist\Banques KaleidoscopeFonds.iss
#ifndef BankVersion
  #define BankVersion "0.1.0"
#endif
#ifndef BankDir
  #define BankDir "..\..\dist\Banques"
#endif

[Setup]
AppId={{6F0B7C55-3E1D-4E0A-9C2E-4B1A5F0D0001}
AppName=Kaleidoscope - Fonds
AppVersion={#BankVersion}
AppPublisher=Room Labs
AppPublisherURL=https://7throomacademy.fr
DefaultDirName={commonappdata}\Room Labs\Kaleidoscope\Banques
DisableDirPage=yes
DisableProgramGroupPage=yes
OutputDir=output
OutputBaseFilename=Kaleidoscope-Fonds-Setup-{#BankVersion}
Compression=lzma2/fast
SolidCompression=no
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
WizardStyle=modern
DiskSpanning=no

[Languages]
Name: "fr"; MessagesFile: "compiler:Languages\French.isl"

[Files]
Source: "{#BankDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\..\licences\inventaire.csv"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\licences\FONDS-LICENCES.txt"; DestDir: "{app}"; Flags: ignoreversion isreadme
