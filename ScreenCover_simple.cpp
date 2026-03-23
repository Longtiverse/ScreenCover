// ScreenCover v3.2 - 黑屏工具 + 输入拦截
// 编译: g++ ScreenCover_simple.cpp -o ScreenCover.exe -lgdi32 -luser32 -lshell32 -mwindows -DUNICODE -D_UNICODE -municode

#include <windows.h>
#include <shellapi.h>

// ==================== 常量定义 ====================
#define WM_TRAY         (WM_USER + 1)
#define HOTKEY_BLACKOUT 1
#define HOTKEY_LOCK     2
#define HOTKEY_EXIT     3
#define HOTKEY_MODE     4  // 模式切换热键
#define REG_PATH        L"Software\\ScreenCover"
#define MUTEX_NAME      L"Global\\ScreenCover_Mutex"

// ==================== 全局变量 ====================
static HWND g_hwnd = NULL;
static HWND g_coverWnd = NULL;
static BOOL g_isBlackout = FALSE;
static BOOL g_isLocked = FALSE;
static BOOL g_isHardwareMode = FALSE;
static HICON g_iconSmile = NULL;
static HICON g_iconSad = NULL;
static HHOOK g_kbHook = NULL;
static HHOOK g_mouseHook = NULL;

// 函数声明
void ShowHint(LPCWSTR text);
void UpdateTrayIcon();

// 热键配置
static UINT g_blackoutMod = MOD_WIN | MOD_SHIFT;
static UINT g_blackoutVk = 'H';
static UINT g_lockMod = MOD_WIN | MOD_SHIFT;
static UINT g_lockVk = 'L';
static UINT g_modeMod = MOD_WIN | MOD_SHIFT;
static UINT g_modeVk = 'X';
// 安全退出热键固定: Win+Shift+ESC

// 热键捕获
static HWND g_captureWnd = NULL;
static INT g_captureMode = 0;  // 0=黑屏, 1=锁定, 2=模式
static UINT g_capturedMod = 0;
static UINT g_capturedVk = 0;
static WCHAR g_capturedText[128] = L"请按下热键组合...";

// ==================== 配置存储 ====================
void LoadConfig() {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, REG_PATH, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD size = sizeof(UINT);
        RegQueryValueExW(hKey, L"BlackoutMod", NULL, NULL, (LPBYTE)&g_blackoutMod, &size);
        RegQueryValueExW(hKey, L"BlackoutVk", NULL, NULL, (LPBYTE)&g_blackoutVk, &size);
        RegQueryValueExW(hKey, L"LockMod", NULL, NULL, (LPBYTE)&g_lockMod, &size);
        RegQueryValueExW(hKey, L"LockVk", NULL, NULL, (LPBYTE)&g_lockVk, &size);
        RegQueryValueExW(hKey, L"ModeMod", NULL, NULL, (LPBYTE)&g_modeMod, &size);
        RegQueryValueExW(hKey, L"ModeVk", NULL, NULL, (LPBYTE)&g_modeVk, &size);
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
        RegSetValueExW(hKey, L"ModeMod", 0, REG_DWORD, (LPBYTE)&g_modeMod, sizeof(UINT));
        RegSetValueExW(hKey, L"ModeVk", 0, REG_DWORD, (LPBYTE)&g_modeVk, sizeof(UINT));
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

// 检查热键是否匹配
BOOL CheckHotkeyMatch(UINT vk, UINT requiredVk, UINT requiredMod) {
    if (vk != requiredVk) return FALSE;
    
    BOOL winPressed = (GetAsyncKeyState(VK_LWIN) & 0x8000) || (GetAsyncKeyState(VK_RWIN) & 0x8000);
    BOOL shiftPressed = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    BOOL ctrlPressed = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
    BOOL altPressed = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
    
    if (requiredMod & MOD_CONTROL) { if (!ctrlPressed) return FALSE; }
    else { if (ctrlPressed) return FALSE; }
    
    if (requiredMod & MOD_SHIFT) { if (!shiftPressed) return FALSE; }
    else { if (shiftPressed) return FALSE; }
    
    if (requiredMod & MOD_ALT) { if (!altPressed) return FALSE; }
    else { if (altPressed) return FALSE; }
    
    if (requiredMod & MOD_WIN) { if (!winPressed) return FALSE; }
    else { if (winPressed) return FALSE; }
    
    return TRUE;
}

// ==================== 输入拦截钩子 ====================
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && g_isLocked && wParam == WM_KEYDOWN) {
        KBDLLHOOKSTRUCT* kb = (KBDLLHOOKSTRUCT*)lParam;
        
        // 放行所有修饰键（必须放行，否则热键检测会失败）
        if (kb->vkCode == VK_LWIN || kb->vkCode == VK_RWIN ||
            kb->vkCode == VK_LSHIFT || kb->vkCode == VK_RSHIFT ||
            kb->vkCode == VK_LCONTROL || kb->vkCode == VK_RCONTROL ||
            kb->vkCode == VK_LMENU || kb->vkCode == VK_RMENU) {
            return CallNextHookEx(g_kbHook, nCode, wParam, lParam);
        }
        
        // 安全退出: Win+Shift+ESC (必须最优先检查)
        BOOL winPressed = (GetAsyncKeyState(VK_LWIN) & 0x8000) || (GetAsyncKeyState(VK_RWIN) & 0x8000);
        BOOL shiftPressed = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
        if (kb->vkCode == VK_ESCAPE && winPressed && shiftPressed) {
            return CallNextHookEx(g_kbHook, nCode, wParam, lParam);
        }
        
        // 检查自定义热键
        if (CheckHotkeyMatch(kb->vkCode, g_blackoutVk, g_blackoutMod) ||
            CheckHotkeyMatch(kb->vkCode, g_lockVk, g_lockMod) ||
            CheckHotkeyMatch(kb->vkCode, g_modeVk, g_modeMod)) {
            return CallNextHookEx(g_kbHook, nCode, wParam, lParam);
        }
        
        // 拦截其他按键
        return 1;
    }
    return CallNextHookEx(g_kbHook, nCode, wParam, lParam);
}

LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && g_isLocked) {
        return 1;
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
        ShowHint(L"LOCK ON");
    } else {
        UninstallHooks();
        ShowHint(L"LOCK OFF");
    }
    UpdateTrayIcon();  // 更新托盘提示显示锁定状态
}

void ToggleMode() {
    g_isHardwareMode = !g_isHardwareMode;
    SaveConfig();
    UpdateTrayIcon();
    ShowHint(g_isHardwareMode ? L"HW" : L"SW");
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
    
    SetPixel(hdcMem, 12, 14, RGB(0, 0, 0));
    SetPixel(hdcMem, 20, 14, RGB(0, 0, 0));
    SetPixel(hdcMem, 14, 20, RGB(0, 0, 0));
    SetPixel(hdcMem, 16, smile ? 21 : 20, RGB(0, 0, 0));
    SetPixel(hdcMem, 18, smile ? 21 : 20, RGB(0, 0, 0));
    SetPixel(hdcMem, 20, 20, RGB(0, 0, 0));
    
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
// 空白光标，用于隐藏鼠标
static HCURSOR g_hBlankCursor = NULL;

LRESULT CALLBACK CoverWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_SETCURSOR:
            // 隐藏鼠标光标
            SetCursor(NULL);
            return TRUE;
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_HOTKEY:
            return 0;
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
            return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void CreateBlankCursor() {
    // 创建32x32全透明光标
    BYTE andMask[128];  // 32x32 / 8 = 128
    BYTE xorMask[128];
    ZeroMemory(andMask, 128);
    ZeroMemory(xorMask, 128);
    g_hBlankCursor = CreateCursor(GetModuleHandleW(NULL), 0, 0, 32, 32, andMask, xorMask);
}

void CreateCoverWindow() {
    // 创建空白光标（如果还没有）
    if (!g_hBlankCursor) {
        CreateBlankCursor();
    }
    
    WNDCLASS wc = {0};
    wc.lpfnWndProc = CoverWndProc;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.lpszClassName = L"ScreenCoverOverlay";
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.hCursor = g_hBlankCursor;  // 设置空白光标
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
        // 强制隐藏光标
        SetCursor(NULL);
    }
}

// ==================== 黑屏控制 ====================
void EnterBlackout() {
    if (g_isBlackout) return;
    g_isBlackout = TRUE;
    
    if (g_isHardwareMode) {
        // 使用 SendMessage 同步等待，比 PostMessage 更快
        SendMessage(HWND_BROADCAST, WM_SYSCOMMAND, SC_MONITORPOWER, 2);
    } else {
        CreateCoverWindow();
    }
}

