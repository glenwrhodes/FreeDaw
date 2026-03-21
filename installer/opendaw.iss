#ifndef MyAppVersion
  #define MyAppVersion "1.0.0"
#endif

#ifndef SourceDir
  #define SourceDir "..\build\OpenDaw_artefacts\Release"
#endif

#ifndef OutputDir
  #define OutputDir ".."
#endif

#ifndef LicenseDir
  #define LicenseDir ".."
#endif

#define MyAppName "OpenDaw"
#define MyAppPublisher "OpenDaw contributors"
#define MyAppURL "https://github.com/glenwrhodes/OpenDaw"
#define MyAppExeName "OpenDaw.exe"

[Setup]
AppId={{B8A3F2E1-7C4D-4A6E-9D1B-3E5F8C2A7D4E}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}/issues
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
LicenseFile={#LicenseDir}\LICENSE
OutputBaseFilename=OpenDaw-v{#MyAppVersion}-setup
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
UninstallDisplayIcon={app}\{#MyAppExeName}
DisableProgramGroupPage=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "{#SourceDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Registry]
Root: HKCR; Subkey: ".tracktionedit"; ValueType: string; ValueName: ""; ValueData: "OpenDaw.Project"; Flags: uninsdeletevalue
Root: HKCR; Subkey: "OpenDaw.Project"; ValueType: string; ValueName: ""; ValueData: "OpenDaw Project File"; Flags: uninsdeletekey
Root: HKCR; Subkey: "OpenDaw.Project\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\{#MyAppExeName},0"
Root: HKCR; Subkey: "OpenDaw.Project\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent
