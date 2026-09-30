#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#include <functional>
#include <string>

class WebViewHost {
public:
    using MessageCallback = std::function<void(const std::wstring&)>;

    WebViewHost();
    ~WebViewHost();

    WebViewHost(const WebViewHost&) = delete;
    WebViewHost& operator=(const WebViewHost&) = delete;

    void Initialize(HWND parent, const std::wstring& distRoot,
                    const std::wstring& entryFile, MessageCallback onMessage);
    void Resize(const RECT& bounds, int bottomCornerRadius);
    bool IsReady() const;

private:
    class Impl;
    Impl* impl_ = nullptr;
};
