#include "Dx12App.h"

Dx12App::~Dx12App()
{
    SaveHighScoresNoThrow();

    if (m_constantBufferDataBegin)
    {
        m_constantBuffer->Unmap(0, nullptr);
        m_constantBufferDataBegin = nullptr;
    }

    if (m_fenceEvent)
    {
        CloseHandle(m_fenceEvent);
    }

    if (m_noonPromptFont)
    {
        DeleteObject(m_noonPromptFont);
    }
}

void Dx12App::Initialize(HINSTANCE instance, int showCommand)
{
    m_instance = instance;

    // Win32 控件和 DX12 资源按依赖顺序创建：窗口先有，控件和交换链才能绑定。
    INITCOMMONCONTROLSEX commonControls{};
    commonControls.dwSize = sizeof(commonControls);
    commonControls.dwICC = ICC_BAR_CLASSES | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&commonControls);

    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    CreateAppWindow(instance, showCommand);
    CreateEditorControls();
    CreateGameHudControls();

    m_assetRoot = FindAssetRoot();
    m_userDataRoot = FindUserDataRoot();
    LoadHighScores();
    LoadPipeline();
    LoadAssets();
    SyncBackgroundGrayTrackbar();
    SyncEditorToSelectedModel();
    UpdateAimTrainerLabels();
    UpdateWindowTitle();
}

int Dx12App::Run()
{
    MSG message{};
    auto lastTime = std::chrono::steady_clock::now();

    // 非阻塞消息循环：没有窗口消息时持续更新、录制命令并 Present。
    while (message.message != WM_QUIT)
    {
        if (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
        else
        {
            const auto currentTime = std::chrono::steady_clock::now();
            const std::chrono::duration<float> elapsed = currentTime - lastTime;
            lastTime = currentTime;

            const float deltaSeconds = std::min(elapsed.count(), 0.1f);
            m_totalTime += deltaSeconds;
            UpdateFrameStats(deltaSeconds);

            Update(deltaSeconds);
            PopulateCommandList();

            ID3D12CommandList* commandLists[] = {m_commandList.Get()};
            m_commandQueue->ExecuteCommandLists(1, commandLists);

            const UINT syncInterval = m_vsyncEnabled ? 1 : 0;
            const UINT presentFlags = !m_vsyncEnabled && m_tearingSupported
                ? DXGI_PRESENT_ALLOW_TEARING
                : 0;
            ThrowIfFailed(m_swapChain->Present(syncInterval, presentFlags));
            MoveToNextFrame();
        }
    }

    WaitForGpu();
    return static_cast<int>(message.wParam);
}

void Dx12App::UpdateFrameStats(float deltaSeconds)
{
    ++m_fpsFrameCount;
    m_fpsElapsed += deltaSeconds;

    // 每 0.25 秒刷新一次文本，避免频繁 SetWindowText 影响帧时间。
    if (m_fpsElapsed < 0.25f)
    {
        return;
    }

    m_currentFps = static_cast<float>(m_fpsFrameCount) / m_fpsElapsed;
    m_fpsFrameCount = 0;
    m_fpsElapsed = 0.0f;

    if (m_fpsLabel)
    {
        wchar_t buffer[64]{};
        swprintf_s(buffer, L"当前帧数：%.1f FPS", m_currentFps);
        SetWindowTextW(m_fpsLabel, buffer);
    }
    if (m_hudFpsLabel)
    {
        wchar_t buffer[64]{};
        swprintf_s(buffer, L"FPS：%.1f", m_currentFps);
        SetWindowTextW(m_hudFpsLabel, buffer);
    }
}
