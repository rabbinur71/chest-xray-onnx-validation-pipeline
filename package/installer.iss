#define MyAppName "XRayDoctorAssist"
#define MyAppVersion "0.1.0"
#define MyAppPublisher "Raaz"
#define MyAppExeName "XRayDoctorAssist.exe"

[Setup]
AppId={{8F7116D3-91C6-4C27-B9AD-9D7C02B87F21}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
OutputDir=output
OutputBaseFilename=XRayDoctorAssist_Setup_v0.1.0
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
WizardStyle=modern
PrivilegesRequired=admin
DisableProgramGroupPage=yes
UninstallDisplayIcon={app}\{#MyAppExeName}

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Additional shortcuts:"; Flags: unchecked

[Dirs]
Name: "{commonappdata}\XRayDoctorAssist"; Permissions: users-modify
Name: "{commonappdata}\XRayDoctorAssist\data"; Permissions: users-modify
Name: "{commonappdata}\XRayDoctorAssist\reports"; Permissions: users-modify
Name: "{commonappdata}\XRayDoctorAssist\reports\final_reports"; Permissions: users-modify
Name: "{commonappdata}\XRayDoctorAssist\reports\preprocessing_validation"; Permissions: users-modify
Name: "{commonappdata}\XRayDoctorAssist\reports\onnx_c_runtime_validation"; Permissions: users-modify

[Files]
Source: "app\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\XRayDoctorAssist"; Filename: "{app}\{#MyAppExeName}"; WorkingDir: "{app}"
Name: "{autodesktop}\XRayDoctorAssist"; Filename: "{app}\{#MyAppExeName}"; WorkingDir: "{app}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Launch XRayDoctorAssist"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
Type: filesandordirs; Name: "{app}"

; Deliberately do NOT delete:
; {commonappdata}\XRayDoctorAssist
; This preserves audit database and generated reports across uninstall/reinstall.
