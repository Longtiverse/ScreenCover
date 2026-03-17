#include "overlay_manager.h"
#include <string>

namespace screencover {

OverlayManager* OverlayManager::instance_ = nullptr;

OverlayManager::OverlayManager()
    : isVisible_(false)
    , classRegistered_(false)
{
    instance_ = this;
}

OverlayManager::~OverlayManager() {
    Shutdown();
    instance_ = nullptr;
}

bool OverlayManager::Initialize() {
    if (classRegistered_) {
        return true;
    }
    
    if (!RegisterWindowClass()) {
        return false;
    }
    
    if (!CreateOverlayWindows()) {
        UnregisterWindowClass();
        return false;
    }
    
    return true;
}

void OverlayManager::Shutdown() {
    HideOverlay();
    DestroyOverlayWindows();
    UnregisterWindowClass();
}

bool OverlayManager::RegisterWindowClass() {
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    
    if (!RegisterClassExW(&wc)) {
        return false;
    }
    
    classRegistered_ = true;
    return true;
}

void OverlayManager::UnregisterWindowClass() {
    if (classRegistered_) {
        UnregisterClassW(CLASS_NAME, GetModuleHandle(nullptr));
        classRegistered_ = false;
    }
}

BOOL CALLBACK OverlayManager::MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, 
                                               LPRECT lprcMonitor, LPARAM dwData) {
    auto* manager = reinterpret_cast<OverlayManager*>(dwData);
    
    MONITORINFO mi = {};
    mi.cbSize = sizeof(MONITORINFO);
    
    if (GetMonitorInfo(hMonitor, &mi)) {
        OverlayWindow window;
        window.monitor = hMonitor;
        window.rect = mi.rcMonitor;  // 使用整个显示器区域（包括任务栏区域）
        manager->windows_.push_back(window);
    }
    
    return TRUE;  // 继续枚举
}

bool OverlayManager::CreateOverlayWindows() {
    // 枚举所有显示器
    EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc, 
                        reinterpret_cast<LPARAM>(this));
    
    if (windows_.empty()) {
        return false;
    }
    
    HINSTANCE hInstance = GetModuleHandle(nullptr);
    
    // 为每个显示器创建覆盖窗口
    for (auto& window : windows_) {
        HWND hwnd = CreateWindowExW(
            WS_EX_TOPMOST |           // 置顶
            WS_EX_TOOLWINDOW |        // 不在任务栏显示
            WS_EX_NOACTIVATE,         // 不获取焦点
            CLASS_NAME,
            L"ScreenCover",           // 窗口标题（不会显示）
            WS_POPUP |                // 无边框
            WS_VISIBLE,               // 创建时显示
            window.rect.left,
            window.rect.top,
            window.rect.right - window.rect.left,
            window.rect.bottom - window.rect.top,
            nullptr,                  // 无父窗口
            nullptr,                  // 无菜单
            hInstance,
            nullptr
        );
        
        if (hwnd == nullptr) {
            // 创建失败，清理已创建的窗口
            DestroyOverlayWindows();
            return false;
        }
        
        window.hwnd = hwnd;
        
        // 确保窗口在顶层
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
        
        // 隐藏窗口（初始状态）
        ShowWindow(hwnd, SW_HIDE);
    }
    
    return true;
}

void OverlayManager::DestroyOverlayWindows() {
    for (auto& window : windows_) {
        if (window.hwnd != nullptr) {
            DestroyWindow(window.hwnd);
            window.hwnd = nullptr;
        }
    }
    windows_.clear();
}

void OverlayManager::ShowOverlay() {
    if (isVisible_ || windows_.empty()) {
        return;
    }
    
    for (auto& window : windows_) {
        if (window.hwnd != nullptr) {
            ShowWindow(window.hwnd, SW_SHOW);
            SetWindowPos(window.hwnd, HWND_TOPMOST, 
                        window.rect.left, window.rect.top,
                        window.rect.right - window.rect.left,
                        window.rect.bottom - window.rect.top,
                        SWP_FRAMECHANGED);
            UpdateWindow(window.hwnd);
        }
    }
    
    isVisible_ = true;
    
    // 设置第一个窗口为前台窗口以捕获ESC
    if (!windows_.empty() && windows_[0].hwnd != nullptr) {
        SetForegroundWindow(windows_[0].hwnd);
    }
}

void OverlayManager::HideOverlay() {
    if (!isVisible_) {
        return;
    }
    
    for (auto& window : windows_) {
        if (window.hwnd != nullptr) {
            ShowWindow(window.hwnd, SW_HIDE);
        }
    }
    
    isVisible_ = false;
}

void OverlayManager::ToggleOverlay() {
    if (isVisible_) {
        HideOverlay();
    } else {
        ShowOverlay();
    }
}

void OverlayManager::HandleEscape() {
    HideOverlay();
}

LRESULT CALLBACK OverlayManager::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            // 填充黑色背景
            RECT rect;
            GetClientRect(hwnd, &rect);
            HBRUSH blackBrush = CreateSolidBrush(RGB(0, 0, 0));
            FillRect(hdc, &rect, blackBrush);
            DeleteObject(blackBrush);
            
            EndPaint(hwnd, &ps);
            return 0;
        }
        
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            if (wParam == VK_ESCAPE) {
                // ESC键被按下，隐藏覆盖
                if (instance_ != nullptr) {
                    instance_->HandleEscape();
                }
                return 0;
            }
            break;
            
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
            // 鼠标点击不处理，保持覆盖
            return 0;
            
        case WM_ACTIVATE:
            // 窗口被激活时，确保保持在顶层
            if (wParam != WA_INACTIVE && instance_ != nullptr && instance_->isVisible_) {
                SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                           SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
            }
            break;
            
        case WM_DESTROY:
            return 0;
            
        default:
            break;
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

} // namespace screencover
