[CmdletBinding()]
param(
    [string]$Version = "1.0.0",
    [string]$InnoCompiler = ""
)

$ErrorActionPreference = "Stop"

$workspace = Resolve-Path (Join-Path $PSScriptRoot "..")
$installerScript = Join-Path $workspace "installer\NovaMailDesktop.iss"
$releaseDir = Join-Path $workspace "x64\Release"
$appExe = Join-Path $releaseDir "NovaMailDesktop.exe"
$webViewLoader = Join-Path $releaseDir "WebView2Loader.dll"

function Invoke-Checked {
    param(
        [Parameter(Mandatory = $true)]
        [string]$FilePath,
        [Parameter(ValueFromRemainingArguments = $true)]
        [string[]]$Arguments
    )

    & $FilePath @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed with exit code $LASTEXITCODE`: $FilePath $($Arguments -join ' ')"
    }
}

function Find-InnoCompiler {
    param([string]$ExplicitPath)

    if ($ExplicitPath) {
        if (Test-Path $ExplicitPath) {
            return $ExplicitPath
        }

        throw "The specified Inno Setup compiler does not exist: $ExplicitPath"
    }

    $command = Get-Command ISCC.exe -ErrorAction SilentlyContinue
    if ($command) {
        return $command.Source
    }

    $candidates = @(
        "${env:LOCALAPPDATA}\Programs\Inno Setup 6\ISCC.exe",
        "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe",
        "${env:ProgramFiles}\Inno Setup 6\ISCC.exe"
    )

    foreach ($candidate in $candidates) {
        if ($candidate -and (Test-Path $candidate)) {
            return $candidate
        }
    }

    throw "ISCC.exe was not found. Install Inno Setup 6 first."
}

function Get-InnoLanguageArguments {
    param([string]$InnoCompilerPath)

    $innoDir = Split-Path -Parent $InnoCompilerPath
    $chineseSimplified = Join-Path $innoDir "Languages\ChineseSimplified.isl"
    if (Test-Path $chineseSimplified) {
        return @(
            "/DInstallerLanguageName=chinesesimp",
            "/DInstallerMessagesFile=compiler:Languages\ChineseSimplified.isl"
        )
    }

    return @()
}

Push-Location $workspace
try {
    if (!(Test-Path $appExe)) {
        throw "NovaMailDesktop.exe was not found in the release directory: $appExe"
    }

    if (!(Test-Path $webViewLoader)) {
        throw "WebView2Loader.dll was not found in the release directory: $webViewLoader"
    }

    Write-Host "Using release files: $releaseDir"
    Write-Host "Building installer..."
    $iscc = Find-InnoCompiler -ExplicitPath $InnoCompiler
    $languageArguments = Get-InnoLanguageArguments -InnoCompilerPath $iscc
    Invoke-Checked $iscc "/DAppVersion=$Version" @languageArguments $installerScript

    Write-Host "Installer output: $(Join-Path $workspace "dist\installer")"
}
finally {
    Pop-Location
}
