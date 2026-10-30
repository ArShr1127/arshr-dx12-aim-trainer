#include "App/Dx12App.h"

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand)
{
    try
    {
        // 应用对象拥有窗口、DX12 资源和主循环，退出时由析构函数统一清理。
        Dx12App app;
        app.Initialize(instance, showCommand);
        return app.Run();
    }
    catch (const std::exception& error)
    {
        MessageBoxA(nullptr, error.what(), "DX12 pipeline error", MB_OK | MB_ICONERROR);
        return EXIT_FAILURE;
    }
}