void ExitBlackout() {
    if (!g_isBlackout) return;
    g_isBlackout = FALSE;
    
    if (g_isHardwareMode) {
        // 组合多种方式唤醒，确保兼容性
        // 方式1: SendInput（更可靠）
        INPUT inputs[2] = {0};
        inputs[0].type = INPUT_KEYBOARD;
        inputs[0].ki.wVk = VK_SHIFT;
        inputs[1].type = INPUT_KEYBOARD;
        inputs[1].ki.wVk = VK_SHIFT;
        inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(2, inputs, sizeof(INPUT));
        
        // 方式2: 鼠标移动（备用）
        mouse_event(MOUSEEVENTF_MOVE, 1, 1, 0, 0);
        
        // 方式3: 通知系统显示器需要开启
        SetThreadExecutionState(ES_DISPLAY_REQUIRED);
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
        ShowHint(L"OFF");
    } else {
        EnterBlackout();
        ShowHint(L"ON");
    }
}

// 安全退出 - 解除所有锁定并退出
void SafeExit() {
    if (g_isBlackout) {
        ExitBlackout();
    }
    if (g_isLocked) {
        g_isLocked = FALSE;
        UninstallHooks();
    }
    PostQuitMessage(0);
}

// ==================== 屏幕提示 ====================
#define TIMER_HINT 100

