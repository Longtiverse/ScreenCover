// ScreenCover v3.0 - 黑屏工具 + 输入拦截
// 功能：黑屏模式、输入拦截、自定义热键
// 编译: g++ ScreenCover_simple.cpp -o ScreenCover.exe -lgdi32 -luser32 -lshell32 -mwindows -DUNICODE -D_UNICODE -municode

#include <windows.h>
#include <shellapi.h>

// ==================== 配置 ====================
#define REGISTRY_PATH L"Software\\ScreenCover"

struct Config {
    UINT blackoutMod;      // 黑屏热键修饰键
    UINT blackoutVk;       // 黑屏热键虚拟键码
    UINT lockMod;          // 锁定热键修饰键
    UINT lockVk;           // 锁定热键虚拟键码
    BOOL hardwareMode;     // 硬件模式
};

Config g_config = {
    MOD_WIN | MOD_SHIFT, 'H',   // 默认: Win+Shift+H 黑屏
    MOD_WIN | MOD_SHIFT, 'L',   // 默认: Win+Shift+L 锁定
    FALSE                        // 软件模式
};

// ==================== 全局变量 ====================
#define WM_TRAY (WM_USER + 1)
#define WM_BLACKOUT_EXIT (WM_USER + 2)
#define HOTKEY_BLACKOUT 1
#define HOTKEY_LOCK 2

HWND g_hwnd = NULL;
HWND g_coverWnd = NULL;
BOOL g_isBlackout = FALSE;
BOOL g_isLocked = FALSE;  // 输入拦截状态
HICON g_icon1 = NULL;     // 笑脸
HICON g_icon2 = NULL;     // 不笑
BOOL g_overlayClassRegistered = FALSE;
HHOOK g_kbHook = NULL;    // 键盘钩子
HHOOK g_mouseHook = NULL; // 鼠标钩子

// ==================== 配置读写 ====================
void LoadConfig() {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, REGISTRY_PATH, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD size = sizeof(Config);
        RegQueryValueExW(hKey, L"Config", NULL, NULL, (LPBYTE)&g_config, &size);
        RegCloseKey(hKey);
    }
}

void SaveConfig() {
    HKEY hKey;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, REGISTRY_PATH, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExW(hKey, L"Config", 0, REG_BINARY, (LPBYTE)&g_config, sizeof(g_config));
        RegCloseKey(hKey);
    }
}

// ==================== 热键名称辅助 ====================
void GetKeyName(UINT vk, UINT mod, LPWSTR buf, int bufSize) {
    buf[0] = 0;
    
    if (mod & MOD_WIN) wcscat_s(buf, bufSize, L"Win+");
    if (mod & MOD_CONTROL) wcscat_s(buf, bufSize, L"Ctrl+");
    if (mod & MOD_SHIFT) wcscat_s(buf, bufSize, L"Shift+");
    if (mod & MOD_ALT) wcscat_s(buf, bufSize, L"Alt+");
    
    WCHAR keyName[32];
    UINT scanCode = MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
    if (scanCode) {
        GetKeyNameTextW(scanCode << 16, keyName, 32);
    } else {
        wsprintfW(keyName, L"%c", (char)vk);
    }
    wcscat_s(buf, bufSize, keyName);
}

// ==================== 输入拦截钩子 ====================
BOOL IsHotkeyCombo(UINT vk, UINT mod) {
    // 检查修饰键状态
    BOOL ctrlNeeded = (mod & MOD_CONTROL) != 0;
    BOOL shiftNeeded = (mod & MOD_SHIFT) != 0;
    BOOL altNeeded = (mod & MOD_ALT) != 0;
    BOOL winNeeded = (mod & MOD_WIN) != 0;
    
    BOOL ctrlPressed = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    BOOL shiftPressed = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    BOOL altPressed = (GetKeyState(VK_MENU) & 0x8000) != 0;
    BOOL winPressed = (GetKeyState(VK_LWIN) & 0x8000) || (GetKeyState(VK_RWIN) & 0x8000);
    
    // 检查是否匹配
    if (ctrlNeeded != ctrlPressed) return FALSE;
    if (shiftNeeded != shiftPressed) return FALSE;
    if (altNeeded != altPressed) return FALSE;
    if (winNeeded != winPressed) return FALSE;
    
    return TRUE;
}

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && g_isLocked) {
        KBDLLHOOKSTRUCT* kb = (KBDLLHOOKSTRUCT*)lParam;
        
        // 检查是否是黑屏热键 - 如果是则放行
        if (kb->vkCode == g_config.blackoutVk && IsHotkeyCombo(kb->vkCode, g_config.blackoutMod)) {
            return CallNextHookEx(g_kbHook, nCode, wParam, lParam);
        }
        
        // 检查是否是锁定热键 - 如果是则放行
        if (kb->vkCode == g_config.lockVk && IsHotkeyCombo(kb->vkCode, g_config.lockMod)) {
            return CallNextHookEx(g_kbHook, nCode, wParam, lParam);
        }
        
        // 其他按键全部拦截
        return 1;
    }
    return CallNextHookEx(g_kbHook, nCode, wParam, lParam);
}

LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && g_isLocked) {
        return 1;  // 吞掉所有鼠标输入
    }
    return CallNextHookEx(g_mouseHook, nCode, wParam, lParam);
}

void InstallHooks() {
    if (!g_kbHook) {
        g_kbHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
    }
    if (!g_mouseHook) {
        g_mouseHook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, GetModuleHandle(NULL), 0);
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
    
    SetPixel(hdcMem, 12, 14, RGB(0, 0, 0));
    SetPixel(hdcMem, 20, 14, RGB(0, 0, 0));
    
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
    if (msg == WM_KEYDOWN) {
        PostMessage(g_hwnd, WM_BLACKOUT_EXIT, 0, 0);
        return 0;
    }
    if (msg >= WM_LBUTTONDOWN && msg <= WM_MBUTTONDBLCLK) {
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void CreateCoverWindow() {
    if (!g_overlayClassRegistered) {
        WNDCLASS wc = {0};
        wc.lpfnWndProc = CoverWndProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = L"ScreenCoverOverlay";
        wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        RegisterClass(&wc);
        g_overlayClassRegistered = TRUE;
    }
    
    int w = GetSystemMetrics(SM_CXSCREEN);
    int h = GetSystemMetrics(SM_CYSCREEN);
    
    g_coverWnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        L"ScreenCoverOverlay", L"ScreenCover",
        WS_POPUP | WS_VISIBLE,
        0, 0, w, h,
        NULL, NULL, GetModuleHandle(NULL), NULL
    );
}

// ==================== 黑屏控制 ====================
void EnterBlackout() {
    if (g_isBlackout) return;
    g_isBlackout = TRUE;
    
    if (g_config.hardwareMode) {
        SendMessage(HWND_BROADCAST, WM_SYSCOMMAND, SC_MONITORPOWER, 2);
    } else {
        CreateCoverWindow();
    }
}

void ExitBlackout() {
    if (!g_isBlackout) return;
    g_isBlackout = FALSE;
    
    if (g_config.hardwareMode) {
        mouse_event(MOUSEEVENTF_MOVE, 0, 0, 0, 0);
    } else if (g_coverWnd) {
        DestroyWindow(g_coverWnd);
        g_coverWnd = NULL;
    }
}

void ToggleBlackout() {
    if (g_isBlackout) ExitBlackout();
    else EnterBlackout();
}

// ==================== 热键设置对话框 ====================
UINT g_settingVk = 0;
UINT g_settingMod = 0;
BOOL g_isSettingBlackout = TRUE;
BOOL g_isCapturing = FALSE;

INT_PTR CALLBACK HotkeyDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_INITDIALOG: {
            WCHAR buf[128];
            
            // 显示当前热键
            GetKeyName(g_config.blackoutVk, g_config.blackoutMod, buf, 128);
            SetDlgItemTextW(hDlg, 1001, buf);
            
            GetKeyName(g_config.lockVk, g_config.lockMod, buf, 128);
            SetDlgItemTextW(hDlg, 1002, buf);
            
            return TRUE;
        }
        
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case 1003:  // 设置黑屏热键
                    g_isSettingBlackout = TRUE;
                    g_isCapturing = TRUE;
                    SetDlgItemTextW(hDlg, 1001, L"按下新热键...");
                    SetFocus(GetDlgItem(hDlg, 1001));
                    return TRUE;
                    
                case 1004:  // 设置锁定热键
                    g_isSettingBlackout = FALSE;
                    g_isCapturing = TRUE;
                    SetDlgItemTextW(hDlg, 1002, L"按下新热键...");
                    SetFocus(GetDlgItem(hDlg, 1002));
                    return TRUE;
                    
                case IDOK:
                    SaveConfig();
                    EndDialog(hDlg, IDOK);
                    return TRUE;
                    
                case IDCANCEL:
                    EndDialog(hDlg, IDCANCEL);
                    return TRUE;
            }
            break;
            
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            if (g_isCapturing) {
                UINT mod = 0;
                if (GetKeyState(VK_CONTROL) & 0x8000) mod |= MOD_CONTROL;
                if (GetKeyState(VK_SHIFT) & 0x8000) mod |= MOD_SHIFT;
                if (GetKeyState(VK_MENU) & 0x8000) mod |= MOD_ALT;
                if (GetKeyState(VK_LWIN) & 0x8000 || GetKeyState(VK_RWIN) & 0x8000) mod |= MOD_WIN;
                
                UINT vk = (UINT)wParam;
                
                // 忽略单独的修饰键
                if (vk == VK_CONTROL || vk == VK_SHIFT || vk == VK_MENU || vk == VK_LWIN || vk == VK_RWIN) {
                    return TRUE;
                }
                
                if (g_isSettingBlackout) {
                    g_config.blackoutMod = mod;
                    g_config.blackoutVk = vk;
                } else {
                    g_config.lockMod = mod;
                    g_config.lockVk = vk;
                }
                
                // 更新显示
                WCHAR buf[128];
                GetKeyName(vk, mod, buf, 128);
                SetDlgItemTextW(hDlg, g_isSettingBlackout ? 1001 : 1002, buf);
                
                g_isCapturing = FALSE;
                
                // 重新注册热键
                UnregisterHotKey(g_hwnd, HOTKEY_BLACKOUT);
                UnregisterHotKey(g_hwnd, HOTKEY_LOCK);
                RegisterHotKey(g_hwnd, HOTKEY_BLACKOUT, g_config.blackoutMod, g_config.blackoutVk);
                RegisterHotKey(g_hwnd, HOTKEY_LOCK, g_config.lockMod, g_config.lockVk);
                
                return TRUE;
            }
            break;
    }
    return FALSE;
}

