#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#include <string>

class VueDistResourceExtractor {
public:
    explicit VueDistResourceExtractor(HINSTANCE instance);

    bool Extract();
    const std::wstring& OutputRoot() const;
    const std::wstring& EntryFilePath() const;
    std::wstring EntryFileName() const;

private:
    bool ExtractResourceFile(int resourceId, const std::wstring& relativePath) const;
    std::wstring BuildOutputRoot() const;

    HINSTANCE instance_ = nullptr;
    std::wstring outputRoot_;
    std::wstring entryFilePath_;
};
