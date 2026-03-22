// ScreenCover v3.1 - 黑屏工具 + 输入拦截
// 编译: g++ ScreenCover_simple.cpp -o ScreenCover.exe -lgdi32 -luser32 -lshell32 -mwindows -DUNICODE -D_UNICODE -municode

#include <windows.h>
#include <shellapi.h>

// ==================== 常量定义 ====================
#define WM_TRAY         (WM_USER + 1)
#define WM_EXIT_BLACKOUT (WM_USER + 2)
#define HOTKEY_BLACKOUT 1
#define HOTKEY_LOCK     2
#define REG_PATH        L"Software\\ScreenCover"
#define MUTEX_NAME      L"Global\\ScreenCover_Mutex"

// ==================== 全局变量 ====================
HWND g_hwnd = NULL;           // 主窗口句柄
HWND g_coverWnd = NULL;       // 黑屏覆盖窗口
BOOL g_isBlackout = FALSE;    // 是否正在黑屏
BOOL g_isLocked = FALSE;      // 是否锁定输入
BOOL g_isHardwareMode = FALSE; // 是否硬件模式
HICON g_iconSmile = NULL;     // 笑脸图标
HICON g_iconSad = NULL;       // 不笑图标
HHOOK g_kbHook = NULL;        // 键盘钩子
HHOOK g_mouseHook = NULL;     // 鼠标钩子

// 热键配置
UINT g_blackoutMod = MOD_WIN | MOD_SHIFT;
UINT g_blackoutVk = 'H';
UINT g_lockMod = MOD_WIN | MOD_SHIFT;
UINT g_lockVk = 'L';

// ==================== 配置存储 ====================
void LoadConfig() {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, REG_PATH, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD size = sizeof(UINT);
        RegQueryValueExW(hKey, L"BlackoutMod", NULL, NULL, (LPBYTE)&g_blackoutMod, &size);
        RegQueryValueExW(hKey, L"BlackoutVk", NULL, NULL, (LPBYTE)&g_blackoutVk, &size);
        RegQueryValueExW(hKey, L"LockMod", NULL, NULL, (LPBYTE)&g_lockMod, &size);
        RegQueryValueExW(hKey, L"LockVk", NULL, NULL, (LPBYTE)&g_lockVk, &size);
        DWORD hwMode = 0;
        size = sizeof(DWORD);
        RegQueryValueExW(hKey, L"HardwareMode", NULL, NULL, (LPBYTE)&hwMode, &size);
        g_isHardwareMode = (BOOL)hwMode;
        RegCloseKey(hKey);
    }
}

void SaveConfig() {
    HKEY hKey;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, REG_PATH, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExW(hKey, L"BlackoutMod", 0, REG_DWORD, (LPBYTE)&g_blackoutMod, sizeof(UINT));
        RegSetValueExW(hKey, L"BlackoutVk", 0, REG_DWORD, (LPBYTE)&g_blackoutVk, sizeof(UINT));
        RegSetValueExW(hKey, L"LockMod", 0, REG_DWORD, (LPBYTE)&g_lockMod, sizeof(UINT));
        RegSetValueExW(hKey, L"LockVk", 0, REG_DWORD, (LPBYTE)&g_lockVk, sizeof(UINT));
        DWORD hwMode = (DWORD)g_isHardwareMode;
        RegSetValueExW(hKey, L"HardwareMode", 0, REG_DWORD, (LPBYTE)&hwMode, sizeof(DWORD));
        RegCloseKey(hKey);
    }
}

// ==================== 热键名称 ====================
void GetKeyName(UINT vk, UINT mod, LPWSTR buf, int bufSize) {
    buf[0] = 0;
    if (mod & MOD_WIN) wcscat_s(buf, bufSize, L"Win+");
    if (mod & MOD_CONTROL) wcscat_s(buf, bufSize, L"Ctrl+");
    if (mod & MOD_SHIFT) wcscat_s(buf, bufSize, L"Shift+");
    if (mod & MOD_ALT) wcscat_s(buf, bufSize, L"Alt+");
    
    WCHAR keyName[32] = {0};
    UINT scanCode = MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
    if (scanCode) {
        GetKeyNameTextW(scanCode << 16, keyName, 32);
    } else {
        wsprintfW(keyName, L"%c", (char)vk);
    }
    wcscat_s(buf, bufSize, keyName);
}

