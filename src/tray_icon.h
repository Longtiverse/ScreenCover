#pragma once

#include <windows.h>
#include <string>
#include <functional>

namespace screencover {

// 系统托盘管理器
class TrayIconManager {
public:
    using MenuCallback = std::function<void(int)>;
    
    TrayIconManager();
    ~TrayIconManager();
    
    // 初始化托盘图标
    // hwnd: 主窗口句柄（用于接收托盘消息）
    // iconResId: 图标资源ID
    // tooltip: 鼠标悬停提示文本
    bool Initialize(HWND hwnd, UINT iconResId, const std::wstring& tooltip);
    
    // 关闭托盘图标
    void Shutdown();
    
    // 更新图标
    bool UpdateIcon(UINT iconResId);
    
    // 更新提示文本
    bool UpdateTooltip(const std::wstring& tooltip);
    
    // 显示气泡通知
    bool ShowBalloon(const std::wstring& title, const std::wstring& text, 
                     DWORD timeout = 3000);
    
    // 设置菜单项点击回调
    void SetMenuCallback(MenuCallback callback);
    
    // 显示上下文菜单
    void ShowContextMenu();
    
    // 处理托盘消息（应在主窗口的消息循环中调用）
    bool HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);
    
    // 检查是否已初始化
    bool IsInitialized() const { return initialized_; }
    
    // 菜单项ID定义
    static constexpr int MENU_SWITCH_MODE = 1001;
    static constexpr int MENU_SETTINGS = 1002;
    static constexpr int MENU_EXIT = 1003;
    
private:
    // 添加菜单项
    void AddMenuItem(HMENU menu, int id, const std::wstring& text, bool checked = false);
    
private:
    bool initialized_;
    HWND hwnd_;
    UINT iconId_;
    UINT trayMessage_;
    NOTIFYICONDATAW nid_;
    MenuCallback menuCallback_;
};

} // namespace screencover
