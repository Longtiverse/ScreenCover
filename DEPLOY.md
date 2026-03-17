# ScreenCover 自动构建指南

## 🚀 使用 GitHub Actions 自动构建 EXE

我已经为您配置了完整的自动构建流程。只需按以下步骤操作，即可自动获得编译好的 EXE 文件。

---

## 📋 操作步骤

### 第 1 步：创建 GitHub 仓库

1. 访问 https://github.com/new
2. 仓库名称填写：`ScreenCover`
3. 选择 **Public**（公开）或 **Private**（私有）
4. 勾选 **Add a README file**
5. 点击 **Create repository**

---

### 第 2 步：上传代码

#### 方法 A：使用 Git 命令行

```bash
# 进入项目目录
cd D:\Project\ScreenCover

# 初始化 git
git init

# 添加远程仓库（替换 YOUR_USERNAME 为您的 GitHub 用户名）
git remote add origin https://github.com/YOUR_USERNAME/ScreenCover.git

# 添加所有文件
git add .

# 提交
git commit -m "Initial commit: ScreenCover v2.0"

# 推送到 GitHub
git branch -M main
git push -u origin main
```

#### 方法 B：直接上传（推荐新手）

1. 打开您的 GitHub 仓库页面
2. 点击 **Add file** → **Upload files**
3. 将 `ScreenCover` 文件夹中的所有文件拖拽到网页中
4. 等待上传完成
5. 点击 **Commit changes**

**注意：确保包含以下文件：**
- `src/` 文件夹（所有 .cpp 和 .h 文件）
- `CMakeLists.txt`
- `.github/workflows/build.yml`
- `README.md`

---

### 第 3 步：触发自动构建

上传代码后，GitHub Actions 会自动开始构建：

1. 在 GitHub 仓库页面，点击 **Actions** 标签
2. 您会看到 "Build ScreenCover" 工作流正在运行
3. 等待约 5-10 分钟（首次构建需要安装依赖）

---

### 第 4 步：下载 EXE

构建完成后：

1. 点击 **Actions** 标签
2. 点击最新的成功构建记录（绿色 ✓）
3. 页面底部找到 **Artifacts** 区域
4. 点击 **ScreenCover-v2.0.0-Release** 下载

下载的文件是 `ScreenCover-v2.0.0-Windows-x64.zip`，包含：
- `ScreenCover.exe` - 主程序（单文件，免安装）
- `ScreenCover.ini` - 配置文件
- `README.md` - 使用说明
- `启动ScreenCover.bat` - 快速启动脚本

---

## 🎯 使用方法

### 解压运行

1. 解压下载的 ZIP 文件
2. 双击 `ScreenCover.exe`
3. 程序在后台运行，托盘会出现小狗图标

### 功能操作

| 操作 | 说明 |
|------|------|
| **触发黑屏** | 按住 `Ctrl`，连续按 `6` 三次 |
| **退出软件黑屏** | 按 `ESC` 键 |
| **退出硬件断电** | 右键托盘图标 → 恢复显示 |
| **切换模式** | 右键托盘图标 → 切换模式 |

---

## 🔄 更新代码后重新构建

如果您修改了代码，只需：

```bash
git add .
git commit -m "Update: xxx"
git push
```

GitHub Actions 会自动重新构建，您可以在 Actions 页面下载最新的 EXE。

---

## ❓ 常见问题

### Q: 构建失败了怎么办？

A: 查看 Actions 页面的错误日志，常见原因：
- 代码语法错误
- 缺少必要的文件
- 文件编码问题（确保使用 UTF-8）

### Q: 可以私有仓库吗？

A: 可以！GitHub Actions 对私有仓库也免费，但有每月使用限额。

### Q: 需要安装 Visual Studio 吗？

A: 不需要！GitHub 的服务器已经预装了所有编译工具。

### Q: 构建好的 EXE 在哪里下载？

A: 在 Actions 页面 → 点击成功构建 → Artifacts 区域下载。

---

## 📞 需要帮助？

如果在操作过程中遇到问题：

1. 检查是否正确上传了所有文件
2. 查看 Actions 页面的错误日志
3. 确保仓库是 Public 或您有 GitHub Pro 订阅

---

## ✅ 快速检查清单

构建前请确认：

- [ ] GitHub 仓库已创建
- [ ] 所有代码文件已上传
- [ ] 包含 `.github/workflows/build.yml`
- [ ] Actions 页面显示构建成功
- [ ] 成功下载 `ScreenCover-v2.0.0-Windows-x64.zip`

**完成后，您将获得一个免安装、单文件、可直接运行的 ScreenCover.exe！**
