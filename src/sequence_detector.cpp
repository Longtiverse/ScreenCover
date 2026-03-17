#include "sequence_detector.h"
#include <chrono>

namespace screencover {

SequenceDetector* SequenceDetector::instance_ = nullptr;

SequenceDetector::SequenceDetector(DWORD timeoutMs)
    : hookHandle_(nullptr)
    , currentState_(SequenceState::IDLE)
    , timeoutMs_(timeoutMs)
    , lastKeyTime_(0)
    , sixCount_(0)
    , callback_(nullptr)
{
    instance_ = this;
}

SequenceDetector::~SequenceDetector() {
    Shutdown();
    instance_ = nullptr;
}

bool SequenceDetector::Initialize() {
    if (hookHandle_ != nullptr) {
        return true;  // 已经初始化
    }
    
    // 安装低级键盘钩子
    HINSTANCE hInstance = GetModuleHandle(nullptr);
    hookHandle_ = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, hInstance, 0);
    
    if (hookHandle_ == nullptr) {
        return false;
    }
    
    return true;
}

void SequenceDetector::Shutdown() {
    if (hookHandle_ != nullptr) {
        UnhookWindowsHookEx(hookHandle_);
        hookHandle_ = nullptr;
    }
    
    ResetState();
}

void SequenceDetector::SetCallback(TriggerCallback callback) {
    callback_ = callback;
}

LRESULT CALLBACK SequenceDetector::KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (instance_ != nullptr && nCode == HC_ACTION) {
        const KBDLLHOOKSTRUCT* kbStruct = reinterpret_cast<const KBDLLHOOKSTRUCT*>(lParam);
        instance_->HandleKeyboardEvent(wParam, kbStruct);
    }
    
    // 必须调用下一个钩子
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

void SequenceDetector::HandleKeyboardEvent(WPARAM wParam, const KBDLLHOOKSTRUCT* kbStruct) {
    // 只处理按键按下事件
    if (wParam != WM_KEYDOWN && wParam != WM_SYSKEYDOWN) {
        return;
    }
    
    DWORD vkCode = kbStruct->vkCode;
    DWORD currentTime = GetTickCount();
    
    // 检查Ctrl键状态
    bool ctrlPressed = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
    
    // 检查是否是6键（主键盘区或小键盘区）
    bool isSix = (vkCode == '6') || (vkCode == VK_NUMPAD6);
    
    // 状态机处理
    switch (currentState_) {
        case SequenceState::IDLE:
            // 等待Ctrl按下
            if (ctrlPressed) {
                TransitionTo(SequenceState::CTRL_HELD);
            }
            break;
            
        case SequenceState::CTRL_HELD:
            // Ctrl已按住，等待第一个6
            if (!ctrlPressed) {
                // Ctrl被释放，重置
                ResetState();
            } else if (isSix) {
                // 检测到第一个6
                lastKeyTime_ = currentTime;
                sixCount_ = 1;
                TransitionTo(SequenceState::FIRST_SIX);
            }
            break;
            
        case SequenceState::FIRST_SIX:
            // 等待第二个6
            if (!ctrlPressed) {
                ResetState();
            } else if (IsTimeout()) {
                ResetState();
            } else if (isSix) {
                // 检测到第二个6
                lastKeyTime_ = currentTime;
                sixCount_ = 2;
                TransitionTo(SequenceState::SECOND_SIX);
            }
            break;
            
        case SequenceState::SECOND_SIX:
            // 等待第三个6
            if (!ctrlPressed) {
                ResetState();
            } else if (IsTimeout()) {
                ResetState();
            } else if (isSix) {
                // 检测到第三个6！触发！
                sixCount_ = 3;
                TransitionTo(SequenceState::TRIGGERED);
                Trigger();
            }
            break;
            
        case SequenceState::TRIGGERED:
            // 触发后等待Ctrl释放，然后重置
            if (!ctrlPressed) {
                ResetState();
            }
            break;
    }
}

void SequenceDetector::TransitionTo(SequenceState newState) {
    currentState_ = newState;
}

bool SequenceDetector::IsTimeout() const {
    if (lastKeyTime_ == 0) {
        return false;
    }
    
    DWORD currentTime = GetTickCount();
    DWORD elapsed = currentTime - lastKeyTime_;
    
    return elapsed > timeoutMs_;
}

void SequenceDetector::ResetState() {
    currentState_ = SequenceState::IDLE;
    lastKeyTime_ = 0;
    sixCount_ = 0;
}

void SequenceDetector::Trigger() {
    if (callback_) {
        callback_();
    }
}

} // namespace screencover
