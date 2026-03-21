#pragma once

#include "mode_base.h"
#include <thread>
#include <atomic>

namespace screencover {

// 硬件断电模式 - 使用 Windows 显示器电源管理
class ModePowerOff : public BlackoutMode {
public:
    ModePowerOff();
    ~ModePowerOff() override;
    
    bool Enter() override;
    bool Exit() override;
    bool IsActive() const override;
    std::wstring GetName() const override;
    BlackoutModeType GetType() const override;
    
    // 输入模式接口
    void SetInputMode(InputModeType mode) override;
    InputModeType GetInputMode() const override;
    void ToggleInputMode() override;
    
    // 设置是否启用 Watchdog
    void SetWatchdogEnabled(bool enabled);
    void SetWatchdogInterval(DWORD intervalMs);
    void SetMaxWakeDuration(DWORD durationMs);
    
    // 检查是否需要退出（供 Watchdog 调用）
    bool ShouldExit() const;
    
private:
    // Watchdog 线程函数
    void WatchdogLoop();
    
    // 关闭显示器
    static bool TurnOffMonitor();
    
    // 打开显示器
    static bool TurnOnMonitor();
    
    // 获取当前显示器电源状态
    static bool IsMonitorOn();
    
    // 安装输入拦截钩子
    void InstallInputHooks();
    
    // 卸载输入拦截钩子
    void RemoveInputHooks();
    
    // 键盘钩子回调
    static LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
    
    // 鼠标钩子回调
    static LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam);
    
private:
    std::atomic<bool> active_;
    std::atomic<bool> watchdogEnabled_;
    std::atomic<bool> shouldExit_;
    DWORD watchdogIntervalMs_;
    DWORD maxWakeDurationMs_;
    DWORD powerOffTime_;
    std::thread watchdogThread_;
    InputModeType inputMode_;
    HHOOK keyboardHook_;
    HHOOK mouseHook_;
    
    // 单例指针（用于钩子回调）
    static ModePowerOff* instance_;
};

} // namespace screencover