LRESULT CALLBACK HintWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_TIMER && wParam == TIMER_HINT) {
        KillTimer(hwnd, TIMER_HINT);
        DestroyWindow(hwnd);
        return 0;
    }
    if (msg == WM_PAINT) {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(255, 255, 255));
        
        WCHAR text[32] = {0};
        GetWindowTextW(hwnd, text, 32);
        DrawTextW(hdc, text, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        EndPaint(hwnd, &ps);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void ShowHint(LPCWSTR text) {
    // 注册提示窗口类（如果还没注册）
    static BOOL registered = FALSE;
    if (!registered) {
        WNDCLASS wc = {0};
        wc.lpfnWndProc = HintWndProc;
        wc.hInstance = GetModuleHandleW(NULL);
        wc.lpszClassName = L"ScreenCoverHint";
        wc.hbrBackground = (HBRUSH)GetStockObject(DKGRAY_BRUSH);
        RegisterClassW(&wc);
        registered = TRUE;
    }
    
    // 获取屏幕尺寸
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    
    // 窗口大小和位置（左下角）
    int w = 120, h = 50;
    int x = 30;
    int y = screenH - h - 80;
    
    HWND hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
        L"ScreenCoverHint", text,
        WS_POPUP | WS_VISIBLE,
        x, y, w, h,
        NULL, NULL, GetModuleHandleW(NULL), NULL
    );
    
    if (hwnd) {
        // 设置半透明
        SetLayeredWindowAttributes(hwnd, 0, 200, LWA_ALPHA);
        
        // 500ms 后销毁
        SetTimer(hwnd, TIMER_HINT, 500, NULL);
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
    wsprintfW(nid.szTip, L"ScreenCover [%s%s]", 
              g_isHardwareMode ? L"硬件" : L"软件",
              g_isLocked ? L" 已锁定" : L"");
    Shell_NotifyIconW(NIM_MODIFY, &nid);
}

// ==================== 热键捕获窗口 ====================
#define ID_BTN_CONFIRM 101
#define ID_BTN_CANCEL  102
#define IDC_HOTKEY_DISPLAY 103

void UpdateHotkeyDisplay(HWND hwnd) {
    SetDlgItemTextW(hwnd, IDC_HOTKEY_DISPLAY, g_capturedText);
}

LRESULT CALLBACK CaptureWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            UnregisterHotKey(g_hwnd, HOTKEY_BLACKOUT);
            UnregisterHotKey(g_hwnd, HOTKEY_LOCK);
            UnregisterHotKey(g_hwnd, HOTKEY_MODE);
            
            g_capturedMod = 0;
            g_capturedVk = 0;
            lstrcpyW(g_capturedText, L"请按下热键组合...");
            
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
            
            EnableWindow(GetDlgItem(hwnd, ID_BTN_CONFIRM), FALSE);
            return 0;
        }
        
        case WM_COMMAND:
            if (LOWORD(wParam) == ID_BTN_CONFIRM) {
                if (g_capturedMod != 0 && g_capturedVk != 0) {
                    if (g_captureMode == 0) {
                        g_blackoutMod = g_capturedMod;
                        g_blackoutVk = g_capturedVk;
                    } else if (g_captureMode == 1) {
                        g_lockMod = g_capturedMod;
                        g_lockVk = g_capturedVk;
                    } else {
                        g_modeMod = g_capturedMod;
                        g_modeVk = g_capturedVk;
                    }
                    SaveConfig();
                    RegisterHotKey(g_hwnd, HOTKEY_BLACKOUT, g_blackoutMod, g_blackoutVk);
                    RegisterHotKey(g_hwnd, HOTKEY_LOCK, g_lockMod, g_lockVk);
                    RegisterHotKey(g_hwnd, HOTKEY_MODE, g_modeMod, g_modeVk);
                    DestroyWindow(hwnd);
                }
            }
            else if (LOWORD(wParam) == ID_BTN_CANCEL) {
                RegisterHotKey(g_hwnd, HOTKEY_BLACKOUT, g_blackoutMod, g_blackoutVk);
                RegisterHotKey(g_hwnd, HOTKEY_LOCK, g_lockMod, g_lockVk);
                RegisterHotKey(g_hwnd, HOTKEY_MODE, g_modeMod, g_modeVk);
                DestroyWindow(hwnd);
            }
            return 0;
        
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN: {
            UINT vk = (UINT)wParam;
            
            if (vk == VK_ESCAPE) {
                RegisterHotKey(g_hwnd, HOTKEY_BLACKOUT, g_blackoutMod, g_blackoutVk);
                RegisterHotKey(g_hwnd, HOTKEY_LOCK, g_lockMod, g_lockVk);
                RegisterHotKey(g_hwnd, HOTKEY_MODE, g_modeMod, g_modeVk);
                DestroyWindow(hwnd);
                return 0;
            }
            
            g_capturedMod = 0;
            if (GetKeyState(VK_CONTROL) & 0x8000) g_capturedMod |= MOD_CONTROL;
            if (GetKeyState(VK_SHIFT) & 0x8000) g_capturedMod |= MOD_SHIFT;
            if (GetKeyState(VK_MENU) & 0x8000) g_capturedMod |= MOD_ALT;
            if (GetKeyState(VK_LWIN) & 0x8000 || GetKeyState(VK_RWIN) & 0x8000) g_capturedMod |= MOD_WIN;
            
            if (vk == VK_CONTROL || vk == VK_SHIFT || vk == VK_MENU || 
                vk == VK_LWIN || vk == VK_RWIN) {
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
            
            if (g_capturedMod == 0) {
                lstrcpyW(g_capturedText, L"请至少按住一个修饰键...");
                UpdateHotkeyDisplay(hwnd);
                return 0;
            }
            
            g_capturedVk = vk;
            WCHAR keyName[64] = {0};
            GetKeyName(vk, g_capturedMod, keyName, 64);
            lstrcpyW(g_capturedText, keyName);
            UpdateHotkeyDisplay(hwnd);
            
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

void StartCaptureHotkey(INT mode) {
    if (g_captureWnd) {
        SetForegroundWindow(g_captureWnd);
        return;
    }
    
    g_captureMode = mode;
    
    WNDCLASS wc = {0};
    wc.lpfnWndProc = CaptureWndProc;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.lpszClassName = L"ScreenCoverCapture";
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    RegisterClassW(&wc);
    
    LPCWSTR title = L"设置热键";
    if (mode == 0) title = L"设置黑屏热键";
    else if (mode == 1) title = L"设置锁定热键";
    else title = L"设置模式热键";
    
    g_captureWnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_DLGMODALFRAME,
        L"ScreenCoverCapture",
        title,
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, 350, 150,
        g_hwnd, NULL, GetModuleHandleW(NULL), NULL
    );
    
    ShowWindow(g_captureWnd, SW_SHOW);
    SetForegroundWindow(g_captureWnd);
}

// ==================== 托盘菜单 ====================
void ShowTrayMenu() {
    HMENU menu = CreatePopupMenu();
    if (!menu) return;
    
    WCHAR buf[128];
    WCHAR item[256];
    
    // 黑屏热键
    GetKeyName(g_blackoutVk, g_blackoutMod, buf, 128);
    wsprintfW(item, L"黑屏热键: %s", buf);
    AppendMenuW(menu, MF_STRING, 1, item);
    
    // 锁定热键
    GetKeyName(g_lockVk, g_lockMod, buf, 128);
    wsprintfW(item, L"锁定热键: %s", buf);
    AppendMenuW(menu, MF_STRING, 2, item);
    
    // 模式热键
    GetKeyName(g_modeVk, g_modeMod, buf, 128);
    wsprintfW(item, L"模式热键: %s", buf);
    AppendMenuW(menu, MF_STRING, 3, item);
    
    AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
    
    // 模式切换 - 带勾选标记
    AppendMenuW(menu, MF_STRING | (g_isHardwareMode ? MF_UNCHECKED : MF_CHECKED), 4, L"软件黑屏");
    AppendMenuW(menu, MF_STRING | (g_isHardwareMode ? MF_CHECKED : MF_UNCHECKED), 5, L"硬件断电");
    
    AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
    
    // 锁定状态
    AppendMenuW(menu, MF_STRING | (g_isLocked ? MF_CHECKED : MF_UNCHECKED), 6,
        g_isLocked ? L"已锁定输入" : L"未锁定输入");
    
    AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(menu, MF_STRING, 7, L"退出");
    
    POINT pt;
    GetCursorPos(&pt);
    SetForegroundWindow(g_hwnd);
    
    // 使用 TPM_NONOTIFY 避免菜单消息问题
    int cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON, 
                            pt.x, pt.y, 0, g_hwnd, NULL);
    
    DestroyMenu(menu);
    
    // 处理菜单命令
    switch (cmd) {
        case 1: StartCaptureHotkey(0); break;
        case 2: StartCaptureHotkey(1); break;
        case 3: StartCaptureHotkey(2); break;
        case 4:
            g_isHardwareMode = FALSE;
            SaveConfig();
            UpdateTrayIcon();
            ShowHint(L"SW");
            break;
        case 5:
            g_isHardwareMode = TRUE;
            SaveConfig();
            UpdateTrayIcon();
            ShowHint(L"HW");
            break;
        case 6: ToggleLock(); break;
        case 7: SafeExit(); break;
    }
}

