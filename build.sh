#!/bin/bash
# ScreenCover 构建脚本 (MinGW)

echo "=========================================="
echo "  ScreenCover 构建脚本 (MinGW)"
echo "=========================================="
echo ""

# 检查 MinGW
if ! command -v g++ &> /dev/null; then
    echo "[✗] 未找到 g++"
    echo "    请安装 MinGW-w64 并添加到 PATH"
    exit 1
fi

if ! command -v cmake &> /dev/null; then
    echo "[✗] 未找到 CMake"
    echo "    请安装 CMake 并添加到 PATH"
    exit 1
fi

echo "[✓] 找到编译器: $(g++ --version | head -1)"
echo "[✓] 找到 CMake: $(cmake --version | head -1)"

echo ""
echo "=========================================="
echo "  开始构建"
echo "=========================================="
echo ""

# 创建构建目录
mkdir -p build
cd build

# 生成项目
echo "[1/3] 生成 Makefile..."
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
if [ $? -ne 0 ]; then
    echo "[✗] CMake 生成失败"
    exit 1
fi

# 构建
echo "[2/3] 编译 Release 版本..."
cmake --build . --parallel
if [ $? -ne 0 ]; then
    echo "[✗] 编译失败"
    exit 1
fi

echo "[3/3] 构建完成！"
echo ""
echo "输出文件: build/bin/ScreenCover.exe"
echo ""

read -p "按 Enter 键退出..."
