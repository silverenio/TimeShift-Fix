@echo off
rem Build with Visual Studio Build Tools (x86 / 32-bit!)
call "C:\Program Files (x86)\Microsoft Visual Studio‚2\BuildTools\VC\Auxiliary\Buildcvarsall.bat" x86
cd /d "%~dp0"
cl /nologo /O2 /LD /MT dinput8.c /link /DEF:dinput8.def /OUT:dinput8.dll kernel32.lib
