#pragma once

#include <windows.h>
#include <string>

namespace screencover {

// 黑屏模式类型
enum class BlackoutModeType {
    OVERLAY,    // 软件黑屏（窗口覆盖）
    POWEROFF    // 硬件断电（显示器电源管理）
};

// 黑屏模式基类
class BlackoutMode {
public:
    virtual ~BlackoutMode() = default;
    
    // 进入黑屏状态
    virtual bool Enter() = 0;
    
    // 退出黑屏状态
    virtual bool Exit() = 0;
    
    // 检查是否处于黑屏状态
    virtual bool IsActive() const = 0;
    
    // 获取模式名称
    virtual std::wstring GetName() const = 0;
    
    // 获取模式类型
    virtual BlackoutModeType GetType() const = 0;
};

} // namespace screencover
