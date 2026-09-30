#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#include <memory>

class ShellWindow {
public:
    ShellWindow();
    ~ShellWindow();

    ShellWindow(const ShellWindow&) = delete;
    ShellWindow& operator=(const ShellWindow&) = delete;

    bool Create(HINSTANCE instance);
    void Show(int commandShow);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