// ==================== 输入拦截钩子 ====================
BOOL CheckHotkeyMatch(UINT vk, UINT requiredVk, UINT requiredMod) {
    if (vk != requiredVk) return FALSE;
    
    BOOL ctrlOk = ((requiredMod & MOD_CONTROL) != 0) == ((GetKeyState(VK_CONTROL) & 0x8000) != 0);
    BOOL shiftOk = ((requiredMod & MOD_SHIFT) != 0) == ((GetKeyState(VK_SHIFT) & 0x8000) != 0);
    BOOL altOk = ((requiredMod & MOD_ALT) != 0) == ((GetKeyState(VK_MENU) & 0x8000) != 0);
    BOOL winOk = ((requiredMod & MOD_WIN) != 0) == ((GetKeyState(VK_LWIN) & 0x8000) || (GetKeyState(VK_RWIN) & 0x8000));
    
    return ctrlOk && shiftOk && altOk && winOk;
}

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && g_isLocked && wParam == WM_KEYDOWN) {
        KBDLLHOOKSTRUCT* kb = (KBDLLHOOKSTRUCT*)lParam;
        
        // 放行黑屏热键
        if (CheckHotkeyMatch(kb->vkCode, g_blackoutVk, g_blackoutMod)) {
            return CallNextHookEx(g_kbHook, nCode, wParam, lParam);
        }
        // 放行锁定热键
        if (CheckHotkeyMatch(kb->vkCode, g_lockVk, g_lockMod)) {
            return CallNextHookEx(g_kbHook, nCode, wParam, lParam);
        }
        // 拦截其他按键
        return 1;
    }
    return CallNextHookEx(g_kbHook, nCode, wParam, lParam);
}

LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && g_isLocked) {
        return 1;  // 拦截所有鼠标输入
    }
    return CallNextHookEx(g_mouseHook, nCode, wParam, lParam);
}

void InstallHooks() {
    if (!g_kbHook) {
        g_kbHook = SetWindowsHookExW(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandleW(NULL), 0);
    }
    if (!g_mouseHook) {
        g_mouseHook = SetWindowsHookExW(WH_MOUSE_LL, LowLevelMouseProc, GetModuleHandleW(NULL), 0);
    }
}

void UninstallHooks() {
    if (g_kbHook) { UnhookWindowsHookEx(g_kbHook); g_kbHook = NULL; }
    if (g_mouseHook) { UnhookWindowsHookEx(g_mouseHook); g_mouseHook = NULL; }
}

void ToggleLock() {
    g_isLocked = !g_isLocked;
    if (g_isLocked) {
        InstallHooks();
    } else {
        UninstallHooks();
    }
}

// ==================== 图标创建 ====================
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
    RECT rc = {0, 0, 32, 32};
    FillRect(hdcMem, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));
    
    COLORREF mainColor = smile ? RGB(255, 200, 100) : RGB(150, 150, 150);
    HBRUSH brush = CreateSolidBrush(mainColor);
    SelectObject(hdcMem, brush);
    SelectObject(hdcMem, GetStockObject(NULL_PEN));
    Ellipse(hdcMem, 4, 4, 28, 28);
    DeleteObject(brush);
    
    // 眼睛
    SetPixel(hdcMem, 12, 14, RGB(0, 0, 0));
    SetPixel(hdcMem, 20, 14, RGB(0, 0, 0));
    
    // 嘴巴
    SetPixel(hdcMem, 14, 20, RGB(0, 0, 0));
    SetPixel(hdcMem, 16, smile ? 21 : 20, RGB(0, 0, 0));
    SetPixel(hdcMem, 18, smile ? 21 : 20, RGB(0, 0, 0));
    SetPixel(hdcMem, 20, 20, RGB(0, 0, 0));
    
    // 掩码
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

// ==================== 黑屏窗口 ====================
LRESULT CALLBACK CoverWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // 不处理任何输入，热键由主窗口处理
    switch (msg) {
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_HOTKEY:
            return 0;  // 吞掉所有按键
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
            return 0;  // 忽略鼠标点击
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void CreateCoverWindow() {
    WNDCLASS wc = {0};
    wc.lpfnWndProc = CoverWndProc;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.lpszClassName = L"ScreenCoverOverlay";
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    RegisterClassW(&wc);
    
    int w = GetSystemMetrics(SM_CXSCREEN);
    int h = GetSystemMetrics(SM_CYSCREEN);
    
    g_coverWnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        L"ScreenCoverOverlay", L"",
        WS_POPUP, 0, 0, w, h,
        NULL, NULL, GetModuleHandleW(NULL), NULL
    );
    
    if (g_coverWnd) {
        ShowWindow(g_coverWnd, SW_SHOW);
        SetForegroundWindow(g_coverWnd);
    }
}

