@echo off
chcp 65001 > nul
echo ==========================================
echo   ScreenCover 构建脚本
echo ==========================================
echo.

REM 检查 Visual Studio 2022
set "VS_PATH=D:\Program Files\Microsoft Visual Studio\2022\Professional"
if not exist "%VS_PATH%" (
    set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community"
)
if not exist "%VS_PATH%" (
    set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Professional"
)
if not exist "%VS_PATH%" (
    set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Enterprise"
)

if exist "%VS_PATH%\Common7\Tools\VsDevCmd.bat" (
    echo [✓] 找到 Visual Studio 2022
    call "%VS_PATH%\Common7\Tools\VsDevCmd.bat" -arch=x64
) else (
    echo [✗] 未找到 Visual Studio 2022
    echo     请安装 Visual Studio 2022 或手动运行此脚本
    pause
    exit /b 1
)

REM 检查 CMake
where cmake > nul 2>&1
if %errorlevel% neq 0 (
    echo [✗] 未找到 CMake
    echo     请安装 CMake 并添加到 PATH
    pause
    exit /b 1
)
echo [✓] 找到 CMake

echo.
echo ==========================================
echo   开始构建
echo ==========================================
echo.

REM 创建构建目录
if not exist build mkdir build
cd build

REM 生成项目
echo [1/3] 生成 Visual Studio 项目...
cmake .. -G "Visual Studio 17 2022" -A x64
if %errorlevel% neq 0 (
    echo [✗] CMake 生成失败
    pause
    exit /b 1
)

REM 构建 Release
echo [2/3] 编译 Release 版本...
cmake --build . --config Release
if %errorlevel% neq 0 (
    echo [✗] 编译失败
    pause
    exit /b 1
)

echo [3/3] 构建完成！
echo.
echo 输出文件: build\bin\Release\ScreenCover.exe
echo.

pause
