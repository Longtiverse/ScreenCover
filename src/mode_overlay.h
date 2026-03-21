#pragma once

#include "mode_base.h"
#include "overlay_manager.h"
#include <functional>

namespace screencover {

// 软件黑屏模式 - 使用全屏黑色窗口
class ModeOverlay : public BlackoutMode {
public:
    using ExitCallback = std::function<void()>;
    
    ModeOverlay();
    ~ModeOverlay() override;
    
    // 禁止拷贝
    ModeOverlay(const ModeOverlay&) = delete;
    ModeOverlay& operator=(const ModeOverlay&) = delete;
    
    bool Enter() override;
    bool Exit() override;
    bool IsActive() const override;
    std::wstring GetName() const override;
    BlackoutModeType GetType() const override;
    
    // 输入模式接口
    void SetInputMode(InputModeType mode) override;
    InputModeType GetInputMode() const override;
    void ToggleInputMode() override;
    
    // 设置 ESC 退出回调
    void SetExitCallback(ExitCallback callback);
    
private:
    // 安装输入拦截钩子
    void InstallInputHooks();
    
    // 卸载输入拦截钩子
    void RemoveInputHooks();
    
    // 键盘钩子回调
    static LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
    
    // 鼠标钩子回调
    static LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam);
    
    // 检查是否是退出热键
    bool IsExitHotkey(WPARAM wParam, LPARAM lParam) const;
    
private:
    std::unique_ptr<OverlayManager> overlay_;
    ExitCallback exitCallback_;
    InputModeType inputMode_;
    HHOOK keyboardHook_;
    HHOOK mouseHook_;
    
    // 单例指针（用于钩子回调）
    static ModeOverlay* instance_;
};

} // namespace screencover
