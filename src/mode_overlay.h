#pragma once

#include "mode_base.h"
#include "overlay_manager.h"

namespace screencover {

// 软件黑屏模式 - 使用全屏黑色窗口
class ModeOverlay : public BlackoutMode {
public:
    ModeOverlay();
    ~ModeOverlay() override;
    
    bool Enter() override;
    bool Exit() override;
    bool IsActive() const override;
    std::wstring GetName() const override;
    BlackoutModeType GetType() const override;
    UINT GetTrayIconId() const override;
    
private:
    std::unique_ptr<OverlayManager> overlay_;
};

} // namespace screencover
