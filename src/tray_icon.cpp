#include "tray_icon.h"

namespace screencover {

TrayIconManager::TrayIconManager()
    : initialized_(false)
    , hwnd_(nullptr)
    , hIcon_(nullptr)
    , trayMessage_(0)
    , menuCallback_(nullptr)
{
    ZeroMemory(&nid_, sizeof(nid_));
}

TrayIconManager::~TrayIconManager() {
    Shutdown();
}

bool TrayIconManager::Initialize(HWND hwnd, bool smile, const std::wstring& tooltip) {
    if (initialized_) {
        return true;
    }
    
    hwnd_ = hwnd;
    
    // 创建图标
    hIcon_ = CreateDogIcon(smile);
    if (!hIcon_) {
        return false;
    }
    
    // 注册自定义托盘消息
    trayMessage_ = RegisterWindowMessageW(L"ScreenCoverTrayMessage");
    
    // 初始化 NOTIFYICONDATA
    nid_.cbSize = sizeof(NOTIFYICONDATAW);
    nid_.hWnd = hwnd_;
    nid_.uID = 1;
    nid_.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid_.uCallbackMessage = trayMessage_;
    nid_.hIcon = hIcon_;
    wcsncpy_s(nid_.szTip, tooltip.c_str(), _TRUNCATE);
    
    // 添加托盘图标
    if (!Shell_NotifyIconW(NIM_ADD, &nid_)) {
        DestroyIcon(hIcon_);
        hIcon_ = nullptr;
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
    
    if (hIcon_) {
        DestroyIcon(hIcon_);
        hIcon_ = nullptr;
    }
    
    initialized_ = false;
}

bool TrayIconManager::UpdateIcon(bool smile) {
    if (!initialized_) {
        return false;
    }
    
    // 删除旧图标
    if (hIcon_) {
        DestroyIcon(hIcon_);
    }
    
    // 创建新图标
    hIcon_ = CreateDogIcon(smile);
    if (!hIcon_) {
        return false;
    }
    
    nid_.hIcon = hIcon_;
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
    AddMenuItem(menu, MENU_SWITCH_MODE, L"切换模式");
    AddMenuItem(menu, MENU_SEPARATOR, L"", false);  // 分隔线
    AddMenuItem(menu, MENU_EXIT, L"退出");
    
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
            // 双击托盘图标 - 触发切换模式
            if (menuCallback_) {
                menuCallback_(MENU_SWITCH_MODE);
            }
            return true;
    }
    
    return false;
}

HICON TrayIconManager::CreateDogIcon(bool smile) {
    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    
    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = 32;
    bmi.bmiHeader.biHeight = 32;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    
    void* bits = NULL;
    HBITMAP hbmColor = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, &bits, NULL, 0);
    HBITMAP hbmMask = CreateBitmap(32, 32, 1, 1, NULL);
    
    SelectObject(hdcMem, hbmColor);
    
    // 清透明
    RECT rc = {0, 0, 32, 32};
    FillRect(hdcMem, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));
    
    // 狗头颜色
    COLORREF mainColor = smile ? RGB(255, 200, 100) : RGB(150, 150, 150);
    HBRUSH brush = CreateSolidBrush(mainColor);
    
    // 简单的圆
    SelectObject(hdcMem, brush);
    SelectObject(hdcMem, GetStockObject(NULL_PEN));
    Ellipse(hdcMem, 4, 4, 28, 28);
    
    // 眼睛
    SetPixel(hdcMem, 12, 14, RGB(0, 0, 0));
    SetPixel(hdcMem, 20, 14, RGB(0, 0, 0));
    
    // 嘴巴
    if (smile) {
        SetPixel(hdcMem, 14, 20, RGB(0, 0, 0));
        SetPixel(hdcMem, 16, 21, RGB(0, 0, 0));
        SetPixel(hdcMem, 18, 21, RGB(0, 0, 0));
        SetPixel(hdcMem, 20, 20, RGB(0, 0, 0));
    } else {
        SetPixel(hdcMem, 14, 20, RGB(0, 0, 0));
        SetPixel(hdcMem, 16, 20, RGB(0, 0, 0));
        SetPixel(hdcMem, 18, 20, RGB(0, 0, 0));
        SetPixel(hdcMem, 20, 20, RGB(0, 0, 0));
    }
    
    DeleteObject(brush);
    
    // 创建掩码
    HDC hdcMask = CreateCompatibleDC(hdcScreen);
    SelectObject(hdcMask, hbmMask);
    FillRect(hdcMask, &rc, (HBRUSH)GetStockObject(WHITE_BRUSH));
    SelectObject(hdcMask, GetStockObject(BLACK_BRUSH));
    Ellipse(hdcMask, 4, 4, 28, 28);
    DeleteDC(hdcMask);
    
    ICONINFO ii = {0};
    ii.fIcon = TRUE;
    ii.hbmMask = hbmMask;
    ii.hbmColor = hbmColor;
    HICON hIcon = CreateIconIndirect(&ii);
    
    DeleteObject(hbmColor);
    DeleteObject(hbmMask);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
    
    return hIcon;
}

void TrayIconManager::AddMenuItem(HMENU menu, int id, const std::wstring& text, bool checked) {
    if (id == MENU_SEPARATOR) {
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        return;
    }
    
    UINT flags = MF_STRING;
    if (checked) {
        flags |= MF_CHECKED;
    }
    AppendMenuW(menu, flags, id, text.c_str());
}

} // namespace screencover
