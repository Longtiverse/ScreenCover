#include "mode_overlay.h"

namespace screencover {

ModeOverlay* ModeOverlay::instance_ = nullptr;

ModeOverlay::ModeOverlay()
    : overlay_(std::make_unique<OverlayManager>())
    , inputMode_(InputModeType::FREE)
    , keyboardHook_(nullptr)
    , mouseHook_(nullptr)
{
    instance_ = this;
}

ModeOverlay::~ModeOverlay() {
    if (IsActive()) {
        Exit();
    }
    RemoveInputHooks();
    instance_ = nullptr;
}

bool ModeOverlay::Enter() {
    if (!overlay_) {
        return false;
    }
    
    if (!overlay_->IsInitialized()) {
        if (!overlay_->Initialize()) {
            return false;
        }
    }
    
    overlay_->ShowOverlay();
    
    // 如果是锁定模式，安装输入拦截钩子
    if (inputMode_ == InputModeType::LOCKED) {
        InstallInputHooks();
    }
    
    return true;
}

bool ModeOverlay::Exit() {
    // 先卸载输入拦截钩子
    RemoveInputHooks();
    
    if (overlay_) {
        overlay_->HideOverlay();
    }
    return true;
}

bool ModeOverlay::IsActive() const {
    return overlay_ && overlay_->IsVisible();
}

std::wstring ModeOverlay::GetName() const {
    return L"软件黑屏";
}

BlackoutModeType ModeOverlay::GetType() const {
    return BlackoutModeType::OVERLAY;
}

void ModeOverlay::SetInputMode(InputModeType mode) {
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

InputModeType ModeOverlay::GetInputMode() const {
    return inputMode_;
}

void ModeOverlay::ToggleInputMode() {
    if (inputMode_ == InputModeType::FREE) {
        SetInputMode(InputModeType::LOCKED);
    } else {
        SetInputMode(InputModeType::FREE);
    }
}

void ModeOverlay::SetExitCallback(ExitCallback callback) {
    exitCallback_ = callback;
    // 设置 OverlayManager 的退出回调
    if (overlay_) {
        overlay_->SetEscapeCallback(callback);
    }
}

void ModeOverlay::InstallInputHooks() {
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

void ModeOverlay::RemoveInputHooks() {
    if (keyboardHook_) {
        UnhookWindowsHookEx(keyboardHook_);
        keyboardHook_ = nullptr;
    }
    
    if (mouseHook_) {
        UnhookWindowsHookEx(mouseHook_);
        mouseHook_ = nullptr;
    }
}

LRESULT CALLBACK ModeOverlay::KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (instance_ && nCode == HC_ACTION) {
        // 检查是否是退出热键（ESC）
        if (instance_->IsExitHotkey(wParam, lParam)) {
            // 放行退出热键
            if (instance_->exitCallback_) {
                instance_->exitCallback_();
            }
            return 1;  // 阻断事件，由我们处理
        }
        
        // 阻断其他所有按键
        return 1;
    }
    
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

LRESULT CALLBACK ModeOverlay::MouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (instance_ && nCode == HC_ACTION) {
        // 阻断所有鼠标事件
        return 1;
    }
    
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

bool ModeOverlay::IsExitHotkey(WPARAM wParam, LPARAM lParam) const {
    // 只检查 ESC 键
    if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
        const KBDLLHOOKSTRUCT* kbStruct = reinterpret_cast<const KBDLLHOOKSTRUCT*>(lParam);
        if (kbStruct && kbStruct->vkCode == VK_ESCAPE) {
            return true;
        }
    }
    return false;
}

} // namespace screencover
