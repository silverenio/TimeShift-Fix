// TimeShift Fix installer: finds the Steam TimeShift install, drops the embedded dinput8.dll into bin\,
// and optionally switches the text language (English / Russian / Ukrainian) by rewriting the tail of
// preload\paks\patch.s3darc (original kept as patch.s3darc.tsfix_orig). License: MIT.
#include <windows.h>
#include <commdlg.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <wchar.h>
#include "resource.h"
#include "pak_info.h"

#define TITLE L"TimeShift Fix 2.2"

static wchar_t g_bin[MAX_PATH], g_pak[MAX_PATH], g_bak[MAX_PATH], g_dll[MAX_PATH];
static HINSTANCE g_hi;

static int Msg(HWND w, UINT flags, const wchar_t *fmt, ...) {
    wchar_t buf[2048]; va_list ap; va_start(ap, fmt); _vsnwprintf_s(buf, 2048, _TRUNCATE, fmt, ap); va_end(ap);
    return MessageBoxW(w, buf, TITLE, flags);
}
static BOOL FileExists(const wchar_t *p) { DWORD a = GetFileAttributesW(p); return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY); }

static DWORD crc_table[256];
static DWORD Crc32(const BYTE *p, DWORD n) {
    if (!crc_table[1]) for (DWORD i = 0; i < 256; i++) { DWORD c = i; for (int k = 0; k < 8; k++) c = c & 1 ? 0xEDB88320 ^ (c >> 1) : c >> 1; crc_table[i] = c; }
    DWORD c = 0xFFFFFFFF; while (n--) c = crc_table[(c ^ *p++) & 0xFF] ^ (c >> 8); return c ^ 0xFFFFFFFF;
}
static BYTE *ReadAll(const wchar_t *path, DWORD *size) {
    HANDLE f = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (f == INVALID_HANDLE_VALUE) return NULL;
    DWORD s = GetFileSize(f, NULL), rd = 0; BYTE *b = (BYTE *)malloc(s ? s : 1);
    if (!b || !ReadFile(f, b, s, &rd, NULL) || rd != s) { CloseHandle(f); free(b); return NULL; }
    CloseHandle(f); *size = s; return b;
}
static BOOL WriteAll(const wchar_t *path, const void *data, DWORD size) {
    HANDLE f = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL); DWORD wr = 0;
    if (f == INVALID_HANDLE_VALUE) return FALSE;
    BOOL ok = WriteFile(f, data, size, &wr, NULL) && wr == size; CloseHandle(f); return ok;
}
static const BYTE *Res(int id, DWORD *size) {
    HRSRC rs = FindResourceW(g_hi, MAKEINTRESOURCEW(id), RT_RCDATA); HGLOBAL rg = rs ? LoadResource(g_hi, rs) : NULL;
    *size = rs ? SizeofResource(g_hi, rs) : 0; return rg ? (const BYTE *)LockResource(rg) : NULL;
}

// ---------- locating the game ----------
static BOOL TryLibrary(const wchar_t *lib, wchar_t *out) {
    wchar_t p[MAX_PATH];
    swprintf_s(p, MAX_PATH, L"%s\\steamapps\\common\\TimeShift\\bin\\TimeShift.Exe", lib);
    if (!FileExists(p)) return FALSE;
    swprintf_s(out, MAX_PATH, L"%s\\steamapps\\common\\TimeShift\\bin", lib); return TRUE;
}
static BOOL FindGame(wchar_t *out) {
    wchar_t steam[MAX_PATH]; DWORD sz = sizeof steam;
    if (RegGetValueW(HKEY_CURRENT_USER, L"Software\\Valve\\Steam", L"SteamPath", RRF_RT_REG_SZ, NULL, steam, &sz) != ERROR_SUCCESS) {
        sz = sizeof steam;
        if (RegGetValueW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\WOW6432Node\\Valve\\Steam", L"InstallPath", RRF_RT_REG_SZ, NULL, steam, &sz) != ERROR_SUCCESS) return FALSE;
    }
    for (wchar_t *c = steam; *c; c++) if (*c == L'/') *c = L'\\';
    if (TryLibrary(steam, out)) return TRUE;
    wchar_t vdf[MAX_PATH]; swprintf_s(vdf, MAX_PATH, L"%s\\steamapps\\libraryfolders.vdf", steam);
    FILE *f; if (_wfopen_s(&f, vdf, L"rb")) return FALSE;
    char line[2048];
    while (fgets(line, sizeof line, f)) {
        char *k = strstr(line, "\"path\""); if (!k) continue;
        char *a = strchr(k + 6, '"'); if (!a) continue; a++;
        char val[1024]; int n = 0;
        for (; *a && *a != '"' && n < 1023; a++) { if (*a == '\\' && a[1] == '\\') a++; val[n++] = *a; }
        val[n] = 0; wchar_t lib[MAX_PATH]; MultiByteToWideChar(CP_UTF8, 0, val, -1, lib, MAX_PATH);
        if (TryLibrary(lib, out)) { fclose(f); return TRUE; }
    }
    fclose(f); return FALSE;
}
static BOOL AskForGame(wchar_t *out) {
    wchar_t file[MAX_PATH] = L"TimeShift.Exe"; OPENFILENAMEW of = { sizeof of };
    of.lpstrFilter = L"TimeShift.Exe\0TimeShift.Exe\0All files\0*.*\0"; of.lpstrFile = file; of.nMaxFile = MAX_PATH;
    of.lpstrTitle = L"Find TimeShift.Exe (steamapps\\common\\TimeShift\\bin)"; of.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (!GetOpenFileNameW(&of)) return FALSE;
    wcscpy_s(out, MAX_PATH, file); wchar_t *s = wcsrchr(out, L'\\'); if (s) *s = 0; return TRUE;
}
static BOOL GameRunning(void) {
    HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); PROCESSENTRY32W e = { sizeof e }; BOOL r = FALSE;
    if (h == INVALID_HANDLE_VALUE) return FALSE;
    for (BOOL ok = Process32FirstW(h, &e); ok; ok = Process32NextW(h, &e)) if (!_wcsicmp(e.szExeFile, L"TimeShift.Exe")) { r = TRUE; break; }
    CloseHandle(h); return r;
}

