/**
 * ScreenCover - 屏幕覆盖工具 (混合模式版本)
 * 
 * 功能：
 * - 软件黑屏模式：全屏黑色置顶窗口，ESC 退出
 * - 硬件断电模式：关闭显示器电源，Watchdog 自动保护
 * 
 * 热键：按住 Ctrl 键，连续按下 3 次 "6" 键触发
 * 切换：右键托盘图标选择模式
 * 
 * 托盘图标：
 * - 笑脸小狗 = 软件黑屏模式
 * - 不笑小狗 = 硬件断电模式
 */

#include <windows.h>
#include "application.h"

using namespace screencover;

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
                    LPWSTR lpCmdLine, int nCmdShow) {
    // 避免多实例运行
    HANDLE hMutex = CreateMutexW(nullptr, TRUE, L"ScreenCover_Mutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        MessageBoxW(nullptr, 
                   L"ScreenCover 已经在运行！",
                   L"ScreenCover", MB_ICONWARNING | MB_OK);
        CloseHandle(hMutex);
        return 1;
    }
    
    // 创建并运行应用程序
    Application app;
    
    if (!app.Initialize()) {
        MessageBoxW(nullptr,
                   L"程序初始化失败！",
                   L"ScreenCover", MB_ICONERROR | MB_OK);
        CloseHandle(hMutex);
        return 1;
    }
    
    // 运行主循环
    app.Run();
    
    // 清理
    CloseHandle(hMutex);
    
    return 0;
}
