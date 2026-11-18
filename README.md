# TimeShift (Steam) — Fix + Українізатор + Русификатор

**[English](#english) · [Українська](#українська) · [Русский](#русский)**

One installer for the Steam version of **TimeShift** (2007):
fixes the startup crash on modern PCs and lets you choose the game text language — **English / Українська / Русский**.

**Download:** [TimeShift-Fix-Installer.exe](../../releases/latest) → run → choose language → Install.

---

## English

### What it fixes
- Crash on startup (`Unhandled exception ... timeshift_minidump.dmp saved`) on CPUs with more than 8 threads (Ryzen, Intel 12th–14th gen…).
- The "Incompatible Video Driver" window on modern GPUs.
- Optional: **Ukrainian** or **Russian** text (menus, subtitles, hints). Voice-over stays English.

### Install
1. Download `TimeShift-Fix-Installer.exe` from [Releases](../../releases/latest) and run it.
2. Choose the text language and click **Install**. The installer finds TimeShift in your Steam libraries by itself.
3. Start the game from Steam.

To switch language or uninstall, just run the installer again.
SmartScreen may warn because the exe is not code-signed: **More info → Run anyway**. All source code is in this repo.

### How it works
- A small proxy `dinput8.dll` makes the game see 4 CPU cores (the 2007 engine crashes with > 8 threads; CPU affinity does **not** help), reports a driver version the game accepts, and skips the driver warning with a 1-byte in-memory patch (only if the exact expected bytes are found). The Steam `TimeShift.Exe` is never modified — it is DRM-protected and refuses to start if changed.
- Language: the Steam version already contains the official Russian text, but switching the game to Russian mode breaks startup. The installer instead points the game's English text entry in `preload\paks\patch.s3darc` to Russian or Ukrainian text. The original file is backed up as `patch.s3darc.tsfix_orig` and restored on uninstall.
- Ukrainian: fan translation (≈3,900 lines). The game's 8-bit text table has no **і ї є ґ**: і/ї use Latin i/ï, and **Є/є are real separate glyphs** built into the game font (the font's rounded Е serves as Є; a new closed Е and a new є are assembled from existing letter blocks, see `ua/font_patch.py`). Ґ is not needed in this text.

### Notes
- Tested: Steam version, Windows 11, i7-14700KF (28 threads), RTX 3090.
- GameSpy Comrade installer hangs on .NET 1.1 — press Cancel, it's not needed.
- The game is 32-bit (2 GB memory limit): at 4K cutscenes may crash — use 2560×1440. Don't use DXVK at 4K.

---

## Українська

### Що виправляє
- Виліт одразу після запуску (`Unhandled exception ... timeshift_minidump.dmp saved`) на процесорах із понад 8 потоками.
- Вікно «Incompatible Video Driver» на сучасних відеокартах.
- **Українізатор:** меню, субтитри й підказки українською (≈3900 рядків). Озвучення лишається англійським.

### Встановлення
1. Завантажте `TimeShift-Fix-Installer.exe` з [Releases](../../releases/latest) і запустіть.
2. Оберіть **Українська** та натисніть **Install / Встановити**. Інсталятор сам знайде гру в бібліотеках Steam.
3. Запускайте гру зі Steam.

Щоб змінити мову або видалити — запустіть інсталятор ще раз.
Якщо Windows SmartScreen попереджає (файл без цифрового підпису): **Докладніше → Усе одно запустити**.

### Важливо
- Літери **Є/є** намальовані в шрифті гри окремо від Е/е (ми дорисували шрифт). **і, ї** показуються латинськими i, ï — виглядають так само. Ґ у тексті гри не трапляється.
- Оригінальні файли гри зберігаються й повертаються при видаленні. «Перевірити цілісність файлів» у Steam теж поверне англійську.

---

## Русский

### Что исправляет
- Вылет сразу после запуска (`Unhandled exception ... timeshift_minidump.dmp saved`) на процессорах с более чем 8 потоками.
- Окно «Incompatible Video Driver» на современных видеокартах.
- **Русификатор:** официальный русский текст (меню, субтитры, подсказки), который уже есть в Steam-версии, но не включается штатно. Озвучка остаётся английской.

### Установка
1. Скачайте `TimeShift-Fix-Installer.exe` из [Releases](../../releases/latest) и запустите.
2. Выберите **Русский** и нажмите **Install**. Установщик сам найдёт игру в библиотеках Steam.
3. Запускайте игру из Steam.

Чтобы сменить язык или удалить — запустите установщик ещё раз.
Если SmartScreen предупреждает (файл без цифровой подписи): **Подробнее → Выполнить в любом случае**.

---

## Build from source
Visual Studio Build Tools (x86) → `build.bat`. Language payloads are generated from your own game files: `ua/extract_fonts.py <out.s3darc>`, then `ua/make_installer_data.py` (needs the original `patch.s3darc` from the Steam version; game files are not included in this repo).

## License
Code: MIT. Ukrainian translation (`ua/uk_parts/`): free to use with credit. TimeShift © Sierra/Activision; this is an unofficial fan project.
