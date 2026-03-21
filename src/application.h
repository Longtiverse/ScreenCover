#pragma once

#include "mode_base.h"
#include "mode_overlay.h"
#include "mode_poweroff.h"
#include "sequence_detector.h"
#include "tray_icon.h"
#include "settings_manager.h"
#include <memory>

namespace screencover {

// 自定义消息常量
#define WM_SC_TOGGLE_BLACKOUT (WM_USER + 1)
#define WM_SC_EXIT_BLACKOUT   (WM_USER + 2)
#define WM_SC_TOGGLE_INPUT    (WM_USER + 3)

// 应用程序主控制器
class Application {
public:
    Application();
    ~Application();
    
    // 禁止拷贝
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    
    // 初始化
    bool Initialize();
    
    // 运行主循环
    void Run();
    
    // 关闭
    void Shutdown();
    
    // 切换黑屏模式
    void ToggleBlackout();
    
    // 退出黑屏（供外部回调）
    void OnBlackoutExited();
    
    // 切换工作模式（软件/硬件）
    void SwitchMode();
    
    // 切换输入模式（自由/锁定）
    void ToggleInputMode();
    
    // 获取当前模式
    BlackoutMode* GetCurrentMode() const;
    
    // 是否处于黑屏状态
    bool IsBlackoutActive() const;
    
    // 获取窗口句柄（用于发送消息）
    HWND GetHwnd() const { return hwnd_; }
    
    // 菜单处理
    void HandleMenuCommand(int cmd);
    
private:
    // 创建主窗口（用于接收消息）
    bool CreateMessageWindow();
    
    // 窗口过程（静态）
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    
    // 序列触发回调
    void OnSequenceTriggered();
    
    // 更新托盘图标和提示
    void UpdateTrayStatus();
    
private:
    HWND hwnd_;
    bool running_;
    bool isBlackoutActive_;
    
    // 两种模式
    std::unique_ptr<ModeOverlay> modeOverlay_;
    std::unique_ptr<ModePowerOff> modePowerOff_;
    BlackoutMode* currentMode_;
    
    // 热键检测
    std::unique_ptr<SequenceDetector> detector_;
    
    // 托盘图标
    std::unique_ptr<TrayIconManager> trayIcon_;
    
    // 配置管理器
    std::unique_ptr<SettingsManager> settings_;
    
    // 单例实例（用于静态回调）
    static Application* instance_;
};

} // namespace screencover