// ==================== 黑屏控制 ====================
void EnterBlackout() {
    if (g_isBlackout) return;
    g_isBlackout = TRUE;
    
    if (g_isHardwareMode) {
        SendMessageW(HWND_BROADCAST, WM_SYSCOMMAND, SC_MONITORPOWER, 2);
    } else {
        CreateCoverWindow();
    }
}

void ExitBlackout() {
    if (!g_isBlackout) return;
    g_isBlackout = FALSE;
    
    if (g_isHardwareMode) {
        // 唤醒显示器
        mouse_event(MOUSEEVENTF_MOVE, 1, 0, 0, 0);
        mouse_event(MOUSEEVENTF_MOVE, -1, 0, 0, 0);
    } else {
        if (g_coverWnd) {
            DestroyWindow(g_coverWnd);
            g_coverWnd = NULL;
        }
    }
}

void ToggleBlackout() {
    if (g_isBlackout) {
        ExitBlackout();
    } else {
        EnterBlackout();
    }
}

// ==================== 更新托盘 ====================
void UpdateTrayIcon() {
    NOTIFYICONDATAW nid = {0};
    nid.cbSize = sizeof(NOTIFYICONDATAW);
    nid.hWnd = g_hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_TIP;
    nid.hIcon = g_isHardwareMode ? g_iconSad : g_iconSmile;
    wsprintfW(nid.szTip, L"ScreenCover [%s]", g_isHardwareMode ? L"硬件模式" : L"软件模式");
    Shell_NotifyIconW(NIM_MODIFY, &nid);
}

// ==================== 热键捕获窗口 ====================
HWND g_captureWnd = NULL;
BOOL g_captureForBlackout = TRUE;
UINT g_capturedMod = 0;
UINT g_capturedVk = 0;
WCHAR g_capturedText[128] = L"请按下热键组合...";

#define ID_BTN_CONFIRM 101
#define ID_BTN_CANCEL  102
#define IDC_HOTKEY_DISPLAY 103

void UpdateHotkeyDisplay(HWND hwnd) {
    SetDlgItemTextW(hwnd, IDC_HOTKEY_DISPLAY, g_capturedText);
    InvalidateRect(hwnd, NULL, TRUE);
}

