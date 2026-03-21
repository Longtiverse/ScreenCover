#include "settings_manager.h"
#include "mode_base.h"
#include <shlwapi.h>
#include <sstream>

#pragma comment(lib, "shlwapi.lib")

namespace screencover {

SettingsManager::SettingsManager()
    : blackoutMode_(BlackoutModeType::OVERLAY)
    , inputMode_(InputModeType::FREE)
{
    // 设置默认配置文件路径（与 exe 同目录）
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    PathRemoveFileSpecW(exePath);
    configPath_ = std::wstring(exePath) + L"\\ScreenCover.ini";
}

SettingsManager::~SettingsManager() {
    Save();
}

bool SettingsManager::Load() {
    // 读取热键配置
    std::wstring exitHotkey = ReadIniString(L"Hotkeys", L"ExitHotkey", L"ESC");
    StringToHotkey(exitHotkey, hotkeys_.exitMod, hotkeys_.exitKey);
    
    std::wstring toggleHotkey = ReadIniString(L"Hotkeys", L"ToggleInputModeHotkey", L"Ctrl+Shift+L");
    StringToHotkey(toggleHotkey, hotkeys_.toggleInputMod, hotkeys_.toggleInputKey);
    
    // 读取设置
    settings_.defaultBlockInput = ReadIniInt(L"Settings", L"DefaultBlockInput", 0) != 0;
    settings_.showNotification = ReadIniInt(L"Settings", L"ShowNotification", 1) != 0;
    settings_.hotkeyTimeout = ReadIniInt(L"Settings", L"HotkeyTimeout", 500);
    
    // 读取状态
    std::wstring modeStr = ReadIniString(L"State", L"CurrentMode", L"overlay");
    blackoutMode_ = (modeStr == L"poweroff") ? BlackoutModeType::POWEROFF : BlackoutModeType::OVERLAY;
    
    std::wstring inputStr = ReadIniString(L"State", L"CurrentInputMode", L"free");
    inputMode_ = (inputStr == L"locked") ? InputModeType::LOCKED : InputModeType::FREE;
    
    return true;
}

bool SettingsManager::Save() {
    // 保存热键配置
    WriteIniString(L"Hotkeys", L"ExitHotkey", HotkeyToString(hotkeys_.exitMod, hotkeys_.exitKey));
    WriteIniString(L"Hotkeys", L"ToggleInputModeHotkey", HotkeyToString(hotkeys_.toggleInputMod, hotkeys_.toggleInputKey));
    
    // 保存设置
    WriteIniInt(L"Settings", L"DefaultBlockInput", settings_.defaultBlockInput ? 1 : 0);
    WriteIniInt(L"Settings", L"ShowNotification", settings_.showNotification ? 1 : 0);
    WriteIniInt(L"Settings", L"HotkeyTimeout", static_cast<int>(settings_.hotkeyTimeout));
    
    // 保存状态
    WriteIniString(L"State", L"CurrentMode", (blackoutMode_ == BlackoutModeType::POWEROFF) ? L"poweroff" : L"overlay");
    WriteIniString(L"State", L"CurrentInputMode", (inputMode_ == InputModeType::LOCKED) ? L"locked" : L"free");
    
    return true;
}

std::wstring SettingsManager::GetConfigPath() const {
    return configPath_;
}

void SettingsManager::SetExitHotkey(DWORD mod, DWORD key) {
    hotkeys_.exitMod = mod;
    hotkeys_.exitKey = key;
}

void SettingsManager::SetToggleInputHotkey(DWORD mod, DWORD key) {
    hotkeys_.toggleInputMod = mod;
    hotkeys_.toggleInputKey = key;
}

void SettingsManager::SetDefaultBlockInput(bool block) {
    settings_.defaultBlockInput = block;
}

void SettingsManager::SetShowNotification(bool show) {
    settings_.showNotification = show;
}

void SettingsManager::SetHotkeyTimeout(DWORD timeout) {
    settings_.hotkeyTimeout = timeout;
}

void SettingsManager::SetBlackoutMode(BlackoutModeType mode) {
    blackoutMode_ = mode;
}

void SettingsManager::SetInputMode(InputModeType mode) {
    inputMode_ = mode;
}

std::wstring SettingsManager::FormatHotkey(DWORD mod, DWORD key) {
    std::wstring result;
    
    if (mod & MOD_CONTROL) result += L"Ctrl+";
    if (mod & MOD_ALT) result += L"Alt+";
    if (mod & MOD_SHIFT) result += L"Shift+";
    if (mod & MOD_WIN) result += L"Win+";
    
    result += GetKeyName(key);
    
    return result;
}

bool SettingsManager::ParseHotkey(const std::wstring& str, DWORD& mod, DWORD& key) {
    return StringToHotkey(str, mod, key);
}

std::wstring SettingsManager::ReadIniString(const std::wstring& section,
                                            const std::wstring& key,
                                            const std::wstring& defaultValue) {
    wchar_t buffer[MAX_PATH];
    GetPrivateProfileStringW(section.c_str(), key.c_str(), 
                             defaultValue.c_str(), buffer, MAX_PATH, 
                             configPath_.c_str());
    return buffer;
}

int SettingsManager::ReadIniInt(const std::wstring& section,
                                const std::wstring& key,
                                int defaultValue) {
    return GetPrivateProfileIntW(section.c_str(), key.c_str(), 
                                 defaultValue, configPath_.c_str());
}

void SettingsManager::WriteIniString(const std::wstring& section,
                                     const std::wstring& key,
                                     const std::wstring& value) {
    WritePrivateProfileStringW(section.c_str(), key.c_str(), 
                               value.c_str(), configPath_.c_str());
}

void SettingsManager::WriteIniInt(const std::wstring& section,
                                  const std::wstring& key,
                                  int value) {
    std::wstring str = std::to_wstring(value);
    WritePrivateProfileStringW(section.c_str(), key.c_str(), 
                               str.c_str(), configPath_.c_str());
}

std::wstring SettingsManager::HotkeyToString(DWORD mod, DWORD key) {
    return FormatHotkey(mod, key);
}

bool SettingsManager::StringToHotkey(const std::wstring& str, DWORD& mod, DWORD& key) {
    mod = 0;
    key = 0;
    
    std::wstring s = str;
    std::transform(s.begin(), s.end(), s.begin(), ::towlower);
    
    // 解析修饰键
    if (s.find(L"ctrl+") != std::wstring::npos) {
        mod |= MOD_CONTROL;
        s.erase(s.find(L"ctrl+"), 5);
    }
    if (s.find(L"alt+") != std::wstring::npos) {
        mod |= MOD_ALT;
        s.erase(s.find(L"alt+"), 4);
    }
    if (s.find(L"shift+") != std::wstring::npos) {
        mod |= MOD_SHIFT;
        s.erase(s.find(L"shift+"), 6);
    }
    if (s.find(L"win+") != std::wstring::npos) {
        mod |= MOD_WIN;
        s.erase(s.find(L"win+"), 4);
    }
    
    // 解析主键
    key = GetKeyCode(s);
    
    return key != 0;
}

const wchar_t* SettingsManager::GetKeyName(DWORD vkCode) {
    switch (vkCode) {
        case VK_ESCAPE: return L"Esc";
        case VK_RETURN: return L"Enter";
        case VK_SPACE: return L"Space";
        case VK_TAB: return L"Tab";
        case VK_BACK: return L"Backspace";
        case VK_DELETE: return L"Delete";
        case VK_INSERT: return L"Insert";
        case VK_HOME: return L"Home";
        case VK_END: return L"End";
        case VK_PRIOR: return L"PageUp";
        case VK_NEXT: return L"PageDown";
        case VK_UP: return L"Up";
        case VK_DOWN: return L"Down";
        case VK_LEFT: return L"Left";
        case VK_RIGHT: return L"Right";
        case VK_F1: return L"F1";
        case VK_F2: return L"F2";
        case VK_F3: return L"F3";
        case VK_F4: return L"F4";
        case VK_F5: return L"F5";
        case VK_F6: return L"F6";
        case VK_F7: return L"F7";
        case VK_F8: return L"F8";
        case VK_F9: return L"F9";
        case VK_F10: return L"F10";
        case VK_F11: return L"F11";
        case VK_F12: return L"F12";
        case VK_NUMPAD0: return L"Numpad0";
        case VK_NUMPAD1: return L"Numpad1";
        case VK_NUMPAD2: return L"Numpad2";
        case VK_NUMPAD3: return L"Numpad3";
        case VK_NUMPAD4: return L"Numpad4";
        case VK_NUMPAD5: return L"Numpad5";
        case VK_NUMPAD6: return L"Numpad6";
        case VK_NUMPAD7: return L"Numpad7";
        case VK_NUMPAD8: return L"Numpad8";
        case VK_NUMPAD9: return L"Numpad9";
        default:
            if (vkCode >= 'A' && vkCode <= 'Z') {
                static wchar_t buf[2] = {0, 0};
                buf[0] = static_cast<wchar_t>(vkCode);
                return buf;
            }
            if (vkCode >= '0' && vkCode <= '9') {
                static wchar_t buf[2] = {0, 0};
                buf[0] = static_cast<wchar_t>(vkCode);
                return buf;
            }
            return L"Unknown";
    }
}

DWORD SettingsManager::GetKeyCode(const std::wstring& name) {
    if (name == L"esc" || name == L"escape") return VK_ESCAPE;
    if (name == L"enter") return VK_RETURN;
    if (name == L"space") return VK_SPACE;
    if (name == L"tab") return VK_TAB;
    if (name == L"backspace" || name == L"back") return VK_BACK;
    if (name == L"delete" || name == L"del") return VK_DELETE;
    if (name == L"insert" || name == L"ins") return VK_INSERT;
    if (name == L"home") return VK_HOME;
    if (name == L"end") return VK_END;
    if (name == L"pageup" || name == L"pgup") return VK_PRIOR;
    if (name == L"pagedown" || name == L"pgdn") return VK_NEXT;
    if (name == L"up") return VK_UP;
    if (name == L"down") return VK_DOWN;
    if (name == L"left") return VK_LEFT;
    if (name == L"right") return VK_RIGHT;
    if (name == L"f1") return VK_F1;
    if (name == L"f2") return VK_F2;
    if (name == L"f3") return VK_F3;
    if (name == L"f4") return VK_F4;
    if (name == L"f5") return VK_F5;
    if (name == L"f6") return VK_F6;
    if (name == L"f7") return VK_F7;
    if (name == L"f8") return VK_F8;
    if (name == L"f9") return VK_F9;
    if (name == L"f10") return VK_F10;
    if (name == L"f11") return VK_F11;
    if (name == L"f12") return VK_F12;
    if (name == L"numpad0") return VK_NUMPAD0;
    if (name == L"numpad1") return VK_NUMPAD1;
    if (name == L"numpad2") return VK_NUMPAD2;
    if (name == L"numpad3") return VK_NUMPAD3;
    if (name == L"numpad4") return VK_NUMPAD4;
    if (name == L"numpad5") return VK_NUMPAD5;
    if (name == L"numpad6") return VK_NUMPAD6;
    if (name == L"numpad7") return VK_NUMPAD7;
    if (name == L"numpad8") return VK_NUMPAD8;
    if (name == L"numpad9") return VK_NUMPAD9;
    
    // 单个字符
    if (name.length() == 1) {
        wchar_t c = towupper(name[0]);
        if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
            return static_cast<DWORD>(c);
        }
    }
    
    return 0;
}

} // namespace screencover
