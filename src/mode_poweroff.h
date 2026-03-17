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
    UINT GetTrayIconId() const override;
    
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
    
private:
    std::atomic<bool> active_;
    std::atomic<bool> watchdogEnabled_;
    std::atomic<bool> shouldExit_;
    DWORD watchdogIntervalMs_;
    DWORD maxWakeDurationMs_;
    DWORD powerOffTime_;
    std::thread watchdogThread_;
};

} // namespace screencover
