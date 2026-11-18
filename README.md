# TimeShift (Steam) — Modern PC Fix

Fixes for the Steam version of **TimeShift** (2007, app 10130) on Windows 10/11:

- Crash on startup (`Unhandled exception ... timeshift_minidump.dmp saved`) on CPUs with more than 8 threads (Ryzen, Intel 12th–14th gen, etc.)
- "Incompatible Video Driver" dialog on modern GPUs

No .exe patching: the Steam exe is DRM-protected and refuses to start if modified
(`Application load error 3:0000065432`). Everything is done at runtime by a small proxy DLL (`dinput8.dll`).

## Install

**Easy way:** download `TimeShift-Fix-Installer.exe` from [Releases](../../releases/latest) and run it.
It finds TimeShift in your Steam libraries and installs the fix. Run it again to uninstall.

> The installer is not code-signed, so Windows SmartScreen may show "Windows protected your PC" →
> click **More info → Run anyway**. Source code is in `installer/`.

**Manual way:**

1. Download `TimeShift-Fix.zip` from [Releases](../../releases/latest).
2. Copy `dinput8.dll` to `<Steam>\steamapps\common\TimeShift\bin\` (next to `TimeShift.Exe`).
3. Launch the game from Steam.

## Uninstall

Run the installer again and choose **No = remove**, or delete `bin\dinput8.dll`.

## What it does

1. Makes the game see 4 CPU cores instead of the real number (`GetSystemInfo` hook). The engine crashes with > 8 threads; setting CPU affinity does **not** help, because the game asks Windows for the total core count.
2. Reports a driver version the game accepts (`8.15.11.8593`) via Direct3D 9 (it wants NVIDIA `x.15.11.6700+` on Vista and newer).
3. Skips the "Incompatible Video Driver" dialog with a 1-byte patch **in memory** (after Steam decrypts the exe), applied only if the exact expected bytes are found — otherwise nothing is touched.

DirectInput calls are forwarded to the real Windows `dinput8.dll`.

## Notes

- Tested: Steam version, Windows 11, i7-14700KF (28 threads), RTX 3090.
- During install Steam runs **GameSpy Comrade**, which tries to download .NET 1.1 from a dead link. Press Cancel — it is not needed.
- The game is 32-bit (2 GB memory limit). 4K may crash in cutscenes or loading; if so, use 2560×1440. Avoid DXVK at 4K — it uses extra address space.
- Some antivirus tools flag unknown DLLs. Full source is in `src/` — build it yourself with Visual Studio Build Tools: run `build.bat`.

SHA-256: see the release notes.

## License

MIT
