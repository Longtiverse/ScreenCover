#ifndef SEQUENCE_DETECTOR_H
#define SEQUENCE_DETECTOR_H

#include <windows.h>
#include <functional>

namespace screencover {

// 序列检测状态
enum class SequenceState {
    IDLE,           // 等待Ctrl按下
    CTRL_HELD,      // Ctrl已按住，等待第一个6
    FIRST_SIX,      // 第一个6已按下，等待第二个6
    SECOND_SIX,     // 第二个6已按下，等待第三个6
    TRIGGERED       // 已触发（重置中）
};

class SequenceDetector {
public:
    using TriggerCallback = std::function<void()>;
    
    // 构造函数
    // timeoutMs: 按键间隔超时时间（毫秒）
    explicit SequenceDetector(DWORD timeoutMs = 500);
    
    ~SequenceDetector();
    
    // 禁止拷贝
    SequenceDetector(const SequenceDetector&) = delete;
    SequenceDetector& operator=(const SequenceDetector&) = delete;
    
    // 初始化并安装键盘钩子
    bool Initialize();
    
    // 卸载键盘钩子
    void Shutdown();
    
    // 设置触发回调
    void SetCallback(TriggerCallback callback);
    
    // 检查是否已初始化
    bool IsInitialized() const { return hookHandle_ != nullptr; }
    
    // 获取当前状态（用于调试）
    SequenceState GetCurrentState() const { return currentState_; }
    
private:
    // 键盘钩子回调（静态）
    static LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
    
    // 处理键盘事件（实例方法）
    void HandleKeyboardEvent(WPARAM wParam, const KBDLLHOOKSTRUCT* kbStruct);
    
    // 状态转换
    void TransitionTo(SequenceState newState);
    
    // 检查是否超时
    bool IsTimeout() const;
    
    // 重置状态机
    void ResetState();
    
    // 触发回调
    void Trigger();
    
private:
    HHOOK hookHandle_;
    SequenceState currentState_;
    DWORD timeoutMs_;
    DWORD lastKeyTime_;
    int sixCount_;
    TriggerCallback callback_;
    
    // 单例指针（用于静态回调访问实例）
    static SequenceDetector* instance_;
};

} // namespace screencover

#endif // SEQUENCE_DETECTOR_H