// ==================== 主窗口过程 ====================
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            RegisterHotKey(hwnd, HOTKEY_BLACKOUT, g_blackoutMod, g_blackoutVk);
            RegisterHotKey(hwnd, HOTKEY_LOCK, g_lockMod, g_lockVk);
            RegisterHotKey(hwnd, HOTKEY_EXIT, MOD_WIN | MOD_SHIFT, VK_ESCAPE);
            RegisterHotKey(hwnd, HOTKEY_MODE, g_modeMod, g_modeVk);
            
            g_iconSmile = CreateDogIcon(TRUE);
            g_iconSad = CreateDogIcon(FALSE);
            
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
            } else if (wParam == HOTKEY_EXIT) {
                SafeExit();
            } else if (wParam == HOTKEY_MODE) {
                ToggleMode();
            }
            return 0;
            
        case WM_TRAY:
            if (lParam == WM_LBUTTONUP) {
                // 左键单击切换模式
                ToggleMode();
            } else if (lParam == WM_RBUTTONUP) {
                // 右键单击显示菜单
                ShowTrayMenu();
            }
            return 0;
            
        case WM_DESTROY:
            UnregisterHotKey(hwnd, HOTKEY_BLACKOUT);
            UnregisterHotKey(hwnd, HOTKEY_LOCK);
            UnregisterHotKey(hwnd, HOTKEY_EXIT);
            UnregisterHotKey(hwnd, HOTKEY_MODE);
            
            {
                NOTIFYICONDATAW nid = {0};
                nid.cbSize = sizeof(NOTIFYICONDATAW);
                nid.hWnd = hwnd;
                nid.uID = 1;
                Shell_NotifyIconW(NIM_DELETE, &nid);
            }
            
            if (g_iconSmile) DestroyIcon(g_iconSmile);
            if (g_iconSad) DestroyIcon(g_iconSad);
            if (g_hBlankCursor) DestroyCursor(g_hBlankCursor);
            
            UninstallHooks();
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// ==================== 入口 ====================
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int) {
    HANDLE mutex = CreateMutexW(NULL, TRUE, MUTEX_NAME);
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(mutex);
        return 0;
    }
    
    LoadConfig();
    
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"ScreenCoverMain";
    RegisterClassW(&wc);
    
    g_hwnd = CreateWindowExW(
        0, L"ScreenCoverMain", L"ScreenCover",
        0, 0, 0, 0, 0,
        HWND_MESSAGE, NULL, hInstance, NULL
    );
    
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
