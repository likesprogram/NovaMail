#ifndef AppVersion
#define AppVersion "1.0.0"
#endif

#define AppName "NovaMailDesktop"
#define AppPublisher "NovaMail"
#define AppExeName "NovaMailDesktop.exe"
#define ShortcutName "新星邮箱"
#define ReleaseDir "..\x64\Release"
#define WebView2Bootstrapper "runtime\MicrosoftEdgeWebview2Setup.exe"

#ifndef InstallerLanguageName
#define InstallerLanguageName "english"
#endif

#ifndef InstallerMessagesFile
#define InstallerMessagesFile "compiler:Default.isl"
#endif

#ifexist WebView2Bootstrapper
#define BundlesWebView2
#endif

[Setup]
AppId=8da5e7c7-52bc-431e-b8ac-5a0cd1a68735
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher={#AppPublisher}
DefaultDirName={localappdata}\Programs\{#AppName}
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
OutputDir=..\dist\installer
OutputBaseFilename=NovaMailDesktop-Setup-{#AppVersion}-x64
SetupIconFile=..\NovaMailDesktop\res\favicon.ico
UninstallDisplayIcon={app}\{#AppExeName}
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
PrivilegesRequired=lowest
CloseApplications=yes
CloseApplicationsFilter={#AppExeName}
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern

[Languages]
Name: "{#InstallerLanguageName}"; MessagesFile: "{#InstallerMessagesFile}"

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Additional tasks:"; Flags: unchecked

[Files]
Source: "{#ReleaseDir}\{#AppExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#ReleaseDir}\WebView2Loader.dll"; DestDir: "{app}"; Flags: ignoreversion
#ifdef BundlesWebView2
Source: "{#WebView2Bootstrapper}"; DestDir: "{tmp}"; Flags: deleteafterinstall; Check: NeedsWebView2
#endif

[InstallDelete]
Type: filesandordirs; Name: "{localappdata}\NovaMailDesktop\vue-dist"

[Icons]
Name: "{autoprograms}\{#ShortcutName}"; Filename: "{app}\{#AppExeName}"; WorkingDir: "{app}"
Name: "{autodesktop}\{#ShortcutName}"; Filename: "{app}\{#AppExeName}"; WorkingDir: "{app}"; Tasks: desktopicon

[Run]
#ifdef BundlesWebView2
Filename: "{tmp}\MicrosoftEdgeWebview2Setup.exe"; Parameters: "/silent /install"; StatusMsg: "Installing Microsoft Edge WebView2 Runtime..."; Flags: waituntilterminated; Check: NeedsWebView2
#endif
Filename: "{app}\{#AppExeName}"; Description: "Launch {#AppName}"; Flags: nowait postinstall skipifsilent

[Code]
const
  WebView2ClientGuid = '{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}';

function BundlesWebView2Runtime(): Boolean;
begin
#ifdef BundlesWebView2
  Result := True;
#else
  Result := False;
#endif
end;

function HasWebView2VersionAt(RootKey: Integer; SubKey: String): Boolean;
var
  Version: String;
begin
  Result :=
    RegQueryStringValue(RootKey, SubKey, 'pv', Version) and
    (Version <> '') and
    (Version <> '0.0.0.0');
end;

function HasWebView2Runtime(): Boolean;
begin
  Result :=
    HasWebView2VersionAt(HKLM, 'Software\WOW6432Node\Microsoft\EdgeUpdate\Clients\' + WebView2ClientGuid) or
    HasWebView2VersionAt(HKLM, 'Software\Microsoft\EdgeUpdate\Clients\' + WebView2ClientGuid) or
    HasWebView2VersionAt(HKCU, 'Software\Microsoft\EdgeUpdate\Clients\' + WebView2ClientGuid);
end;

function NeedsWebView2(): Boolean;
begin
  Result := not HasWebView2Runtime();
end;

function InitializeSetup(): Boolean;
begin
  Result := True;

  if NeedsWebView2() and not BundlesWebView2Runtime() then
  begin
    MsgBox(
      'Microsoft Edge WebView2 Runtime was not detected.' + #13#10#13#10 +
      'NovaMailDesktop will still be installed, but the target computer needs WebView2 Runtime before the app can run.' + #13#10 +
      'To bundle it, put MicrosoftEdgeWebview2Setup.exe in installer\runtime and build the installer again.',
      mbInformation,
      MB_OK);
  end;
end;
