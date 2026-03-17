#include "mode_overlay.h"

namespace screencover {

ModeOverlay::ModeOverlay()
    : overlay_(std::make_unique<OverlayManager>())
{
}

ModeOverlay::~ModeOverlay() {
    if (IsActive()) {
        Exit();
    }
}

bool ModeOverlay::Enter() {
    if (!overlay_) {
        return false;
    }
    
    if (!overlay_>IsInitialized()) {
        if (!overlay_>Initialize()) {
            return false;
        }
    }
    
    overlay_>ShowOverlay();
    return true;
}

bool ModeOverlay::Exit() {
    if (overlay_) {
        overlay_>HideOverlay();
    }
    return true;
}

bool ModeOverlay::IsActive() const {
    return overlay_ && overlay_>IsVisible();
}

std::wstring ModeOverlay::GetName() const {
    return L"软件黑屏";
}

BlackoutModeType ModeOverlay::GetType() const {
    return BlackoutModeType::OVERLAY;
}

UINT ModeOverlay::GetTrayIconId() const {
    return 1;  // 笑脸小狗图标资源ID
}

} // namespace screencover
