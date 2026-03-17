#include "tray_icon.h"

namespace screencover {

TrayIconManager::TrayIconManager()
    : initialized_(false)
    , hwnd_(nullptr)
    , iconId_(0)
    , trayMessage_(0)
    , menuCallback_(nullptr)
{
    ZeroMemory(&nid_, sizeof(nid_));
}

TrayIconManager::~TrayIconManager() {
    Shutdown();
}

bool TrayIconManager::Initialize(HWND hwnd, UINT iconResId, const std::wstring& tooltip) {
    if (initialized_) {
        return true;
    }
    
    hwnd_ = hwnd;
    iconId_ = iconResId;
    
    // 注册自定义托盘消息
    trayMessage_ = RegisterWindowMessageW(L"ScreenCoverTrayMessage");
    
    // 初始化 NOTIFYICONDATA
    nid_.cbSize = sizeof(NOTIFYICONDATAW);
    nid_.hWnd = hwnd_;
    nid_.uID = 1;
    nid_.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid_.uCallbackMessage = trayMessage_;
    nid_.hIcon = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(iconResId));
    wcsncpy_s(nid_.szTip, tooltip.c_str(), _TRUNCATE);
    
    // 添加托盘图标
    if (!Shell_NotifyIconW(NIM_ADD, &nid_)) {
        return false;
    }
    
    initialized_ = true;
    return true;
}

void TrayIconManager::Shutdown() {
    if (!initialized_) {
        return;
    }
    
    Shell_NotifyIconW(NIM_DELETE, &nid_);
    initialized_ = false;
}

bool TrayIconManager::UpdateIcon(UINT iconResId) {
    if (!initialized_) {
        return false;
    }
    
    nid_.hIcon = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(iconResId));
    nid_.uFlags = NIF_ICON;
    
    return Shell_NotifyIconW(NIM_MODIFY, &nid_);
}

bool TrayIconManager::UpdateTooltip(const std::wstring& tooltip) {
    if (!initialized_) {
        return false;
    }
    
    wcsncpy_s(nid_.szTip, tooltip.c_str(), _TRUNCATE);
    nid_.uFlags = NIF_TIP;
    
    return Shell_NotifyIconW(NIM_MODIFY, &nid_);
}

bool TrayIconManager::ShowBalloon(const std::wstring& title, const std::wstring& text, DWORD timeout) {
    if (!initialized_) {
        return false;
    }
    
    nid_.uFlags = NIF_INFO;
    wcsncpy_s(nid_.szInfo, text.c_str(), _TRUNCATE);
    wcsncpy_s(nid_.szInfoTitle, title.c_str(), _TRUNCATE);
    nid_.dwInfoFlags = NIIF_INFO;
    nid_.uTimeout = timeout;
    
    return Shell_NotifyIconW(NIM_MODIFY, &nid_);
}

void TrayIconManager::SetMenuCallback(MenuCallback callback) {
    menuCallback_ = callback;
}

void TrayIconManager::ShowContextMenu() {
    if (!initialized_) {
        return;
    }
    
    HMENU menu = CreatePopupMenu();
    if (!menu) {
        return;
    }
    
    // 添加菜单项
    // 注意：这里需要根据当前模式动态生成菜单
    // 这部分由调用者通过 menuCallback_ 处理
    
    // 显示菜单
    POINT pt;
    GetCursorPos(&pt);
    SetForegroundWindow(hwnd_);
    
    int cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_NONOTIFY, 
                            pt.x, pt.y, 0, hwnd_, nullptr);
    
    DestroyMenu(menu);
    
    if (menuCallback_ && cmd != 0) {
        menuCallback_(cmd);
    }
}

bool TrayIconManager::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    if (!initialized_ || msg != trayMessage_) {
        return false;
    }
    
    switch (lParam) {
        case WM_RBUTTONUP:
        case WM_CONTEXTMENU:
            ShowContextMenu();
            return true;
            
        case WM_LBUTTONDBLCLK:
            // 双击托盘图标 - 可以触发某种操作
            if (menuCallback_) {
                menuCallback_(MENU_SWITCH_MODE);
            }
            return true;
    }
    
    return false;
}

void TrayIconManager::AddMenuItem(HMENU menu, int id, const std::wstring& text, bool checked) {
    UINT flags = MF_STRING;
    if (checked) {
        flags |= MF_CHECKED;
    }
    AppendMenuW(menu, flags, id, text.c_str());
}

} // namespace screencover
