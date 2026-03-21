#include "mode_poweroff.h"
#include <windows.h>

namespace screencover {

ModePowerOff* ModePowerOff::instance_ = nullptr;

ModePowerOff::ModePowerOff()
    : active_(false)
    , watchdogEnabled_(true)
    , shouldExit_(false)
    , watchdogIntervalMs_(500)
    , maxWakeDurationMs_(1000)
    , powerOffTime_(0)
    , inputMode_(InputModeType::FREE)
    , keyboardHook_(nullptr)
    , mouseHook_(nullptr)
{
    instance_ = this;
}

ModePowerOff::~ModePowerOff() {
    if (active_) {
        Exit();
    }
    
    watchdogEnabled_ = false;
    if (watchdogThread_.joinable()) {
        watchdogThread_.join();
    }
    
    RemoveInputHooks();
    instance_ = nullptr;
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
    
    // 如果是锁定模式，安装输入拦截钩子
    if (inputMode_ == InputModeType::LOCKED) {
        InstallInputHooks();
    }
    
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
    
    // 先卸载输入拦截钩子
    RemoveInputHooks();
    
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

void ModePowerOff::SetInputMode(InputModeType mode) {
    inputMode_ = mode;
    
    // 如果当前处于黑屏状态，立即更新钩子
    if (IsActive()) {
        if (mode == InputModeType::LOCKED) {
            InstallInputHooks();
        } else {
            RemoveInputHooks();
        }
    }
}

InputModeType ModePowerOff::GetInputMode() const {
    return inputMode_;
}

void ModePowerOff::ToggleInputMode() {
    if (inputMode_ == InputModeType::FREE) {
        SetInputMode(InputModeType::LOCKED);
    } else {
        SetInputMode(InputModeType::FREE);
    }
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

void ModePowerOff::InstallInputHooks() {
    HINSTANCE hInstance = GetModuleHandle(nullptr);
    
    // 安装键盘钩子
    if (!keyboardHook_) {
        keyboardHook_ = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, hInstance, 0);
    }
    
    // 安装鼠标钩子
    if (!mouseHook_) {
        mouseHook_ = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, hInstance, 0);
    }
}

void ModePowerOff::RemoveInputHooks() {
    if (keyboardHook_) {
        UnhookWindowsHookEx(keyboardHook_);
        keyboardHook_ = nullptr;
    }
    
    if (mouseHook_) {
        UnhookWindowsHookEx(mouseHook_);
        mouseHook_ = nullptr;
    }
}

LRESULT CALLBACK ModePowerOff::KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (instance_ && nCode == HC_ACTION) {
        // 检查是否是 Ctrl+Alt+Del
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            const KBDLLHOOKSTRUCT* kbStruct = reinterpret_cast<const KBDLLHOOKSTRUCT*>(lParam);
            if (kbStruct) {
                // 允许 Ctrl+Alt+Del（系统安全组合键）
                bool ctrlPressed = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
                bool altPressed = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
                
                if (ctrlPressed && altPressed && kbStruct->vkCode == VK_DELETE) {
                    // 放行 Ctrl+Alt+Del
                    return CallNextHookEx(nullptr, nCode, wParam, lParam);
                }
                
                // 阻断其他所有按键
                return 1;
            }
        }
        
        // 阻断其他按键事件
        return 1;
    }
    
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

LRESULT CALLBACK ModePowerOff::MouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (instance_ && nCode == HC_ACTION) {
        // 阻断所有鼠标事件
        return 1;
    }
    
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

} // namespace screencover
