
rem ember value of GOTO: is used to know recursion has happened.
if "%1" == "GOTO:" goto %2

if NOT "x%WD%" == "x" set WD=

rem ember command.com only uses the first eight characters of the label.
goto _WindowsNT

rem ember that we only execute here if we are in command.com.
:_Windows

if "x%COMSPEC%" == "x" set COMSPEC=command.com
start /min %COMSPEC% /e:4096 /c %0 GOTO: _Resume %0 %1 %2 %3 %4 %5 %6 %7 %8 %9
goto EOF

rem ember that we execute here if we recursed.
:_Resume
for %%F in (1 2 3) do shift
if NOT EXIST %WD%msys-1.0.dll set WD=.\bin\

rem ember that we get here even in command.com.
:_WindowsNT

rem Hopefully a temporary workaround for getting MSYS shell to run on x64
rem (WoW64 cmd prompt sets PROCESSOR_ARCHITECTURE to x86)
if not "x%PROCESSOR_ARCHITECTURE%" == "xAMD64" goto _NotX64
set COMSPEC=%WINDIR%\SysWOW64\cmd.exe
%COMSPEC% /c %0 %1 %2 %3 %4 %5 %6 %7 %8 %9
goto EOF
:_NotX64

if NOT EXIST %WD%msys-1.0.dll set WD=%~dp0\bin\

rem ember Set up option to use rxvt based on value of %1
set MSYSCON=unknown
if "x%1" == "x-norxvt" set MSYSCON=sh.exe
if "x%1" == "x--norxvt" set MSYSCON=sh.exe
if "x%1" == "x-rxvt" set MSYSCON=rxvt.exe
if "x%1" == "x--rxvt" set MSYSCON=rxvt.exe
if NOT "x%MSYSCON%" == "xunknown" shift

if "x%MSYSCON%" == "xunknown" set MSYSCON=sh.exe

if "x%MSYSTEM%" == "x" set MSYSTEM=MINGW32
if "%1" == "MINGW32" set MSYSTEM=MINGW32
if "%1" == "MSYS" set MSYSTEM=MSYS

if NOT "x%DISPLAY%" == "x" set DISPLAY=

if "x%MSYSCON%" == "xrxvt.exe" goto startrxvt
if "x%MSYSCON%" == "xsh.exe" goto startsh

:unknowncon
echo %MSYSCON% is an unknown option for msys.bat.
pause
exit 1

:notfound
echo Cannot find the rxvt.exe or sh.exe binary -- aborting.
pause
exit 1

rem If you don't want to use rxvt then rename the file rxvt.exe to something
rem else.  Then sh.exe will be used instead.
:startrxvt
if NOT EXIST %WD%rxvt.exe goto startsh

rem Setup the default colors for rxvt.
if "x%MSYSBGCOLOR%" == "x" set MSYSBGCOLOR=White
if "x%MSYSFGCOLOR%" == "x" set MSYSFGCOLOR=Black
if "x%MINGW32BGCOLOR%" == "x" set MINGW32BGCOLOR=LightYellow
if "x%MINGW32FGCOLOR%" == "x" set MINGW32FGCOLOR=Navy
if "%MSYSTEM%" == "MSYS" set BGCOLOR=%MSYSBGCOLOR%
if "%MSYSTEM%" == "MSYS" set FGCOLOR=%MSYSFGCOLOR%
if "%MSYSTEM%" == "MINGW32" set BGCOLOR=%MINGW32BGCOLOR%
if "%MSYSTEM%" == "MINGW32" set FGCOLOR=%MINGW32FGCOLOR%

start %WD%rxvt -backspacekey  -sl 2500 -fg %FGCOLOR% -bg %BGCOLOR% -sr -fn Courier-12 -tn msys -geometry 80x25 -e /bin/sh --login -i
exit

:startsh
if NOT EXIST %WD%sh.exe goto notfound
start %WD%sh --login -i
exit

:EOF
