#pragma once

#include "mode_base.h"
#include "mode_overlay.h"
#include "mode_poweroff.h"
#include "sequence_detector.h"
#include "tray_icon.h"
#include <memory>

namespace screencover {

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
    
    // 切换工作模式（软件/硬件）
    void SwitchMode();
    
    // 获取当前模式
    BlackoutMode* GetCurrentMode() const;
    
    // 是否处于黑屏状态
    bool IsBlackoutActive() const;
    
    // 消息循环处理
    void ProcessMessages();
    
    // 托盘消息处理
    void HandleTrayMessage(WPARAM wParam, LPARAM lParam);
    
    // 菜单处理
    void HandleMenuCommand(int cmd);
    
private:
    // 创建主窗口（用于接收消息）
    bool CreateMessageWindow();
    
    // 窗口过程（静态）
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    
    // 序列触发回调
    void OnSequenceTriggered();
    
    // 更新托盘提示
    void UpdateTrayTooltip();
    
    // 显示上下文菜单
    void ShowContextMenu();
    
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
    
    // 单例实例（用于静态回调）
    static Application* instance_;
};

} // namespace screencover
