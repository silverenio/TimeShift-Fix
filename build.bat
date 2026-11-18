@echo off
rem Builds src\dinput8.dll and installer\TimeShift-Fix-Installer.exe (32-bit) with VS Build Tools
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x86 >nul
cd /d "%~dp0src"
cl /nologo /O2 /LD /MT dinput8.c /link /DEF:dinput8.def /OUT:dinput8.dll kernel32.lib || exit /b 1
cd /d "%~dp0installer"
rc /nologo installer.rc || exit /b 1
cl /nologo /O2 /MT /DUNICODE /D_UNICODE installer.c installer.res /Fe:TimeShift-Fix-Installer.exe /link /SUBSYSTEM:WINDOWS user32.lib advapi32.lib comdlg32.lib || exit /b 1
