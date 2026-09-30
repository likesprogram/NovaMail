#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include "ShellApplication.h"

int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE, LPWSTR, int commandShow) {
    return RunNovaMailDesktopShell(instance, commandShow);
}
