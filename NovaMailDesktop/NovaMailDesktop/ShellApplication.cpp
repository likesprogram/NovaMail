#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <objidl.h>
#include <olectl.h>
#include <gdiplus.h>

#include "ShellApplication.h"
#include "ShellDebugLog.h"
#include "ShellWindow.h"

#include <string>

#pragma comment(lib, "gdiplus.lib")

namespace {

void EnableDpiAwareness() {
    HMODULE user32 = LoadLibraryW(L"user32.dll");
    if (!user32) {
        return;
    }

    using SetDpiAwarenessContextFn = BOOL(WINAPI*)(DPI_AWARENESS_CONTEXT);
    auto* setDpiAwarenessContext = reinterpret_cast<SetDpiAwarenessContextFn>(
        GetProcAddress(user32, "SetProcessDpiAwarenessContext"));

    if (setDpiAwarenessContext) {
        setDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    } else {
        SetProcessDPIAware();
    }

    FreeLibrary(user32);
}

}  // namespace

int RunNovaMailDesktopShell(HINSTANCE instance, int commandShow) {
    WriteShellDebugLog(L"application start");
    EnableDpiAwareness();

    const HRESULT comResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(comResult) && comResult != RPC_E_CHANGED_MODE) {
        WriteShellDebugLog(L"CoInitializeEx failed: " + std::to_wstring(comResult));
        return 1;
    }

    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken = 0;
    if (Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput,
                                nullptr) != Gdiplus::Ok) {
        WriteShellDebugLog(L"GdiplusStartup failed");
        if (SUCCEEDED(comResult)) {
            CoUninitialize();
        }
        return 1;
    }

    int exitCode = 0;

    {
        ShellWindow shellWindow;
        if (!shellWindow.Create(instance)) {
            WriteShellDebugLog(L"shell window create failed");
            exitCode = 1;
        } else {
            WriteShellDebugLog(L"shell window created");
            shellWindow.Show(commandShow);

            MSG message = {};
            while (GetMessageW(&message, nullptr, 0, 0) > 0) {
                TranslateMessage(&message);
                DispatchMessageW(&message);
            }

            exitCode = static_cast<int>(message.wParam);
        }
    }

    Gdiplus::GdiplusShutdown(gdiplusToken);
    if (SUCCEEDED(comResult)) {
        CoUninitialize();
    }
    WriteShellDebugLog(L"application exit: " + std::to_wstring(exitCode));
    return exitCode;
}
