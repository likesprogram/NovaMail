#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shlwapi.h>

#include "VueDistResourceExtractor.h"
#include "res/vue-dist-resource-map.h"

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

std::wstring ParentPath(const std::wstring& path) {
    const size_t slash = path.find_last_of(L"\\/");
    if (slash == std::wstring::npos) {
        return L"";
    }

    return path.substr(0, slash);
}

}  // namespace

VueDistResourceExtractor::VueDistResourceExtractor(HINSTANCE instance)
    : instance_(instance) {}

bool VueDistResourceExtractor::Extract() {
    outputRoot_ = BuildOutputRoot();
    if (outputRoot_.empty() || !EnsureDirectoryExists(outputRoot_)) {
        return false;
    }

    for (const auto& resource : NovaMailVueDist::kResources) {
        if (!ExtractResourceFile(resource.id, resource.path)) {
            return false;
        }
    }

    entryFilePath_ = JoinPath(outputRoot_, NovaMailVueDist::kEntryFile);
    return PathFileExistsW(entryFilePath_.c_str()) == TRUE;
}

const std::wstring& VueDistResourceExtractor::EntryFilePath() const {
    return entryFilePath_;
}

const std::wstring& VueDistResourceExtractor::OutputRoot() const {
    return outputRoot_;
}

std::wstring VueDistResourceExtractor::EntryFileName() const {
    return NovaMailVueDist::kEntryFile;
}

bool VueDistResourceExtractor::ExtractResourceFile(
    int resourceId, const std::wstring& relativePath) const {
    HRSRC resource = FindResourceW(instance_, MAKEINTRESOURCEW(resourceId), RT_RCDATA);
    if (!resource) {
        return false;
    }

    HGLOBAL loadedResource = LoadResource(instance_, resource);
    if (!loadedResource) {
        return false;
    }

    const void* data = LockResource(loadedResource);
    const DWORD size = SizeofResource(instance_, resource);
    if (!data || size == 0) {
        return false;
    }

    const std::wstring outputPath = JoinPath(outputRoot_, relativePath);
    if (!EnsureDirectoryExists(ParentPath(outputPath))) {
        return false;
    }

    HANDLE file = CreateFileW(outputPath.c_str(), GENERIC_WRITE, 0, nullptr,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD written = 0;
    const BOOL ok = WriteFile(file, data, size, &written, nullptr);
    CloseHandle(file);
    return ok && written == size;
}

std::wstring VueDistResourceExtractor::BuildOutputRoot() const {
    wchar_t localAppData[MAX_PATH] = {};
    DWORD length = GetEnvironmentVariableW(L"LOCALAPPDATA", localAppData, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) {
        length = GetTempPathW(MAX_PATH, localAppData);
        if (length == 0 || length >= MAX_PATH) {
            return L"";
        }
    }

    return JoinPath(localAppData, L"NovaMailDesktop\\vue-dist");
}