// ---------- language ----------
enum { LANG_UNKNOWN = -1, LANG_EN = 0, LANG_RU = 1, LANG_UK = 2 };
typedef struct { DWORD new_idx, tail_len, new_crc, new_size; } LangHdr;

static int DetectLang(void) {
    DWORD sz; BYTE *b = ReadAll(g_pak, &sz); if (!b) return LANG_UNKNOWN;
    DWORD crc = Crc32(b, sz); free(b);
    if (sz == PAK_ORIG_SIZE && crc == PAK_ORIG_CRC) return LANG_EN;
    for (int l = LANG_RU; l <= LANG_UK; l++) {
        DWORD rs; const LangHdr *h = (const LangHdr *)Res(l == LANG_RU ? IDR_LANG_RU : IDR_LANG_UK, &rs);
        if (h && sz == h->new_size && crc == h->new_crc) return l;
    }
    return LANG_UNKNOWN;
}
// Returns malloc'd original patch.s3darc (from backup or current file), or NULL if not the known Steam version.
static BYTE *LoadOriginal(void) {
    const wchar_t *src[2] = { g_bak, g_pak };
    for (int i = 0; i < 2; i++) {
        DWORD sz; BYTE *b = ReadAll(src[i], &sz); if (!b) continue;
        if (sz == PAK_ORIG_SIZE && Crc32(b, sz) == PAK_ORIG_CRC) return b;
        free(b);
    }
    return NULL;
}
static BOOL SetLanguage(HWND w, int lang) {
    BYTE *orig = LoadOriginal();
    if (!orig) {
        Msg(w, MB_OK | MB_ICONWARNING,
            L"The game's text archive (preload\\paks\\patch.s3darc) is not the standard Steam version,\nso the language was not changed (the fix itself is installed).\n\n"
            L"In Steam: TimeShift → Properties → Installed Files → Verify integrity, then run this installer again.");
        return FALSE;
    }
    if (!FileExists(g_bak) && !WriteAll(g_bak, orig, PAK_ORIG_SIZE)) { free(orig); Msg(w, MB_OK | MB_ICONERROR, L"Could not create backup:\n%s", g_bak); return FALSE; }
    BOOL ok;
    if (lang == LANG_EN) ok = WriteAll(g_pak, orig, PAK_ORIG_SIZE);
    else {
        DWORD rs; const BYTE *r = Res(lang == LANG_RU ? IDR_LANG_RU : IDR_LANG_UK, &rs); const LangHdr *h = (const LangHdr *)r;
        BYTE *n = (BYTE *)malloc(h->new_size);
        memcpy(n, orig, PAK_IDX_OFF); memcpy(n + PAK_HDR_POS, &h->new_idx, 4); memcpy(n + PAK_IDX_OFF, r + sizeof(LangHdr), h->tail_len);
        ok = Crc32(n, h->new_size) == h->new_crc && WriteAll(g_pak, n, h->new_size);
        free(n);
    }
    free(orig);
    if (!ok) Msg(w, MB_OK | MB_ICONERROR, L"Could not write:\n%s\n\nTry running the installer as administrator.", g_pak);
    return ok;
}

