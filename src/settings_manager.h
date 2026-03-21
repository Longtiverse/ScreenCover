#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

#include <windows.h>
#include <string>
#include <functional>

namespace screencover {

// 配置项结构
struct HotkeyConfig {
    // 触发热键（固定为 Ctrl+666）
    static constexpr DWORD TRIGGER_MOD = MOD_CONTROL;
    static constexpr DWORD TRIGGER_KEY = '6';
    static constexpr int TRIGGER_COUNT = 3;
    
    // 退出热键
    DWORD exitMod = 0;
    DWORD exitKey = VK_ESCAPE;
    
    // 切换输入模式热键
    DWORD toggleInputMod = MOD_CONTROL | MOD_SHIFT;
    DWORD toggleInputKey = 'L';
};

struct SettingsConfig {
    // 输入阻断默认状态
    bool defaultBlockInput = false;
    
    // 切换模式时显示通知
    bool showNotification = true;
    
    // 热键响应超时（ms）
    DWORD hotkeyTimeout = 500;
};

// 黑屏模式类型
enum class BlackoutModeType;

// 输入模式类型
enum class InputModeType {
    FREE,      // 自由模式：输入穿透
    LOCKED     // 锁定模式：阻断输入
};

class SettingsManager {
public:
    SettingsManager();
    ~SettingsManager();
    
    // 禁止拷贝
    SettingsManager(const SettingsManager&) = delete;
    SettingsManager& operator=(const SettingsManager&) = delete;
    
    // 加载配置文件
    bool Load();
    
    // 保存配置文件
    bool Save();
    
    // 获取配置文件路径
    std::wstring GetConfigPath() const;
    
    // 热键配置
    const HotkeyConfig& GetHotkeys() const { return hotkeys_; }
    void SetExitHotkey(DWORD mod, DWORD key);
    void SetToggleInputHotkey(DWORD mod, DWORD key);
    
    // 设置配置
    const SettingsConfig& GetSettings() const { return settings_; }
    void SetDefaultBlockInput(bool block);
    void SetShowNotification(bool show);
    void SetHotkeyTimeout(DWORD timeout);
    
    // 黑屏模式
    BlackoutModeType GetBlackoutMode() const { return blackoutMode_; }
    void SetBlackoutMode(BlackoutModeType mode);
    
    // 输入模式
    InputModeType GetInputMode() const { return inputMode_; }
    void SetInputMode(InputModeType mode);
    
    // 格式化热键显示字符串
    static std::wstring FormatHotkey(DWORD mod, DWORD key);
    
    // 解析热键字符串
    static bool ParseHotkey(const std::wstring& str, DWORD& mod, DWORD& key);
    
private:
    HotkeyConfig hotkeys_;
    SettingsConfig settings_;
    BlackoutModeType blackoutMode_;
    InputModeType inputMode_;
    std::wstring configPath_;
    
    // INI 文件读写辅助
    std::wstring ReadIniString(const std::wstring& section, 
                               const std::wstring& key,
                               const std::wstring& defaultValue);
    int ReadIniInt(const std::wstring& section, 
                   const std::wstring& key,
                   int defaultValue);
    void WriteIniString(const std::wstring& section,
                        const std::wstring& key,
                        const std::wstring& value);
    void WriteIniInt(const std::wstring& section,
                     const std::wstring& key,
                     int value);
    
    // 热键字符串转换
    std::wstring HotkeyToString(DWORD mod, DWORD key);
    bool StringToHotkey(const std::wstring& str, DWORD& mod, DWORD& key);
    
    // 虚拟键码名称映射
    static const wchar_t* GetKeyName(DWORD vkCode);
    static DWORD GetKeyCode(const std::wstring& name);
};

} // namespace screencover

#endif // SETTINGS_MANAGER_H
