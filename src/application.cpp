#include "application.h"
#include <windows.h>
#include <shellapi.h>

namespace screencover {

Application* Application::instance_ = nullptr;

Application::Application()
    : hwnd_(nullptr)
    , running_(true)
    , isBlackoutActive_(false)
    , currentMode_(nullptr)
{
    instance_ = this;
}

Application::~Application() {
    Shutdown();
    instance_ = nullptr;
}

bool Application::Initialize() {
    // 创建消息窗口
    if (!CreateMessageWindow()) {
        return false;
    }
    
    // 初始化两种模式
    modeOverlay_ = std::make_unique<ModeOverlay>();
    modePowerOff_ = std::make_unique<ModePowerOff>();
    
    // 设置软件黑屏模式的 ESC 退出回调
    modeOverlay_->SetExitCallback([this]() {
        OnBlackoutExited();
    });
    
    // 默认使用软件模式（笑脸小狗）
    currentMode_ = modeOverlay_.get();
    
    // 初始化序列检测器
    detector_ = std::make_unique<SequenceDetector>(500);
    if (!detector_->Initialize()) {
        return false;
    }
    
    // 设置回调：使用 PostMessage 避免在钩子回调中阻塞
    detector_->SetCallback([this]() {
        OnSequenceTriggered();
    });
    
    // 初始化托盘图标
    trayIcon_ = std::make_unique<TrayIconManager>();
    
    // 使用动态创建的图标
    bool isSmileMode = (currentMode_->GetType() == BlackoutModeType::OVERLAY);
    if (!trayIcon_->Initialize(hwnd_, isSmileMode, L"ScreenCover - 软件黑屏模式")) {
        // 托盘初始化失败不是致命的
    }
    
    trayIcon_->SetMenuCallback([this](int cmd) {
        HandleMenuCommand(cmd);
    });
    
    // 显示启动通知
    trayIcon_->ShowBalloon(
        L"ScreenCover 已启动",
        L"当前模式: 软件黑屏\n热键: 按住 Ctrl 按 3 次 6\n右键图标切换模式",
        5000
    );
    
    return true;
}

void Application::Run() {
    MSG msg;
    while (running_) {
        // 使用 GetMessage 等待消息（更高效）
        BOOL ret = GetMessageW(&msg, nullptr, 0, 0);
        if (ret == 0 || ret == -1) {
            // WM_QUIT 或错误
            running_ = false;
            break;
        }
        
        // 检查是否是托盘消息
        if (trayIcon_ && trayIcon_->HandleMessage(msg.message, msg.wParam, msg.lParam)) {
            continue;
        }
        
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

void Application::Shutdown() {
    // 先退出黑屏状态
    if (isBlackoutActive_ && currentMode_) {
        currentMode_->Exit();
    }
    
    // 清理资源
    trayIcon_.reset();
    detector_.reset();
    modePowerOff_.reset();
    modeOverlay_.reset();
    
    // 销毁窗口
    if (hwnd_) {
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }
    
    running_ = false;
}

void Application::ToggleBlackout() {
    if (!currentMode_) {
        return;
    }
    
    if (isBlackoutActive_) {
        // 退出黑屏
        currentMode_->Exit();
        isBlackoutActive_ = false;
        
        // 恢复托盘图标
        bool isSmileMode = (currentMode_->GetType() == BlackoutModeType::OVERLAY);
        trayIcon_->UpdateIcon(isSmileMode);
        trayIcon_->UpdateTooltip(
            std::wstring(L"ScreenCover - ") + currentMode_->GetName() + L" - 监控中"
        );
    } else {
        // 进入黑屏
        if (currentMode_->Enter()) {
            isBlackoutActive_ = true;
            
            // 更新托盘提示
            trayIcon_->UpdateTooltip(
                std::wstring(L"ScreenCover - ") + currentMode_->GetName() + L" - 黑屏中"
            );
        }
    }
}

void Application::OnBlackoutExited() {
    // 外部（如 ESC 键）退出黑屏时调用
    isBlackoutActive_ = false;
    
    // 更新托盘图标
    bool isSmileMode = (currentMode_->GetType() == BlackoutModeType::OVERLAY);
    trayIcon_->UpdateIcon(isSmileMode);
    trayIcon_->UpdateTooltip(
        std::wstring(L"ScreenCover - ") + currentMode_->GetName() + L" - 监控中"
    );
}

void Application::SwitchMode() {
    // 如果当前正在黑屏，先退出
    if (isBlackoutActive_) {
        ToggleBlackout();
    }
    
    // 切换模式
    if (currentMode_ == modeOverlay_.get()) {
        currentMode_ = modePowerOff_.get();
    } else {
        currentMode_ = modeOverlay_.get();
    }
    
    // 更新托盘图标
    if (trayIcon_) {
        bool isSmileMode = (currentMode_->GetType() == BlackoutModeType::OVERLAY);
        trayIcon_->UpdateIcon(isSmileMode);
        trayIcon_->UpdateTooltip(
            std::wstring(L"ScreenCover - ") + currentMode_->GetName() + L" - 监控中"
        );
        
        // 显示模式切换通知
        std::wstring msg = L"已切换到: " + currentMode_->GetName();
        trayIcon_->ShowBalloon(L"模式切换", msg.c_str(), 3000);
    }
}

BlackoutMode* Application::GetCurrentMode() const {
    return currentMode_;
}

bool Application::IsBlackoutActive() const {
    return isBlackoutActive_;
}

void Application::HandleMenuCommand(int cmd) {
    switch (cmd) {
        case TrayIconManager::MENU_SWITCH_MODE:
            SwitchMode();
            break;
            
        case TrayIconManager::MENU_EXIT:
            running_ = false;
            PostQuitMessage(0);
            break;
    }
}

void Application::OnSequenceTriggered() {
    // 使用 PostMessage 发送到主窗口，避免在钩子回调中阻塞
    if (hwnd_) {
        PostMessageW(hwnd_, WM_SC_TOGGLE_BLACKOUT, 0, 0);
    }
}

bool Application::CreateMessageWindow() {
    const wchar_t* CLASS_NAME = L"ScreenCoverMessageWindow";
    
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = CLASS_NAME;
    
    RegisterClassExW(&wc);
    
    hwnd_ = CreateWindowExW(
        0,
        CLASS_NAME,
        L"ScreenCover",
        0,
        0, 0, 0, 0,
        HWND_MESSAGE,  // 消息-only窗口
        nullptr,
        GetModuleHandle(nullptr),
        nullptr
    );
    
    if (!hwnd_) {
        return false;
    }
    
    // 将 this 指针存储在窗口数据中
    SetWindowLongPtr(hwnd_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    
    return true;
}

LRESULT CALLBACK Application::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // 获取 Application 实例
    Application* app = reinterpret_cast<Application*>(
        GetWindowLongPtr(hwnd, GWLP_USERDATA)
    );
    
    switch (msg) {
        case WM_SC_TOGGLE_BLACKOUT:
            // 处理黑屏切换消息
            if (app) {
                app->ToggleBlackout();
            }
            return 0;
            
        case WM_SC_EXIT_BLACKOUT:
            // 处理退出黑屏消息
            if (app && app->isBlackoutActive_) {
                app->OnBlackoutExited();
            }
            return 0;
            
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
            
        default:
            break;
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

} // namespace screencover