// ---------- install / uninstall ----------
static BOOL InstallDll(HWND w) {
    DWORD size; const BYTE *data = Res(IDR_DLL, &size);
    if (!data || !WriteAll(g_dll, data, size)) { Msg(w, MB_OK | MB_ICONERROR, L"Could not write:\n%s\n\nTry running the installer as administrator.", g_dll); return FALSE; }
    return TRUE;
}
static void Uninstall(HWND w) {
    BOOL ok = TRUE;
    if (FileExists(g_dll) && !DeleteFileW(g_dll)) ok = FALSE;
    if (FileExists(g_bak)) {
        DWORD sz; BYTE *b = ReadAll(g_bak, &sz);
        if (b && WriteAll(g_pak, b, sz)) DeleteFileW(g_bak); else ok = FALSE;
        free(b);
    }
    if (ok) Msg(w, MB_OK | MB_ICONINFORMATION, L"Removed. The game is back to its original state.\n\nВидалено. / Удалено.");
    else Msg(w, MB_OK | MB_ICONERROR, L"Some files could not be restored. Use Steam → Verify integrity of game files.");
}

static INT_PTR CALLBACK DlgProc(HWND w, UINT m, WPARAM wp, LPARAM lp) {
    switch (m) {
    case WM_INITDIALOG: {
        HICON ic = LoadIconW(g_hi, MAKEINTRESOURCEW(1));
        SendMessageW(w, WM_SETICON, ICON_BIG, (LPARAM)ic); SendMessageW(w, WM_SETICON, ICON_SMALL, (LPARAM)ic);
        SetDlgItemTextW(w, IDC_PATH, g_bin);
        int l = DetectLang(); if (l == LANG_UNKNOWN) l = LANG_EN;
        CheckRadioButton(w, IDC_EN, IDC_UK, IDC_EN + l);
        wchar_t st[256];
        swprintf_s(st, 256, L"Status: fix %s, text: %s", FileExists(g_dll) ? L"installed" : L"not installed",
                   l == LANG_RU ? L"Русский" : l == LANG_UK ? L"Українська" : L"English");
        SetDlgItemTextW(w, IDC_STATUS, st);
        EnableWindow(GetDlgItem(w, IDC_UNINSTALL), FileExists(g_dll) || FileExists(g_bak));
        return TRUE;
    }
    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case IDOK: {
            while (GameRunning()) if (Msg(w, MB_RETRYCANCEL | MB_ICONWARNING, L"TimeShift is running. Close the game and click Retry.") != IDRETRY) return TRUE;
            int lang = IsDlgButtonChecked(w, IDC_RU) ? LANG_RU : IsDlgButtonChecked(w, IDC_UK) ? LANG_UK : LANG_EN;
            if (!InstallDll(w)) return TRUE;
            BOOL langok = (DetectLang() == lang) || SetLanguage(w, lang);
            Msg(w, MB_OK | MB_ICONINFORMATION,
                L"Done! Start TimeShift from Steam.%s\n\n"
                L"Готово! Запускайте TimeShift из Steam.\nГотово! Запускайте TimeShift зі Steam.\n\n"
                L"Tip: if the game crashes in cutscenes at 4K, use 2560x1440.",
                langok ? L"" : L"\n(Language was not changed — see previous message.)");
            EndDialog(w, 0); return TRUE;
        }
        case IDC_UNINSTALL:
            while (GameRunning()) if (Msg(w, MB_RETRYCANCEL | MB_ICONWARNING, L"TimeShift is running. Close the game and click Retry.") != IDRETRY) return TRUE;
            if (Msg(w, MB_YESNO | MB_ICONQUESTION, L"Remove the fix and restore the original game files?\n\nВидалити фікс? / Удалить фикс?") == IDYES) { Uninstall(w); EndDialog(w, 0); }
            return TRUE;
        case IDCANCEL: EndDialog(w, 0); return TRUE;
        }
    }
    return FALSE;
}

int WINAPI wWinMain(HINSTANCE hi, HINSTANCE hp, PWSTR cmd, int show) {
    g_hi = hi;
    if (!FindGame(g_bin)) {
        if (Msg(NULL, MB_OKCANCEL | MB_ICONQUESTION, L"Could not find TimeShift in your Steam libraries.\n\nClick OK and select TimeShift.Exe manually\n(usually ...\\steamapps\\common\\TimeShift\\bin\\TimeShift.Exe).") != IDOK) return 1;
        if (!AskForGame(g_bin)) return 1;
    }
    swprintf_s(g_dll, MAX_PATH, L"%s\\dinput8.dll", g_bin);
    swprintf_s(g_pak, MAX_PATH, L"%s\\..\\preload\\paks\\patch.s3darc", g_bin);
    swprintf_s(g_bak, MAX_PATH, L"%s.tsfix_orig", g_pak);
    DialogBoxW(hi, MAKEINTRESOURCEW(IDD_MAIN), NULL, DlgProc);
    return 0;
}
