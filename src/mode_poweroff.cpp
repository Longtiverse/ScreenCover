#include "mode_poweroff.h"
#include <windows.h>

namespace screencover {

ModePowerOff::ModePowerOff()
    : active_(false)
    , watchdogEnabled_(true)
    , shouldExit_(false)
    , watchdogIntervalMs_(500)
    , maxWakeDurationMs_(1000)
    , powerOffTime_(0)
{
}

ModePowerOff::~ModePowerOff() {
    if (active_) {
        Exit();
    }
    
    watchdogEnabled_ = false;
    if (watchdogThread_.joinable()) {
        watchdogThread_.join();
    }
}

bool ModePowerOff::Enter() {
    if (active_) {
        return true;
    }
    
    if (!TurnOffMonitor()) {
        return false;
    }
    
    active_ = true;
    shouldExit_ = false;
    powerOffTime_ = GetTickCount();
    
    // 启动 Watchdog
    if (watchdogEnabled_) {
        if (watchdogThread_.joinable()) {
            watchdogThread_.join();
        }
        watchdogThread_ = std::thread(&ModePowerOff::WatchdogLoop, this);
    }
    
    return true;
}

bool ModePowerOff::Exit() {
    if (!active_) {
        return true;
    }
    
    shouldExit_ = true;
    active_ = false;
    
    // 停止 Watchdog
    watchdogEnabled_ = false;
    if (watchdogThread_.joinable()) {
        watchdogThread_.join();
    }
    
    // 打开显示器
    return TurnOnMonitor();
}

bool ModePowerOff::IsActive() const {
    return active_;
}

std::wstring ModePowerOff::GetName() const {
    return L"硬件断电";
}

BlackoutModeType ModePowerOff::GetType() const {
    return BlackoutModeType::POWEROFF;
}

UINT ModePowerOff::GetTrayIconId() const {
    return 2;  // 不笑小狗图标资源ID
}

void ModePowerOff::SetWatchdogEnabled(bool enabled) {
    watchdogEnabled_ = enabled;
}

void ModePowerOff::SetWatchdogInterval(DWORD intervalMs) {
    watchdogIntervalMs_ = intervalMs;
}

void ModePowerOff::SetMaxWakeDuration(DWORD durationMs) {
    maxWakeDurationMs_ = durationMs;
}

bool ModePowerOff::ShouldExit() const {
    return shouldExit_;
}

void ModePowerOff::WatchdogLoop() {
    while (watchdogEnabled_ && active_) {
        Sleep(watchdogIntervalMs_);
        
        if (!active_ || shouldExit_) {
            break;
        }
        
        // 检查显示器是否被意外唤醒
        if (IsMonitorOn()) {
            DWORD currentTime = GetTickCount();
            DWORD wakeDuration = currentTime - powerOffTime_;
            
            // 如果在允许的最大唤醒时间内，且不是用户主动退出
            if (wakeDuration < maxWakeDurationMs_ && !shouldExit_) {
                // 重新关闭显示器
                TurnOffMonitor();
                powerOffTime_ = GetTickCount();
            }
        }
    }
}

bool ModePowerOff::TurnOffMonitor() {
    // SC_MONITORPOWER:
    // -1 = 开启显示器
    //  1 = 低功耗模式
    //  2 = 关闭显示器
    LRESULT result = SendMessage(
        HWND_BROADCAST,
        WM_SYSCOMMAND,
        SC_MONITORPOWER,
        2
    );
    return (result != 0);
}

bool ModePowerOff::TurnOnMonitor() {
    // 方法1: 发送鼠标移动事件唤醒显示器
    INPUT input = {};
    input.type = INPUT_MOUSE;
    input.mi.dx = 0;
    input.mi.dy = 0;
    input.mi.dwFlags = MOUSEEVENTF_MOVE;
    SendInput(1, &input, sizeof(INPUT));
    
    // 方法2: 发送 SC_MONITORPOWER -1
    SendMessage(HWND_BROADCAST, WM_SYSCOMMAND, SC_MONITORPOWER, -1);
    
    return true;
}

bool ModePowerOff::IsMonitorOn() {
    // 检测显示器是否开启的简单方法：
    // 检查屏幕状态 - 如果显示器关闭，某些 API 可能不可用
    // 这里使用一个简单的方法：尝试获取桌面窗口
    HWND desktop = GetDesktopWindow();
    if (!desktop) {
        return false;
    }
    
    // 另一个方法是检查 GetDevicePowerState，但这需要设备句柄
    // 简化处理：假设显示器通常是被正确控制的
    return true;
}

} // namespace screencover
