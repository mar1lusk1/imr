#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <tchar.h>

#if (!defined(UNICODE))
#	define UNICODE
#endif

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "comctl32.lib")

#define ID_EDIT     101
#define ID_BUTTON   102

HWND hEdit;

// Get current page file size in MiB
DWORD GetCurrentPageFileSizeMiB() {
    MEMORYSTATUSEX mem;
    mem.dwLength = sizeof(mem);
    if (GlobalMemoryStatusEx(&mem)) {
        return (DWORD)(mem.ullTotalPageFile / (1024 * 1024));
    }
    return 4096;
}

// Set page file size via registry
BOOL SetPageFileSize(DWORD sizeMiB) {
    HKEY hKey;
    LONG res = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
        L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management",
        0, KEY_SET_VALUE, &hKey);
    if (res != ERROR_SUCCESS) return FALSE;

    WCHAR data[128];
    swprintf(data, 128, L"C:\\pagefile.sys %lu %lu", sizeMiB, sizeMiB);

    // MULTI_SZ requires two nulls at the end
    WCHAR multi[130] = { 0 };
    lstrcpy(multi, data);
    multi[lstrlen(data) + 1] = 0;

    res = RegSetValueEx(hKey, L"PagingFiles", 0, REG_MULTI_SZ,
                        (BYTE*)multi, (lstrlen(data) + 2) * sizeof(WCHAR));

    RegCloseKey(hKey);
    return res == ERROR_SUCCESS;
}

// Window procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        DWORD currentSize = GetCurrentPageFileSizeMiB();
        WCHAR buffer[32] = {0};
        swprintf(buffer, sizeof(buffer), L"%lu", currentSize);

        CreateWindowW(L"STATIC", L"Page File Size (MiB):", WS_VISIBLE | WS_CHILD,
            10, 10, 200, 20, hwnd, NULL, NULL, NULL);

        hEdit = CreateWindowW(L"EDIT", buffer, WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER,
            10, 35, 200, 25, hwnd, (HMENU)ID_EDIT, NULL, NULL);

        CreateWindowW(L"BUTTON", L"Apply", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            220, 35, 80, 25, hwnd, (HMENU)ID_BUTTON, NULL, NULL);
        break;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == ID_BUTTON) {
            WCHAR buf[32];
            GetWindowTextW(hEdit, buf, 32);
            DWORD newSize = _wtoi(buf);

            if (newSize < 256 || newSize > 65536) {
                MessageBoxW(hwnd, L"Enter a value between 256 and 65536 MiB.", L"Invalid", MB_OK | MB_ICONWARNING);
                return 0;
            }

            if (SetPageFileSize(newSize)) {
                MessageBoxW(hwnd, L"Page file size set. Reboot required.", L"Success", MB_OK | MB_ICONINFORMATION);
            } else {
                MessageBoxW(hwnd, L"Failed to set page file size. Run as administrator.", L"Error", MB_OK | MB_ICONERROR);
            }
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE hPrev, PWSTR lpCmdLine, int nCmdShow) {
    INITCOMMONCONTROLSEX icex = { sizeof(INITCOMMONCONTROLSEX), ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icex);

    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = L"PageFileWndClass";

    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(0, wc.lpszClassName, L"Page File Size Setter",
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 330, 130,
        NULL, NULL, hInst, NULL);

    if (!hwnd) return 1;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return 0;
}

