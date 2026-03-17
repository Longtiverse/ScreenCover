# ScreenCover - 屏幕覆盖工具

一个轻量级 Windows 桌面程序，按住 `Ctrl` 键连续按下 3 次 `6` 键即可触发全屏黑色覆盖，保护屏幕隐私同时保持系统后台运行。

## ✨ 功能特性

- 🔑 **独特热键**：按住 `Ctrl` 不放，连续按 `6` 键 3 次触发覆盖（500ms内完成）
- 🖥️ **多显示器支持**：自动覆盖所有连接的显示器
- ⬛ **纯黑覆盖**：全屏纯黑色窗口，彻底遮挡屏幕内容
- ⌨️ **ESC退出**：按 `ESC` 键即可退出覆盖状态
- 🚀 **轻量快速**：原生 Win32 API 实现，无额外依赖
- 🔒 **单实例运行**：防止多个实例同时运行

## 📋 系统要求

- Windows 10 / Windows 11
- x64 处理器

## 🔧 编译说明

### 使用 Visual Studio 2022

```bash
# 创建构建目录
mkdir build
cd build

# 生成 Visual Studio 项目
cmake .. -G "Visual Studio 17 2022" -A x64

# 编译
cmake --build . --config Release

# 输出位置
# build\bin\Release\ScreenCover.exe
```

### 使用 MinGW-w64

```bash
# 创建构建目录
mkdir build
cd build

# 生成 Makefile
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release

# 编译
mingw32-make -j4

# 输出位置
# build\bin\ScreenCover.exe
```

## 🚀 使用方法

1. **启动程序**：双击 `ScreenCover.exe` 运行
   - 程序启动后没有界面，在后台运行
   - 可在任务管理器中找到 "ScreenCover" 进程

2. **触发覆盖**：
   - 按住键盘左侧的 `Ctrl` 键不放
   - 连续按下 `6` 键 3 次（小键盘或主键盘区均可）
   - 必须在 500ms 内完成三次按键

3. **退出覆盖**：
   - 按 `ESC` 键退出覆盖状态

4. **关闭程序**：
   - 打开任务管理器
   - 找到 "ScreenCover" 进程
   - 右键选择 "结束任务"

## 📝 热键说明

| 操作 | 说明 |
|------|------|
| `Ctrl` + `6` `6` `6` | 触发/取消全屏覆盖 |
| `ESC` | 退出覆盖状态 |

**注意事项**：
- 必须按住 `Ctrl` 键不放，再按 3 次 `6`
- 三次按键间隔不能超过 500ms
- 支持主键盘区的 `6` 键和小键盘区的 `6` 键

## 🛡️ 安全特性

- **防误触设计**：需要按住 `Ctrl` + 三次 `6`，降低误触发概率
- **程序崩溃保护**：即使程序异常退出，系统 Ctrl+Alt+Del 仍可正常使用
- **无系统钩子残留**：程序退出时自动清理键盘钩子

## 🐛 故障排除

### 程序无法启动

**症状**：双击程序没有反应

**解决方案**：
1. 检查是否有杀毒软件拦截（某些软件可能误报键盘钩子）
2. 尝试以管理员身份运行
3. 检查 Windows 事件查看器中的错误信息

### 热键不工作

**症状**：按 Ctrl+666 没有反应

**解决方案**：
1. 确保按键间隔在 500ms 内
2. 检查是否按住的是 `Ctrl` 键（不是 Alt 或 Shift）
3. 确认程序正在运行（检查任务管理器）
4. 某些游戏或全屏应用可能拦截了按键

### 无法退出覆盖

**症状**：按 ESC 没有反应

**解决方案**：
1. 确保覆盖窗口已获取焦点（点击一下黑色屏幕）
2. 使用 `Ctrl+Alt+Del` 打开安全屏幕，选择任务管理器结束进程
3. 如果以上方法无效，按电源键强制关机

## 📁 项目结构

```
ScreenCover/
├── src/
│   ├── main.cpp              # 程序入口和主循环
│   ├── sequence_detector.h   # Ctrl+666 序列检测器头文件
│   ├── sequence_detector.cpp # Ctrl+666 序列检测器实现
│   ├── overlay_manager.h     # 多显示器覆盖管理器头文件
│   └── overlay_manager.cpp   # 多显示器覆盖管理器实现
├── CMakeLists.txt            # CMake 构建配置
├── README.md                 # 本文件
└── build/                    # 构建输出目录
```

## ⚙️ 技术细节

### 核心实现

- **全局热键**：使用 `SetWindowsHookEx(WH_KEYBOARD_LL)` 低级键盘钩子
- **序列检测**：状态机检测 Ctrl 按住状态下的三连击
- **覆盖窗口**：`WS_EX_TOPMOST` 置顶窗口 + `WS_POPUP` 无边框
- **多显示器**：`EnumDisplayMonitors` 枚举所有显示器

### API 使用

- `SetWindowsHookEx` / `UnhookWindowsHookEx` - 键盘钩子
- `CreateWindowEx` - 创建置顶窗口
- `EnumDisplayMonitors` - 多显示器支持
- `SetWindowPos` - 窗口置顶控制

## 📄 许可证

MIT License

## 🤝 贡献

欢迎提交 Issue 和 Pull Request！

## 📝 更新日志

### v1.0.0
- ✨ 初始版本发布
- 🔑 支持 Ctrl+666 热键触发
- ⬛ 实现全屏黑色覆盖
- ⌨️ 支持 ESC 键退出
- 🖥️ 支持多显示器