// ==================== 资源定义 ====================
#define IDD_HOTKEY 100
#define IDC_STATIC_BLACKOUT 1001
#define IDC_STATIC_LOCK 1002
#define IDC_BTN_BLACKOUT 1003
#define IDC_BTN_LOCK 1004

// 创建对话框资源（内存中）
HGLOBAL CreateDialogResource() {
    // 简单的对话框模板
    struct DLGTEMPLATEEX {
        WORD dlgVer;
        WORD signature;
        DWORD helpID;
        DWORD exStyle;
        DWORD style;
        WORD cDlgItems;
        short x, y, cx, cy;
        // 后面是菜单、类、标题等
    };
    
    // 使用更简单的方式 - MessageBox 输入
    return NULL;
}

// ==================== 简化的热键设置（使用捕获窗口） ====================
HWND g_captureWnd = NULL;
BOOL g_captureForBlackout = TRUE;

LRESULT CALLBACK CaptureWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            return 0;
            
        case WM_HOTKEY:
            // 这里不处理，因为我们正在捕获
            return 0;
            
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN: {
            UINT mod = 0;
            if (GetKeyState(VK_CONTROL) & 0x8000) mod |= MOD_CONTROL;
            if (GetKeyState(VK_SHIFT) & 0x8000) mod |= MOD_SHIFT;
            if (GetKeyState(VK_MENU) & 0x8000) mod |= MOD_ALT;
            if (GetKeyState(VK_LWIN) & 0x8000 || GetKeyState(VK_RWIN) & 0x8000) mod |= MOD_WIN;
            
            UINT vk = (UINT)wParam;
            
            if (vk == VK_CONTROL || vk == VK_SHIFT || vk == VK_MENU || vk == VK_LWIN || vk == VK_RWIN) {
                return 0;
            }
            
            if (g_captureForBlackout) {
                g_config.blackoutMod = mod;
                g_config.blackoutVk = vk;
            } else {
                g_config.lockMod = mod;
                g_config.lockVk = vk;
            }
            
            SaveConfig();
            
            // 重新注册热键
            UnregisterHotKey(g_hwnd, HOTKEY_BLACKOUT);
            UnregisterHotKey(g_hwnd, HOTKEY_LOCK);
            RegisterHotKey(g_hwnd, HOTKEY_BLACKOUT, g_config.blackoutMod, g_config.blackoutVk);
            RegisterHotKey(g_hwnd, HOTKEY_LOCK, g_config.lockMod, g_config.lockVk);
            
            DestroyWindow(hwnd);
            return 0;
        }
        
        case WM_DESTROY:
            g_captureWnd = NULL;
            return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void StartCaptureHotkey(BOOL forBlackout) {
    if (g_captureWnd) return;
    
    g_captureForBlackout = forBlackout;
    
    WNDCLASS wc = {0};
    wc.lpfnWndProc = CaptureWndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"ScreenCoverCapture";
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    RegisterClass(&wc);
    
    g_captureWnd = CreateWindowEx(
        WS_EX_TOPMOST,
        L"ScreenCoverCapture",
        forBlackout ? L"按下新热键 (黑屏)" : L"按下新热键 (锁定)",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 300, 100,
        NULL, NULL, GetModuleHandle(NULL), NULL
    );
}

