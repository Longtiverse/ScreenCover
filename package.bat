@echo off
chcp 65001 > nul
echo ==========================================
echo   ScreenCover 打包脚本
echo ==========================================
echo.

set "VERSION=2.0.0"
set "BUILD_DIR=build\bin\Release"
set "PACKAGE_NAME=ScreenCover-v%VERSION%-Windows-x64"

REM 检查编译输出
if not exist "%BUILD_DIR%\ScreenCover.exe" (
    echo [✗] 未找到编译输出文件
    echo     请先运行 build.bat 编译程序
    pause
    exit /b 1
)

echo [✓] 找到编译输出

REM 创建打包目录
if exist "%PACKAGE_NAME%" rmdir /s /q "%PACKAGE_NAME%"
mkdir "%PACKAGE_NAME%"

echo [1/4] 复制可执行文件...
copy "%BUILD_DIR%\ScreenCover.exe" "%PACKAGE_NAME%\"

echo [2/4] 复制文档...
copy "README.md" "%PACKAGE_NAME%\"
copy "LICENSE" "%PACKAGE_NAME%\" 2> nul || echo     (无 LICENSE 文件，跳过)

echo [3/4] 创建配置文件...
echo [General] > "%PACKAGE_NAME%\ScreenCover.ini"
echo DefaultMode=overlay >> "%PACKAGE_NAME%\ScreenCover.ini"
echo RememberLastMode=true >> "%PACKAGE_NAME%\ScreenCover.ini"
echo. >> "%PACKAGE_NAME%\ScreenCover.ini"
echo [Hotkeys] >> "%PACKAGE_NAME%\ScreenCover.ini"
echo TriggerKey=6 >> "%PACKAGE_NAME%\ScreenCover.ini"
echo. >> "%PACKAGE_NAME%\ScreenCover.ini"
echo [TrayIcon] >> "%PACKAGE_NAME%\ScreenCover.ini"
echo ShowStartupNotification=true >> "%PACKAGE_NAME%\ScreenCover.ini"

echo [4/4] 压缩打包...
if exist "%PACKAGE_NAME%.zip" del "%PACKAGE_NAME%.zip"

REM 尝试使用 PowerShell 压缩
powershell -Command "Compress-Archive -Path '%PACKAGE_NAME%' -DestinationPath '%PACKAGE_NAME%.zip'" > nul 2>&1

if %errorlevel% equ 0 (
    echo [✓] 打包完成: %PACKAGE_NAME%.zip
    rmdir /s /q "%PACKAGE_NAME%"
) else (
    echo [!] 压缩失败，保留文件夹: %PACKAGE_NAME%
)

echo.
echo ==========================================
echo   打包完成！
echo ==========================================
echo.
echo 发布文件:
if exist "%PACKAGE_NAME%.zip" (
    echo   - %PACKAGE_NAME%.zip
) else (
    echo   - %PACKAGE_NAME%\ (文件夹)
)
echo.

pause
