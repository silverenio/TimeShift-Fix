// TimeShift Fix installer: finds the Steam TimeShift install and drops the embedded dinput8.dll into bin\.
// Run again to uninstall. License: MIT.
#include <windows.h>
#include <commdlg.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <wchar.h>

#define TITLE L"TimeShift Fix"
#define IDR_DLL 101

static int Msg(UINT flags, const wchar_t *fmt, ...) {
    wchar_t buf[2048]; va_list ap; va_start(ap, fmt); _vsnwprintf_s(buf, 2048, _TRUNCATE, fmt, ap); va_end(ap);
    return MessageBoxW(NULL, buf, TITLE, flags);
}

static BOOL FileExists(const wchar_t *p) { DWORD a = GetFileAttributesW(p); return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY); }

// <lib>\steamapps\common\TimeShift\bin  -> out (if TimeShift.Exe exists there)
static BOOL TryLibrary(const wchar_t *lib, wchar_t *out) {
    wchar_t p[MAX_PATH];
    swprintf_s(p, MAX_PATH, L"%s\\steamapps\\common\\TimeShift\\bin\\TimeShift.Exe", lib);
    if (!FileExists(p)) return FALSE;
    swprintf_s(out, MAX_PATH, L"%s\\steamapps\\common\\TimeShift\\bin", lib);
    return TRUE;
}

static BOOL FindGame(wchar_t *out) {
    wchar_t steam[MAX_PATH]; DWORD sz = sizeof steam;
    if (RegGetValueW(HKEY_CURRENT_USER, L"Software\\Valve\\Steam", L"SteamPath", RRF_RT_REG_SZ, NULL, steam, &sz) != ERROR_SUCCESS) {
        sz = sizeof steam;
        if (RegGetValueW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\WOW6432Node\\Valve\\Steam", L"InstallPath", RRF_RT_REG_SZ, NULL, steam, &sz) != ERROR_SUCCESS)
            return FALSE;
    }
    for (wchar_t *c = steam; *c; c++) if (*c == L'/') *c = L'\\';
    if (TryLibrary(steam, out)) return TRUE;

    // Parse libraryfolders.vdf: lines like  "path"		"D:\\SteamLibrary"
    wchar_t vdf[MAX_PATH]; swprintf_s(vdf, MAX_PATH, L"%s\\steamapps\\libraryfolders.vdf", steam);
    FILE *f; if (_wfopen_s(&f, vdf, L"rb")) return FALSE;
    char line[2048];
    while (fgets(line, sizeof line, f)) {
        char *k = strstr(line, "\"path\""); if (!k) continue;
        char *a = strchr(k + 6, '"'); if (!a) continue; a++;
        char val[1024]; int n = 0;
        for (; *a && *a != '"' && n < 1023; a++) { if (*a == '\\' && a[1] == '\\') a++; val[n++] = *a; }
        val[n] = 0;
        wchar_t lib[MAX_PATH]; MultiByteToWideChar(CP_UTF8, 0, val, -1, lib, MAX_PATH);
        if (TryLibrary(lib, out)) { fclose(f); return TRUE; }
    }
    fclose(f);
    return FALSE;
}

static BOOL AskForGame(wchar_t *out) {
    wchar_t file[MAX_PATH] = L"TimeShift.Exe";
    OPENFILENAMEW of = { sizeof of };
    of.lpstrFilter = L"TimeShift.Exe\0TimeShift.Exe\0All files\0*.*\0";
    of.lpstrFile = file; of.nMaxFile = MAX_PATH;
    of.lpstrTitle = L"Find TimeShift.Exe (steamapps\\common\\TimeShift\\bin)";
    of.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (!GetOpenFileNameW(&of)) return FALSE;
    wcscpy_s(out, MAX_PATH, file);
    wchar_t *s = wcsrchr(out, L'\\'); if (s) *s = 0;
    return TRUE;
}

static BOOL GameRunning(void) {
    HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); PROCESSENTRY32W e = { sizeof e }; BOOL r = FALSE;
    if (h == INVALID_HANDLE_VALUE) return FALSE;
    for (BOOL ok = Process32FirstW(h, &e); ok; ok = Process32NextW(h, &e))
        if (!_wcsicmp(e.szExeFile, L"TimeShift.Exe")) { r = TRUE; break; }
    CloseHandle(h); return r;
}

int WINAPI wWinMain(HINSTANCE hi, HINSTANCE hp, PWSTR cmd, int show) {
    wchar_t bin[MAX_PATH], dll[MAX_PATH];
    if (!FindGame(bin)) {
        if (Msg(MB_OKCANCEL | MB_ICONQUESTION, L"Could not find TimeShift in your Steam libraries.\n\nClick OK and select TimeShift.Exe manually\n(usually ...\\steamapps\\common\\TimeShift\\bin\\TimeShift.Exe).") != IDOK) return 1;
        if (!AskForGame(bin)) return 1;
    }
    while (GameRunning())
        if (Msg(MB_RETRYCANCEL | MB_ICONWARNING, L"TimeShift is running. Close the game and click Retry.") != IDRETRY) return 1;

    swprintf_s(dll, MAX_PATH, L"%s\\dinput8.dll", bin);
    if (FileExists(dll)) {
        int r = Msg(MB_YESNOCANCEL | MB_ICONQUESTION,
            L"The fix is already installed in:\n%s\n\nYes = reinstall / update\nNo = REMOVE the fix\nCancel = do nothing", bin);
        if (r == IDCANCEL) return 0;
        if (r == IDNO) {
            if (DeleteFileW(dll)) Msg(MB_OK | MB_ICONINFORMATION, L"Fix removed.");
            else Msg(MB_OK | MB_ICONERROR, L"Could not delete:\n%s\n(error %lu)", dll, GetLastError());
            return 0;
        }
    }

    HRSRC rs = FindResourceW(hi, MAKEINTRESOURCEW(IDR_DLL), RT_RCDATA);
    HGLOBAL rg = rs ? LoadResource(hi, rs) : NULL;
    const void *data = rg ? LockResource(rg) : NULL; DWORD size = rs ? SizeofResource(hi, rs) : 0;
    if (!data || !size) { Msg(MB_OK | MB_ICONERROR, L"Internal error: embedded DLL missing."); return 1; }

    HANDLE f = CreateFileW(dll, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    DWORD wr = 0;
    if (f == INVALID_HANDLE_VALUE || !WriteFile(f, data, size, &wr, NULL) || wr != size) {
        DWORD err = GetLastError(); if (f != INVALID_HANDLE_VALUE) CloseHandle(f);
        Msg(MB_OK | MB_ICONERROR, L"Could not write:\n%s\n(error %lu)\n\nTry running the installer as administrator.", dll, err);
        return 1;
    }
    CloseHandle(f);
    Msg(MB_OK | MB_ICONINFORMATION,
        L"Done! The fix is installed in:\n%s\n\nJust start TimeShift from Steam.\n\nTip: if the game crashes in cutscenes at 4K, use 2560x1440.\nTo uninstall, run this installer again.", bin);
    return 0;
}
