// ScreenCover.cpp - 极简可靠版本
// 编译: cl ScreenCover.cpp user32.lib gdi32.lib shell32.lib

#include <windows.h>
#include <shellapi.h>

#define HOTKEY_ID 1
#define WM_TRAY (WM_USER + 1)

HWND g_hwnd = NULL;
HWND g_coverWnd = NULL;
BOOL g_isBlackout = FALSE;
BOOL g_isHardwareMode = FALSE; // FALSE=软件模式, TRUE=硬件模式
HICON g_icon1 = NULL; // 笑脸
HICON g_icon2 = NULL; // 不笑

// 创建像素图标
HICON CreateDogIcon(BOOL smile) {
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

// 覆盖窗口过程
LRESULT CALLBACK CoverWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_KEYDOWN && wParam == VK_ESCAPE) {
        PostMessage(g_hwnd, WM_APP, 0, 0); // 通知主窗口退出
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// 创建覆盖窗口
void CreateCoverWindow() {
    WNDCLASS wc = {0};
    wc.lpfnWndProc = CoverWndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"ScreenCoverOverlay";
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    RegisterClass(&wc);
    
    int w = GetSystemMetrics(SM_CXSCREEN);
    int h = GetSystemMetrics(SM_CYSCREEN);
    
    g_coverWnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        L"ScreenCoverOverlay",
        L"ScreenCover",
        WS_POPUP | WS_VISIBLE,
        0, 0, w, h,
        NULL, NULL, wc.hInstance, NULL
    );
}

// 关闭显示器
void TurnOffMonitor() {
    SendMessage(HWND_BROADCAST, WM_SYSCOMMAND, SC_MONITORPOWER, 2);
}

// 打开显示器
void TurnOnMonitor() {
    mouse_event(MOUSEEVENTF_MOVE, 0, 0, 0, 0);
    SendMessage(HWND_BROADCAST, WM_SYSCOMMAND, SC_MONITORPOWER, -1);
}

// 进入黑屏
void EnterBlackout() {
    if (g_isBlackout) return;
    g_isBlackout = TRUE;
    
    if (g_isHardwareMode) {
        TurnOffMonitor();
    } else {
        CreateCoverWindow();
    }
    
    // 更新托盘提示
    NOTIFYICONDATA nid = {0};
    nid.cbSize = sizeof(nid);
    nid.hWnd = g_hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_TIP;
    wcscpy_s(nid.szTip, L"ScreenCover - 黑屏中");
    Shell_NotifyIcon(NIM_MODIFY, &nid);
}

// 退出黑屏
void ExitBlackout() {
    if (!g_isBlackout) return;
    g_isBlackout = FALSE;
    
    if (g_isHardwareMode) {
        TurnOnMonitor();
    } else {
        if (g_coverWnd) {
            DestroyWindow(g_coverWnd);
            g_coverWnd = NULL;
        }
    }
    
    // 更新托盘提示
    NOTIFYICONDATA nid = {0};
    nid.cbSize = sizeof(nid);
    nid.hWnd = g_hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_TIP;
    wcscpy_s(nid.szTip, L"ScreenCover - 监控中");
    Shell_NotifyIcon(NIM_MODIFY, &nid);
}

// 切换黑屏
void ToggleBlackout() {
    if (g_isBlackout) ExitBlackout();
    else EnterBlackout();
}

// 切换模式
void SwitchMode() {
    if (g_isBlackout) ExitBlackout();
    g_isHardwareMode = !g_isHardwareMode;
    
    // 更新图标
    NOTIFYICONDATA nid = {0};
    nid.cbSize = sizeof(nid);
    nid.hWnd = g_hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON;
    nid.hIcon = g_isHardwareMode ? g_icon2 : g_icon1;
    Shell_NotifyIcon(NIM_MODIFY, &nid);
    
    MessageBox(g_hwnd, 
               g_isHardwareMode ? L"已切换到: 硬件断电模式" : L"已切换到: 软件黑屏模式",
               L"ScreenCover", MB_OK);
}

// 显示托盘菜单
void ShowTrayMenu() {
    HMENU menu = CreatePopupMenu();
    AppendMenu(menu, MF_STRING, 1, L"切换模式");
    AppendMenu(menu, MF_SEPARATOR, 0, NULL);
    AppendMenu(menu, MF_STRING, 2, L"退出");
    
    POINT pt;
    GetCursorPos(&pt);
    SetForegroundWindow(g_hwnd);
    
    int cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_NONOTIFY, 
                            pt.x, pt.y, 0, g_hwnd, NULL);
    
    DestroyMenu(menu);
    
    if (cmd == 1) SwitchMode();
    else if (cmd == 2) PostQuitMessage(0);
}

// 主窗口过程
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            // 注册热键 Win+Shift+H
            RegisterHotKey(hwnd, HOTKEY_ID, MOD_WIN | MOD_SHIFT, 'H');
            
            // 创建图标
            g_icon1 = CreateDogIcon(TRUE);
            g_icon2 = CreateDogIcon(FALSE);
            
            // 添加托盘图标
            {
                NOTIFYICONDATA nid = {0};
                nid.cbSize = sizeof(nid);
                nid.hWnd = hwnd;
                nid.uID = 1;
                nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
                nid.uCallbackMessage = WM_TRAY;
                nid.hIcon = g_icon1;
                wcscpy_s(nid.szTip, L"ScreenCover - 软件黑屏 - 监控中");
                Shell_NotifyIcon(NIM_ADD, &nid);
            }
            
            // 显示启动提示
            MessageBox(hwnd, L"热键: Win+Shift+H\n右键托盘切换模式", L"ScreenCover 已启动", MB_OK);
            return 0;
            
        case WM_HOTKEY:
            if (wParam == HOTKEY_ID) {
                ToggleBlackout();
            }
            return 0;
            
        case WM_TRAY:
            if (lParam == WM_RBUTTONUP) {
                ShowTrayMenu();
            }
            return 0;
            
        case WM_APP: // 内部消息：退出黑屏
            ExitBlackout();
            return 0;
            
        case WM_DESTROY:
            UnregisterHotKey(hwnd, HOTKEY_ID);
            
            {
                NOTIFYICONDATA nid2 = {0};
                nid2.cbSize = sizeof(nid2);
                nid2.hWnd = hwnd;
                nid2.uID = 1;
                Shell_NotifyIcon(NIM_DELETE, &nid2);
            }
            
            if (g_icon1) DestroyIcon(g_icon1);
            if (g_icon2) DestroyIcon(g_icon2);
            
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int) {
    // 检查单实例
    HANDLE mutex = CreateMutex(NULL, TRUE, L"ScreenCover_Mutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        MessageBox(NULL, L"ScreenCover 已经在运行！", L"ScreenCover", MB_OK);
        return 1;
    }
    
    // 注册窗口类
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"ScreenCoverMain";
    RegisterClass(&wc);
    
    // 创建隐藏窗口
    g_hwnd = CreateWindow(
        L"ScreenCoverMain",
        L"ScreenCover",
        0,
        0, 0, 0, 0,
        HWND_MESSAGE, NULL, hInstance, NULL
    );
    
    // 消息循环
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    ReleaseMutex(mutex);
    CloseHandle(mutex);
    
    return 0;
}
