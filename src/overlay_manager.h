#ifndef OVERLAY_MANAGER_H
#define OVERLAY_MANAGER_H

#include <windows.h>
#include <vector>
#include <memory>

namespace screencover {

// 单个显示器的覆盖窗口信息
struct OverlayWindow {
    HWND hwnd;
    HMONITOR monitor;
    RECT rect;
    
    OverlayWindow() : hwnd(nullptr), monitor(nullptr) {}
};

class OverlayManager {
public:
    OverlayManager();
    ~OverlayManager();
    
    // 禁止拷贝
    OverlayManager(const OverlayManager&) = delete;
    OverlayManager& operator=(const OverlayManager&) = delete;
    
    // 初始化并创建所有覆盖窗口
    bool Initialize();
    
    // 销毁所有覆盖窗口
    void Shutdown();
    
    // 显示覆盖窗口
    void ShowOverlay();
    
    // 隐藏覆盖窗口
    void HideOverlay();
    
    // 切换显示状态
    void ToggleOverlay();
    
    // 检查是否正在显示
    bool IsVisible() const { return isVisible_; }
    
    // 处理ESC键（可由外部调用）
    void HandleEscape();
    
    // 获取覆盖窗口数量
    size_t GetWindowCount() const { return windows_.size(); }
    
private:
    // 窗口过程函数（静态）
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    
    // 注册窗口类
    bool RegisterWindowClass();
    
    // 注销窗口类
    void UnregisterWindowClass();
    
    // 为每个显示器创建窗口
    bool CreateOverlayWindows();
    
    // 销毁所有窗口
    void DestroyOverlayWindows();
    
    // 枚举显示器回调（静态）
    static BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, 
                                          LPRECT lprcMonitor, LPARAM dwData);
    
    // 设置窗口为全屏
    void SetFullscreen(HWND hwnd, const RECT& rect);
    
private:
    std::vector<OverlayWindow> windows_;
    bool isVisible_;
    bool classRegistered_;
    static constexpr const wchar_t* CLASS_NAME = L"ScreenCoverOverlayClass";
    
    // 单例指针
    static OverlayManager* instance_;
};

} // namespace screencover

#endif // OVERLAY_MANAGER_H
