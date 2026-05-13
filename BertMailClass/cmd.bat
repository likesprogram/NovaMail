@echo off
chcp 65001
REM =======================================
REM 批处理：将指定目录下所有文件后缀改为 .eml
REM 使用方法: rename_to_eml.bat "C:\path\to\folder"
REM =======================================

set "TARGET_DIR=%~1"

if "%TARGET_DIR%"=="" (
    echo ❌ 请提供目标文件夹路径
    exit /b 1
)

if not exist "%TARGET_DIR%" (
    echo ❌ 目录不存在: %TARGET_DIR%
    exit /b 1
)

REM 遍历目录及子目录下的所有文件
for /r "%TARGET_DIR%" %%F in (*) do (
    REM 忽略目录，只处理文件
    if not "%%~xF"==".eml" (
        REM 安全输出
        echo 正在重命名: "%%F" -> "%%~dpnF.eml"
        ren "%%F" "%%~nF.eml"
    )
)

echo ✅ 所有文件后缀已修改为 .eml
pause