LRESULT CALLBACK CaptureWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            // 先注销热键，避免冲突
            UnregisterHotKey(g_hwnd, HOTKEY_BLACKOUT);
            UnregisterHotKey(g_hwnd, HOTKEY_LOCK);
            
            // 重置捕获状态
            g_capturedMod = 0;
            g_capturedVk = 0;
            lstrcpyW(g_capturedText, L"请按下热键组合...");
            
            // 创建控件
            CreateWindowW(L"STATIC", g_capturedText,
                WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE,
                20, 20, 310, 40,
                hwnd, (HMENU)IDC_HOTKEY_DISPLAY, 
                GetModuleHandleW(NULL), NULL);
            
            CreateWindowW(L"BUTTON", L"确认",
                WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                80, 80, 80, 30,
                hwnd, (HMENU)ID_BTN_CONFIRM,
                GetModuleHandleW(NULL), NULL);
            
            CreateWindowW(L"BUTTON", L"取消",
                WS_CHILD | WS_VISIBLE,
                190, 80, 80, 30,
                hwnd, (HMENU)ID_BTN_CANCEL,
                GetModuleHandleW(NULL), NULL);
            
            // 初始禁用确认按钮
            EnableWindow(GetDlgItem(hwnd, ID_BTN_CONFIRM), FALSE);
            
            return 0;
        }
        
        case WM_COMMAND:
            if (LOWORD(wParam) == ID_BTN_CONFIRM) {
                // 确认按钮 - 保存设置
                if (g_capturedMod != 0 && g_capturedVk != 0) {
                    if (g_captureForBlackout) {
                        g_blackoutMod = g_capturedMod;
                        g_blackoutVk = g_capturedVk;
                    } else {
                        g_lockMod = g_capturedMod;
                        g_lockVk = g_capturedVk;
                    }
                    SaveConfig();
                    
                    // 重新注册热键
                    RegisterHotKey(g_hwnd, HOTKEY_BLACKOUT, g_blackoutMod, g_blackoutVk);
                    RegisterHotKey(g_hwnd, HOTKEY_LOCK, g_lockMod, g_lockVk);
                    
                    DestroyWindow(hwnd);
                }
            }
            else if (LOWORD(wParam) == ID_BTN_CANCEL) {
                // 取消按钮 - 恢复原热键
                RegisterHotKey(g_hwnd, HOTKEY_BLACKOUT, g_blackoutMod, g_blackoutVk);
                RegisterHotKey(g_hwnd, HOTKEY_LOCK, g_lockMod, g_lockVk);
                DestroyWindow(hwnd);
            }
            return 0;
        
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN: {
            UINT vk = (UINT)wParam;
            
            // ESC 取消
            if (vk == VK_ESCAPE) {
                RegisterHotKey(g_hwnd, HOTKEY_BLACKOUT, g_blackoutMod, g_blackoutVk);
                RegisterHotKey(g_hwnd, HOTKEY_LOCK, g_lockMod, g_lockVk);
                DestroyWindow(hwnd);
                return 0;
            }
            
            // 更新修饰键状态
            g_capturedMod = 0;
            if (GetKeyState(VK_CONTROL) & 0x8000) g_capturedMod |= MOD_CONTROL;
            if (GetKeyState(VK_SHIFT) & 0x8000) g_capturedMod |= MOD_SHIFT;
            if (GetKeyState(VK_MENU) & 0x8000) g_capturedMod |= MOD_ALT;
            if (GetKeyState(VK_LWIN) & 0x8000 || GetKeyState(VK_RWIN) & 0x8000) g_capturedMod |= MOD_WIN;
            
            // 忽略单独的修饰键
            if (vk == VK_CONTROL || vk == VK_SHIFT || vk == VK_MENU || 
                vk == VK_LWIN || vk == VK_RWIN) {
                // 只更新显示
                WCHAR buf[64] = L"";
                if (g_capturedMod & MOD_WIN) wcscat_s(buf, 64, L"Win+");
                if (g_capturedMod & MOD_CONTROL) wcscat_s(buf, 64, L"Ctrl+");
                if (g_capturedMod & MOD_SHIFT) wcscat_s(buf, 64, L"Shift+");
                if (g_capturedMod & MOD_ALT) wcscat_s(buf, 64, L"Alt+");
                if (buf[0] == 0) lstrcpyW(buf, L"请按下热键组合...");
                lstrcpyW(g_capturedText, buf);
                UpdateHotkeyDisplay(hwnd);
                return 0;
            }
            
            // 必须有修饰键
            if (g_capturedMod == 0) {
                lstrcpyW(g_capturedText, L"请至少按住一个修饰键...");
                UpdateHotkeyDisplay(hwnd);
                return 0;
            }
            
            // 记录主键
            g_capturedVk = vk;
            
            // 构建显示文本
            WCHAR keyName[64] = {0};
            GetKeyName(vk, g_capturedMod, keyName, 64);
            lstrcpyW(g_capturedText, keyName);
            UpdateHotkeyDisplay(hwnd);
            
            // 启用确认按钮
            EnableWindow(GetDlgItem(hwnd, ID_BTN_CONFIRM), TRUE);
            SetFocus(GetDlgItem(hwnd, ID_BTN_CONFIRM));
            
            return 0;
        }
        
        case WM_DESTROY:
            g_captureWnd = NULL;
            return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void StartCaptureHotkey(BOOL forBlackout) {
    if (g_captureWnd) {
        SetForegroundWindow(g_captureWnd);
        return;
    }
    
    g_captureForBlackout = forBlackout;
    
    WNDCLASS wc = {0};
    wc.lpfnWndProc = CaptureWndProc;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.lpszClassName = L"ScreenCoverCapture";
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    RegisterClassW(&wc);
    
    g_captureWnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_DLGMODALFRAME,
        L"ScreenCoverCapture",
        forBlackout ? L"设置黑屏热键" : L"设置锁定热键",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, 350, 150,
        NULL, NULL, GetModuleHandleW(NULL), NULL
    );
    
    ShowWindow(g_captureWnd, SW_SHOW);
    SetForegroundWindow(g_captureWnd);
}

