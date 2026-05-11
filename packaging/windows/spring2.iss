#ifndef RepositoryRoot
  #error RepositoryRoot must be defined on the ISCC command line.
#endif

#ifndef BinarySourceDir
  #error BinarySourceDir must be defined on the ISCC command line.
#endif

#ifndef OutputDir
  #error OutputDir must be defined on the ISCC command line.
#endif

#ifndef AppVersion
  #error AppVersion must be defined on the ISCC command line.
#endif

#ifndef AppNumericVersion
  #error AppNumericVersion must be defined on the ISCC command line.
#endif

#ifndef BuildArch
  #error BuildArch must be defined on the ISCC command line.
#endif

#ifndef ArchitecturesAllowed
  #error ArchitecturesAllowed must be defined on the ISCC command line.
#endif

#ifndef ArchitecturesInstallIn64BitMode
  #error ArchitecturesInstallIn64BitMode must be defined on the ISCC command line.
#endif

#define AppId "{{7C5BD38D-58B7-4683-9F08-C03E7F4FF02A}"
#define AppName "SPRING2"
#define AppPublisher "SPRING2 contributors"
#define AppURL "https://github.com/thisisamirv/SPRING2"
#define AppExeName "spring2.exe"

[Setup]
AppId={#AppId}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher={#AppPublisher}
AppPublisherURL={#AppURL}
AppSupportURL={#AppURL}
AppUpdatesURL={#AppURL}
AppCopyright=See LICENSE for SPRING research-use terms.
DefaultDirName={autopf}\{#AppName}
DefaultGroupName={#AppName}
DisableDirPage=no
DisableProgramGroupPage=yes
LicenseFile={#RepositoryRoot}\LICENSE
InfoAfterFile={#RepositoryRoot}\packaging\windows\README.txt
OutputDir={#OutputDir}
OutputBaseFilename=spring2-windows-{#BuildArch}-setup
SetupIconFile={#RepositoryRoot}\docs\assets\icons\logo.ico
UninstallDisplayIcon={app}\{#AppExeName}
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed={#ArchitecturesAllowed}
ArchitecturesInstallIn64BitMode={#ArchitecturesInstallIn64BitMode}
PrivilegesRequired=admin
MinVersion=10.0
VersionInfoVersion={#AppNumericVersion}
VersionInfoProductVersion={#AppVersion}
VersionInfoCompany={#AppPublisher}
VersionInfoDescription={#AppName} Windows Installer
VersionInfoProductName={#AppName}

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "addtopath"; Description: "Add SPRING2 to the system PATH"; Flags: unchecked
Name: "desktopicon"; Description: "Create a desktop shortcut"; Flags: unchecked

[Files]
Source: "{#BinarySourceDir}\{#AppExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#RepositoryRoot}\LICENSE"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#RepositoryRoot}\packaging\windows\README.txt"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\SPRING2"; Filename: "{app}\{#AppExeName}"; WorkingDir: "{app}"
Name: "{group}\SPRING2 README"; Filename: "{app}\README.txt"
Name: "{group}\Uninstall SPRING2"; Filename: "{uninstallexe}"
Name: "{autodesktop}\SPRING2"; Filename: "{app}\{#AppExeName}"; WorkingDir: "{app}"; Tasks: desktopicon

[Run]
Filename: "{app}\README.txt"; Description: "Open Windows installation notes"; Flags: postinstall shellexec skipifsilent unchecked

[Code]
const
  EnvironmentKey = 'SYSTEM\CurrentControlSet\Control\Session Manager\Environment';
  PathValueName = 'Path';
  HWND_BROADCAST = $FFFF;
  WM_SETTINGCHANGE = $001A;
  SMTO_ABORTIFHUNG = $0002;

function SendMessageTimeout(hWnd: Integer; Msg: Integer; wParam: Integer; lParam: string;
  fuFlags: Integer; uTimeout: Integer; out lpdwResult: Integer): Integer;
  external 'SendMessageTimeoutW@user32.dll stdcall';

function NormalizePathValue(const Value: string): string;
begin
  Result := RemoveBackslashUnlessRoot(Value);
end;

function SplitPathEntries(const PathValue: string): TArrayOfString;
begin
  Result := SplitString(PathValue, ';');
end;

function JoinPathEntries(const Entries: TArrayOfString): string;
var
  I: Integer;
begin
  Result := '';
  for I := 0 to GetArrayLength(Entries) - 1 do begin
    if Trim(Entries[I]) = '' then begin
      continue;
    end;

    if Result <> '' then begin
      Result := Result + ';';
    end;
    Result := Result + Entries[I];
  end;
end;

function PathContainsEntry(const PathValue: string; const Entry: string): Boolean;
var
  Entries: TArrayOfString;
  I: Integer;
begin
  Result := False;
  Entries := SplitPathEntries(PathValue);
  for I := 0 to GetArrayLength(Entries) - 1 do begin
    if CompareText(NormalizePathValue(Trim(Entries[I])), NormalizePathValue(Entry)) = 0 then begin
      Result := True;
      exit;
    end;
  end;
end;

function AddPathEntry(const PathValue: string; const Entry: string): string;
begin
  if PathContainsEntry(PathValue, Entry) then begin
    Result := PathValue;
  end else if Trim(PathValue) = '' then begin
    Result := Entry;
  end else begin
    Result := PathValue + ';' + Entry;
  end;
end;

function RemovePathEntry(const PathValue: string; const Entry: string): string;
var
  Entries: TArrayOfString;
  FilteredEntries: TArrayOfString;
  I: Integer;
  Count: Integer;
begin
  Entries := SplitPathEntries(PathValue);
  Count := 0;
  SetArrayLength(FilteredEntries, GetArrayLength(Entries));

  for I := 0 to GetArrayLength(Entries) - 1 do begin
    if CompareText(NormalizePathValue(Trim(Entries[I])), NormalizePathValue(Entry)) <> 0 then begin
      FilteredEntries[Count] := Trim(Entries[I]);
      Count := Count + 1;
    end;
  end;

  SetArrayLength(FilteredEntries, Count);
  Result := JoinPathEntries(FilteredEntries);
end;

procedure BroadcastEnvironmentChange;
var
  MessageResult: Integer;
begin
  SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, 0, 'Environment',
    SMTO_ABORTIFHUNG, 5000, MessageResult);
end;

procedure UpdateSystemPath(const AddEntry: Boolean);
var
  CurrentPath: string;
  UpdatedPath: string;
  InstallPath: string;
begin
  InstallPath := ExpandConstant('{app}');
  if not RegQueryStringValue(HKLM, EnvironmentKey, PathValueName, CurrentPath) then begin
    CurrentPath := '';
  end;

  if AddEntry then begin
    UpdatedPath := AddPathEntry(CurrentPath, InstallPath);
  end else begin
    UpdatedPath := RemovePathEntry(CurrentPath, InstallPath);
  end;

  if UpdatedPath <> CurrentPath then begin
    RegWriteExpandStringValue(HKLM, EnvironmentKey, PathValueName, UpdatedPath);
    BroadcastEnvironmentChange;
  end;
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if (CurStep = ssPostInstall) and WizardIsTaskSelected('addtopath') then begin
    UpdateSystemPath(True);
  end;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if CurUninstallStep = usPostUninstall then begin
    UpdateSystemPath(False);
  end;
end;