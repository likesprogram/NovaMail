#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shlwapi.h>

#include "ShellDebugLog.h"

#include <string>

#pragma comment(lib, "shlwapi.lib")

namespace {

std::wstring JoinPath(const std::wstring& left, const std::wstring& right) {
    if (left.empty()) {
        return right;
    }

    if (left.back() == L'\\' || left.back() == L'/') {
        return left + right;
    }

    return left + L"\\" + right;
}

bool EnsureDirectoryExists(const std::wstring& path) {
    if (path.empty() || PathFileExistsW(path.c_str())) {
        return true;
    }

    const size_t slash = path.find_last_of(L"\\/");
    if (slash != std::wstring::npos) {
        if (!EnsureDirectoryExists(path.substr(0, slash))) {
            return false;
        }
    }

    return CreateDirectoryW(path.c_str(), nullptr) || GetLastError() == ERROR_ALREADY_EXISTS;
}

std::wstring BuildLogPath() {
    wchar_t localAppData[MAX_PATH] = {};
    DWORD length = GetEnvironmentVariableW(L"LOCALAPPDATA", localAppData, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) {
        length = GetTempPathW(MAX_PATH, localAppData);
        if (length == 0 || length >= MAX_PATH) {
            return L"";
        }
    }

    return JoinPath(localAppData, L"NovaMailDesktop\\shell.log");
}

std::wstring FormatLastError(DWORD error) {
    if (error == ERROR_SUCCESS) {
        return L"0";
    }

    wchar_t* buffer = nullptr;
    const DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                        FORMAT_MESSAGE_IGNORE_INSERTS;
    const DWORD length = FormatMessageW(flags, nullptr, error, 0,
                                        reinterpret_cast<wchar_t*>(&buffer), 0, nullptr);
    if (length == 0 || !buffer) {
        return std::to_wstring(error);
    }

    std::wstring message(buffer, length);
    LocalFree(buffer);
    while (!message.empty() && (message.back() == L'\r' || message.back() == L'\n')) {
        message.pop_back();
    }
    return std::to_wstring(error) + L" " + message;
}

}  // namespace

void WriteShellDebugLog(const std::wstring& message) {
    const std::wstring logPath = BuildLogPath();
    if (logPath.empty()) {
        return;
    }

    const size_t slash = logPath.find_last_of(L"\\/");
    if (slash != std::wstring::npos) {
        EnsureDirectoryExists(logPath.substr(0, slash));
    }

    HANDLE file = CreateFileW(logPath.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ,
                              nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return;
    }

    SYSTEMTIME time = {};
    GetLocalTime(&time);
    std::wstring line = L"[";
    line += std::to_wstring(time.wYear) + L"-";
    line += std::to_wstring(time.wMonth) + L"-";
    line += std::to_wstring(time.wDay) + L" ";
    line += std::to_wstring(time.wHour) + L":";
    line += std::to_wstring(time.wMinute) + L":";
    line += std::to_wstring(time.wSecond) + L"] ";
    line += message;
    line += L"\r\n";

    DWORD written = 0;
    WriteFile(file, line.data(), static_cast<DWORD>(line.size() * sizeof(wchar_t)),
              &written, nullptr);
    CloseHandle(file);
}

