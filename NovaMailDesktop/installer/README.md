# NovaMailDesktop Installer

This directory contains the Inno Setup script used to build the Windows x64
installer.

## Prerequisites

1. Visual Studio 2022 or Build Tools with the C++ desktop workload.
2. Node.js and restored dependencies under `VueShell`.
3. Inno Setup 6 with `ISCC.exe` available on `PATH` or in its default install
   location.

## Build

Run this from the repository root:

```powershell
.\tools\build-installer.ps1 -Version 1.0.0
```

The installer is written to:

```text
dist\installer\NovaMailDesktop-Setup-1.0.0-x64.exe
```

## WebView2 Runtime

The installer checks whether Microsoft Edge WebView2 Runtime is present.

To bundle the official Evergreen Bootstrapper, place it here:

```text
installer\runtime\MicrosoftEdgeWebview2Setup.exe
```

Then run the build script again. If this file is not present, the installer is
still generated and shows a runtime notice when WebView2 is missing.
