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
    
    bool Enter() override;
    bool Exit() override;
    bool IsActive() const override;
    std::wstring GetName() const override;
    BlackoutModeType GetType() const override;
    
    // 设置 ESC 退出回调
    void SetExitCallback(ExitCallback callback);
    
private:
    std::unique_ptr<OverlayManager> overlay_;
    ExitCallback exitCallback_;
};

} // namespace screencover