// ==================== 托盘菜单 ====================
void ShowTrayMenu() {
    HMENU menu = CreatePopupMenu();
    
    WCHAR buf[128];
    
    // 黑屏热键
    GetKeyName(g_config.blackoutVk, g_config.blackoutMod, buf, 128);
    WCHAR menuItem1[256];
    wsprintfW(menuItem1, L"黑屏热键: %s", buf);
    AppendMenuW(menu, MF_STRING, 1, menuItem1);
    
    // 锁定热键
    GetKeyName(g_config.lockVk, g_config.lockMod, buf, 128);
    WCHAR menuItem2[256];
    wsprintfW(menuItem2, L"锁定热键: %s", buf);
    AppendMenuW(menu, MF_STRING, 2, menuItem2);
    
    AppendMenu(menu, MF_SEPARATOR, 0, NULL);
    
    // 模式切换
    AppendMenu(menu, MF_STRING, 3, g_config.hardwareMode ? L"切换到: 软件黑屏" : L"切换到: 硬件断电");
    
    // 锁定状态
    AppendMenu(menu, MF_STRING, 4, g_isLocked ? L"解锁输入" : L"锁定输入");
    
    AppendMenu(menu, MF_SEPARATOR, 0, NULL);
    AppendMenu(menu, MF_STRING, 5, L"退出");
    
    POINT pt;
    GetCursorPos(&pt);
    SetForegroundWindow(g_hwnd);
    
    int cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, g_hwnd, NULL);
    DestroyMenu(menu);
    
    switch (cmd) {
        case 1: StartCaptureHotkey(TRUE); break;
        case 2: StartCaptureHotkey(FALSE); break;
        case 3:
            g_config.hardwareMode = !g_config.hardwareMode;
            SaveConfig();
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
            RegisterHotKey(hwnd, HOTKEY_BLACKOUT, g_config.blackoutMod, g_config.blackoutVk);
            RegisterHotKey(hwnd, HOTKEY_LOCK, g_config.lockMod, g_config.lockVk);
            
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
                wcscpy_s(nid.szTip, L"ScreenCover");
                Shell_NotifyIcon(NIM_ADD, &nid);
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
            if (lParam == WM_LBUTTONUP) {
                // 左键单击切换模式
                g_config.hardwareMode = !g_config.hardwareMode;
                SaveConfig();
                // 更新图标和提示
                NOTIFYICONDATA nid = {0};
                nid.cbSize = sizeof(nid);
                nid.hWnd = hwnd;
                nid.uID = 1;
                nid.uFlags = NIF_ICON | NIF_TIP;
                nid.hIcon = g_config.hardwareMode ? g_icon2 : g_icon1;
                wcscpy_s(nid.szTip, g_config.hardwareMode ? L"ScreenCover - 硬件断电模式" : L"ScreenCover - 软件黑屏模式");
                Shell_NotifyIcon(NIM_MODIFY, &nid);
            } else if (lParam == WM_RBUTTONUP) {
                ShowTrayMenu();
            }
            return 0;
            
        case WM_BLACKOUT_EXIT:
            ExitBlackout();
            return 0;
            
        case WM_DESTROY:
            UnregisterHotKey(hwnd, HOTKEY_BLACKOUT);
            UnregisterHotKey(hwnd, HOTKEY_LOCK);
            
            {
                NOTIFYICONDATA nid = {0};
                nid.cbSize = sizeof(nid);
                nid.hWnd = hwnd;
                nid.uID = 1;
                Shell_NotifyIcon(NIM_DELETE, &nid);
            }
            
            if (g_icon1) DestroyIcon(g_icon1);
            if (g_icon2) DestroyIcon(g_icon2);
            
            UninstallHooks();
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// ==================== 入口 ====================
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int) {
    // 检查单实例
    HANDLE mutex = CreateMutex(NULL, TRUE, L"ScreenCover_Mutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        return 0;
    }
    
    // 加载配置
    LoadConfig();
    
    // 注册窗口类
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"ScreenCoverMain";
    RegisterClass(&wc);
    
    // 创建隐藏窗口
    g_hwnd = CreateWindow(
        L"ScreenCoverMain", L"ScreenCover",
        0, 0, 0, 0, 0,
        HWND_MESSAGE, NULL, hInstance, NULL
    );
    
    // 消息循环
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (g_captureWnd && IsDialogMessage(g_captureWnd, &msg)) {
            continue;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    ReleaseMutex(mutex);
    CloseHandle(mutex);
    
    return 0;
}
