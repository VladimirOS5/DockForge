; DockForge Installer Script
; Requires Inno Setup 6.2+

#define MyAppName "DockForge"
#define MyAppVersion "1.0.0-alpha"
#define MyAppPublisher "DockForge Team"
#define MyAppURL "https://dockforge.app"
#define MyAppExeName "DockForge.exe"
#define MyAppMutex "DockForge_SingleInstance_Mutex"

[Setup]
AppId={{A1B2C3D4-E5F6-7890-ABCD-EF1234567890}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}/support
AppUpdatesURL={#MyAppURL}/releases
DefaultDirName={autopf}\{#MyAppName}
DisableProgramGroupPage=yes
LicenseFile=LICENSE.txt
OutputDir=dist
OutputBaseFilename=DockForge_1.0.0_Setup
SetupIconFile=res\icon.ico
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
CloseApplications=force
RestartApplications=no
UninstallDisplayIcon={app}\{#MyAppExeName}
UninstallDisplayName={#MyAppName}
VersionInfoVersion={#MyAppVersion}
VersionInfoCompany={#MyAppPublisher}
VersionInfoDescription=DockForge - Advanced Taskbar Replacement
VersionInfoCopyright=© 2026 {#MyAppPublisher}
MinVersion=10.0.19041
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "startwithwindows"; Description: "Start DockForge with Windows"; GroupDescription: "Startup:"
Name: "installvcredist"; Description: "Install Visual C++ Redistributable (required)"; GroupDescription: "Dependencies:"; Check: VCRedistNeedsInstall; Flags: checked

[Files]
Source: "build\Release\DockForge.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "build\Release\DockForge.Watchdog.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "LICENSE.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "README.md"; DestDir: "{app}"; Flags: ignoreversion isreadme
Source: "res\icon.ico"; DestDir: "{app}\res"; Flags: ignoreversion

; Visual C++ Redistributable
Source: "deps\vc_redist.x64.exe"; DestDir: "{tmp}"; Flags: deleteafterinstall; Check: VCRedistNeedsInstall

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
; Install VC++ Redist silently
Filename: "{tmp}\vc_redist.x64.exe"; Parameters: "/install /quiet /norestart"; StatusMsg: "Installing Visual C++ Redistributable..."; Tasks: installvcredist; Check: VCRedistNeedsInstall

; Launch after install
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

[UninstallRun]
Filename: "{app}\{#MyAppExeName}"; Parameters: "/uninstall"; RunOnceId: "Cleanup"; Flags: waituntilterminated

[Registry]
; Auto-start with Windows
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "DockForge"; ValueData: """{app}\{#MyAppExeName}"""; Tasks: startwithwindows

; File associations (optional - register as handler for .dockforge themes)
Root: HKCU; Subkey: "Software\Classes\.dockforge"; ValueType: string; ValueName: ""; ValueData: "DockForgeTheme"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\DockForgeTheme\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}""" ""%1"""

[UninstallDelete]
Type: filesandordirs; Name: "{localappdata}\DockForge\updates"
Type: filesandordirs; Name: "{localappdata}\DockForge\logs"
Type: files; Name: "{localappdata}\DockForge\crash.flag"

[Code]
function VCRedistNeedsInstall: Boolean;
begin
  Result := not RegKeyExists(HKLM, 'SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64');
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
  begin
    ; Create version marker for OTA
    SaveStringToFile(ExpandConstant('{localappdata}\DockForge\version.txt'), '{#MyAppVersion}', False);
  end;
end;

function InitializeSetup(): Boolean;
begin
  ; Check if already running
  if CheckForMutexes('{#MyAppMutex}') then
  begin
    MsgBox('DockForge is currently running. Please close it before installing.', mbError, MB_OK);
    Result := false;
    Exit;
  end;
  Result := true;
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
begin
  ; Kill any running DockForge processes
  Exec('taskkill', '/F /IM DockForge.exe', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
  Exec('taskkill', '/F /IM DockForge.Watchdog.exe', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
  Result := '';
end;
