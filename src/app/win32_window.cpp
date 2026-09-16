#include "win32_window.h"

#define Rectangle Win32Rectangle
#define CloseWindow Win32CloseWindow
#define ShowCursor Win32ShowCursor
#define LoadImage Win32LoadImage
#define DrawText Win32DrawText
#define DrawTextEx Win32DrawTextEx
#define PlaySound Win32PlaySound

#include <windows.h>

#undef Rectangle
#undef CloseWindow
#undef ShowCursor
#undef LoadImage
#undef DrawText
#undef DrawTextEx
#undef PlaySound

#include <dwmapi.h>
#include <thread>

#include "raylib.h"

#define IDI_APP_ICON 101

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "advapi32.lib")

static bool g_DarkModeSupported = false;
static DWORD g_DwmAttribute = 0;

DWORD GetWindowsBuildNumber() {
    HMODULE hMod = GetModuleHandleW(L"ntdll.dll");
    if (hMod)
    {
        auto fxPtr = reinterpret_cast<LONG(WINAPI*)(POSVERSIONINFOW)>(
            GetProcAddress(hMod, "RtlGetVersion")
        );
        if (fxPtr)
        {
            OSVERSIONINFOW rovi = {};
            rovi.dwOSVersionInfoSize = sizeof(rovi);
            if (fxPtr(&rovi) == 0)
                return rovi.dwBuildNumber;
        }
    }
    return 0;
}

void InitializeDarkModeSupport() {
    DWORD buildNumber = GetWindowsBuildNumber();

    if (buildNumber >= 19041)
    {
        g_DarkModeSupported = true;
        g_DwmAttribute = 20;
    } else if (buildNumber >= 17763) {
        g_DarkModeSupported = true;
        g_DwmAttribute = 19;
    } else {
        g_DarkModeSupported = false;
        g_DwmAttribute = 0;
    }
}

bool IsSystemDarkMode() {
    if (!g_DarkModeSupported)
        return false;

    DWORD value = 1;
    DWORD dataSize = sizeof(value);
    HKEY hKey = nullptr;
    const wchar_t* keyPath = L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize";

    if (RegOpenKeyExW(HKEY_CURRENT_USER, keyPath, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return false;

    LONG result = RegQueryValueExW(hKey, L"AppsUseLightTheme", nullptr, nullptr, 
        reinterpret_cast<LPBYTE>(&value), &dataSize);
    RegCloseKey(hKey);

    if (result != ERROR_SUCCESS)
        return false;

    return value == 0;
}

void ApplyDarkMode(HWND hwnd, bool isDark) {
    if (!g_DarkModeSupported || hwnd == nullptr)
        return;

    BOOL dark = isDark ? TRUE : FALSE;
    DwmSetWindowAttribute(hwnd, g_DwmAttribute, &dark, sizeof(dark));
}

bool IsTopLevelWindow(HWND hwnd) {
    if (hwnd == nullptr)
        return false;

    LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
    return (style & WS_CHILD) == 0;
}

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    DWORD processId = 0;
    GetWindowThreadProcessId(hwnd, &processId);

    if (processId != GetCurrentProcessId())
        return TRUE;

    if (!IsTopLevelWindow(hwnd))
        return TRUE;

    ApplyDarkMode(hwnd, *reinterpret_cast<bool*>(lParam));
    return TRUE;
}

void ApplyToAllProcessWindows(bool isDark) {
    EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&isDark));
}

void CALLBACK WinEventProc(HWINEVENTHOOK, DWORD, HWND hwnd, LONG idObject, LONG, DWORD, DWORD) {
    if (hwnd == nullptr || idObject != OBJID_WINDOW)
        return;

    DWORD processId = 0;
    GetWindowThreadProcessId(hwnd, &processId);

    if (processId != GetCurrentProcessId())
        return;

    if (!IsTopLevelWindow(hwnd))
        return;

    ApplyDarkMode(hwnd, IsSystemDarkMode());
}

void DarkModeWatcherThread()
{
    const wchar_t* keyPath = L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize";
    HKEY hKey = nullptr;

    if (RegOpenKeyExW(HKEY_CURRENT_USER, keyPath, 0, KEY_READ | KEY_NOTIFY, &hKey) != ERROR_SUCCESS)
        return;

    HANDLE hEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (!hEvent)
    {
        RegCloseKey(hKey);
        return;
    }

    HWINEVENTHOOK hHook = 
        SetWinEventHook(EVENT_OBJECT_CREATE, EVENT_OBJECT_SHOW, nullptr, 
            WinEventProc, GetCurrentProcessId(), 0, WINEVENT_OUTOFCONTEXT);

    ApplyToAllProcessWindows(IsSystemDarkMode());

    while (true)
    {
        LONG result = RegNotifyChangeKeyValue(hKey, FALSE, REG_NOTIFY_CHANGE_LAST_SET, hEvent, TRUE);
        if (result != ERROR_SUCCESS)
            break;

        HANDLE handles[] = { hEvent };

        DWORD waitResult = MsgWaitForMultipleObjects(1, handles, FALSE, INFINITE, QS_ALLINPUT);

        if (waitResult == WAIT_OBJECT_0)
        {
            Sleep(50);
            ApplyToAllProcessWindows(IsSystemDarkMode());
        } else if (waitResult == WAIT_OBJECT_0 + 1) {
            MSG msg;

            while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
        } else {
            break;
        }
    }

    if (hHook)
        UnhookWinEvent(hHook);

    CloseHandle(hEvent);
    RegCloseKey(hKey);
}

void startTitleBarThread()
{
    InitializeDarkModeSupport();

    if (!g_DarkModeSupported)
        return;

    std::thread watcher(DarkModeWatcherThread);
    watcher.detach();
}

void setEmbeddedWindowIcon()
{
    HWND hwnd = static_cast<HWND>(GetWindowHandle());
    if (!hwnd)
        return;

    HINSTANCE instance = GetModuleHandleW(nullptr);

    HICON icon = static_cast<HICON>(
        LoadImageW(
            instance,
            MAKEINTRESOURCEW(IDI_APP_ICON),
            IMAGE_ICON,
            0,
            0,
            LR_DEFAULTSIZE
        )
    );

    if (!icon)
        return;

    SendMessageW(hwnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(icon));
    SendMessageW(hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(icon));

    SetClassLongPtrW(
        hwnd,
        GCLP_HICON,
        reinterpret_cast<LONG_PTR>(icon)
    );

    SetClassLongPtrW(
        hwnd,
        GCLP_HICONSM,
        reinterpret_cast<LONG_PTR>(icon)
    );
}