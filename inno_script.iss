#define MyAppName "ThreeFingerDrag"
#define MyAppVersion "1.1.0"
#define MyAppPublisher "Austin Nixholm"
#define MyAppURL "https://github.com/austinnixholm/ThreeFingerDrag"
#define MyAppExeName "ThreeFingerDrag.exe"

[Setup]
AppId={{1D896AD4-D9F6-4A5F-9F96-69B369E0A015}}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName="{#MyAppName} {#MyAppVersion}"
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={pf}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
LicenseFile=LICENSE
OutputDir=installers
OutputBaseFilename="ThreeFingerDrag.Setup.{#MyAppVersion}"
SetupIconFile="{#MyAppName}\{#MyAppName}.ico"
UninstallDisplayIcon="{app}\{#MyAppName}.ico"
Compression=lzma
SolidCompression=yes
WizardStyle=modern

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "build\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#MyAppName}\{#MyAppName}.ico"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#MyAppName}\small.ico"; DestDir: "{app}"; Flags: ignoreversion
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#MyAppName}}"; Flags: nowait postinstall skipifsilent