// ==================== 托盘菜单 ====================
void ShowTrayMenu() {
    HMENU menu = CreatePopupMenu();
    
    WCHAR buf[128];
    WCHAR item[256];
    
    // 显示当前热键
    GetKeyName(g_blackoutVk, g_blackoutMod, buf, 128);
    wsprintfW(item, L"黑屏热键: %s", buf);
    AppendMenuW(menu, MF_STRING, 1, item);
    
    GetKeyName(g_lockVk, g_lockMod, buf, 128);
    wsprintfW(item, L"锁定热键: %s", buf);
    AppendMenuW(menu, MF_STRING, 2, item);
    
    AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
    
    // 模式切换
    AppendMenuW(menu, MF_STRING, 3, 
        g_isHardwareMode ? L"切换到软件模式" : L"切换到硬件模式");
    
    // 锁定切换
    AppendMenuW(menu, MF_STRING, 4,
        g_isLocked ? L"解锁输入" : L"锁定输入");
    
    AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(menu, MF_STRING, 5, L"退出");
    
    POINT pt;
    GetCursorPos(&pt);
    SetForegroundWindow(g_hwnd);
    
    int cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_NONOTIFY, 
                            pt.x, pt.y, 0, g_hwnd, NULL);
    DestroyMenu(menu);
    
    switch (cmd) {
        case 1: StartCaptureHotkey(TRUE); break;
        case 2: StartCaptureHotkey(FALSE); break;
        case 3:
            g_isHardwareMode = !g_isHardwareMode;
            SaveConfig();
            UpdateTrayIcon();
            break;
        case 4:
            ToggleLock();
            break;
        case 5:
            PostQuitMessage(0);
            break;
    }
}

// ==================== 主窗口过程 ====================
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            // 注册热键
            RegisterHotKey(hwnd, HOTKEY_BLACKOUT, g_blackoutMod, g_blackoutVk);
            RegisterHotKey(hwnd, HOTKEY_LOCK, g_lockMod, g_lockVk);
            
            // 创建图标
            g_iconSmile = CreateDogIcon(TRUE);
            g_iconSad = CreateDogIcon(FALSE);
            
            // 添加托盘图标
            {
                NOTIFYICONDATAW nid = {0};
                nid.cbSize = sizeof(NOTIFYICONDATAW);
                nid.hWnd = hwnd;
                nid.uID = 1;
                nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
                nid.uCallbackMessage = WM_TRAY;
                nid.hIcon = g_iconSmile;
                lstrcpyW(nid.szTip, L"ScreenCover [软件模式]");
                Shell_NotifyIconW(NIM_ADD, &nid);
            }
            return 0;
            
        case WM_HOTKEY:
            if (wParam == HOTKEY_BLACKOUT) {
                ToggleBlackout();
            } else if (wParam == HOTKEY_LOCK) {
                ToggleLock();
            }
            return 0;
            
        case WM_TRAY:
            // 左键单击：切换模式
            if (lParam == WM_LBUTTONUP) {
                g_isHardwareMode = !g_isHardwareMode;
                SaveConfig();
                UpdateTrayIcon();
            }
            // 右键单击：显示菜单
            else if (lParam == WM_RBUTTONUP) {
                ShowTrayMenu();
            }
            return 0;
            
        case WM_EXIT_BLACKOUT:
            ExitBlackout();
            return 0;
            
        case WM_DESTROY:
            UnregisterHotKey(hwnd, HOTKEY_BLACKOUT);
            UnregisterHotKey(hwnd, HOTKEY_LOCK);
            
            // 删除托盘图标
            {
                NOTIFYICONDATAW nid = {0};
                nid.cbSize = sizeof(NOTIFYICONDATAW);
                nid.hWnd = hwnd;
                nid.uID = 1;
                Shell_NotifyIconW(NIM_DELETE, &nid);
            }
            
            if (g_iconSmile) DestroyIcon(g_iconSmile);
            if (g_iconSad) DestroyIcon(g_iconSad);
            
            UninstallHooks();
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// ==================== 入口 ====================
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int) {
    // 检查单实例
    HANDLE mutex = CreateMutexW(NULL, TRUE, MUTEX_NAME);
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(mutex);
        return 0;
    }
    
    // 加载配置
    LoadConfig();
    
    // 注册窗口类
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"ScreenCoverMain";
    RegisterClassW(&wc);
    
    // 创建消息窗口
    g_hwnd = CreateWindowExW(
        0, L"ScreenCoverMain", L"ScreenCover",
        0, 0, 0, 0, 0,
        HWND_MESSAGE, NULL, hInstance, NULL
    );
    
    // 消息循环
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        if (g_captureWnd && IsDialogMessageW(g_captureWnd, &msg)) {
            continue;
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    
    ReleaseMutex(mutex);
    CloseHandle(mutex);
    
    return 0;
}
