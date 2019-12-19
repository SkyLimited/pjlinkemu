; -- Example1.iss --
; Demonstrates copying 3 files and creating an icon.

; SEE THE DOCUMENTATION FOR DETAILS ON CREATING .ISS SCRIPT FILES!

[Setup]
AppName=PJTest
AppVersion=0.2
WizardStyle=modern
DefaultDirName={autopf}\PJTest
DefaultGroupName=PJLink Support
UninstallDisplayIcon={app}\PJTest.exe
Compression=lzma2
SolidCompression=yes
OutputDir=C:\Users\Admin\source\repos\PJTest
PrivilegesRequired=admin

[Files]
Source: "c:\Users\Admin\source\repos\PJTest\Release\PJTest.exe"; DestDir: "{app}" ;  Flags: ignoreversion;  BeforeInstall: TaskKill('PJTest.exe')
Source: "c:\Users\Admin\source\repos\PJTest\pdcurses.dll"; DestDir: "{app}"
Source: "c:\Users\Admin\source\repos\PJTest\VC_redist.x86.exe"; DestDir: "{tmp}"

[Run]
Filename: "{tmp}\VC_redist.x86.exe"; Parameters: "/install /quiet"
Filename: {app}\PJTest.exe; Parameters: "--hide"; Flags: nowait postinstall skipifsilent

[Registry]
;any user
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Windows\CurrentVersion\Run"; \
    ValueType: string; ValueName: "PJLinkTest"; \
    ValueData: """{app}\PJTest.exe"" --hide"

[Code]
procedure TaskKill(FileName: String);
var
  ResultCode: Integer;
begin
    Exec(ExpandConstant('taskkill.exe'), '/f /im ' + '"' + FileName + '"', '', SW_HIDE,
     ewWaitUntilTerminated, ResultCode);
end;