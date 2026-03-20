; ===========================================
; FintechApp Installer Script – Updated
; ===========================================

[Setup]
AppName=FintechApp
AppVersion=1.0.0
AppPublisher=FintechApp
DefaultDirName={pf32}\FintechApp
DefaultGroupName=FintechApp
OutputDir=.
OutputBaseFilename=FintechAppInstaller
Compression=lzma
SolidCompression=yes
WizardStyle=modern

; Require admin privileges to write to Program Files
PrivilegesRequired=admin
Uninstallable=yes

; -------------------------------------------
; Directories
; -------------------------------------------
[Dirs]
; Install folder and give modify permission to all users
Name: "{app}"; Permissions: users-modify

; -------------------------------------------
; Files to install
; -------------------------------------------
[Files]
; Main executable
Source: "FintechApp.exe"; DestDir: "{app}"; Flags: ignoreversion

; Add additional files here if needed
; Example: Source: "somefile.dat"; DestDir: "{app}"; Flags: ignoreversion

; -------------------------------------------
; Icons / Shortcuts
; -------------------------------------------
[Icons]
; Start Menu shortcut
Name: "{group}\FintechApp"; Filename: "{app}\FintechApp.exe"
; Desktop shortcut (optional)
Name: "{autodesktop}\FintechApp"; Filename: "{app}\FintechApp.exe"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Additional options:"

; -------------------------------------------
; Run app after installation
; -------------------------------------------
[Run]
Filename: "{app}\FintechApp.exe"; Description: "Launch FintechApp"; Flags: nowait postinstall skipifsilent

; -------------------------------------------
; Optional uninstall run (cleanup)
; -------------------------------------------
[UninstallDelete]
Type: filesandordirs; Name: "{app}\bank_data.json"