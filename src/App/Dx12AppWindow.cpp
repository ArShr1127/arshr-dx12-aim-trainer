#include "Dx12App.h"

LRESULT CALLBACK Dx12App::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_NCCREATE)
    {
        // 把 this 存进窗口用户数据，后续消息就能回到 Dx12App 实例。
        const auto* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(createStruct->lpCreateParams));
        return TRUE;
    }

    auto* app = reinterpret_cast<Dx12App*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    return app ? app->HandleMessage(hwnd, message, wParam, lParam)
               : DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT Dx12App::HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        HandleCommand(LOWORD(wParam), HIWORD(wParam), reinterpret_cast<HWND>(lParam));
        return 0;

    case WM_HSCROLL:
        HandleTrackbar(reinterpret_cast<HWND>(lParam));
        return 0;

    case WM_VSCROLL:
        if (reinterpret_cast<HWND>(lParam) == m_editorScrollbar)
        {
            HandleEditorScroll(static_cast<WORD>(LOWORD(wParam)), static_cast<WORD>(HIWORD(wParam)));
            return 0;
        }
        break;

    case WM_MOUSEWHEEL:
    {
        POINT point{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        ScreenToClient(hwnd, &point);
        if (HandleEditorMouseWheel(point.x, point.y, GET_WHEEL_DELTA_WPARAM(wParam)))
        {
            return 0;
        }
        break;
    }

    case WM_CTLCOLORSTATIC:
        if (reinterpret_cast<HWND>(lParam) == m_hudNoonPromptLabel)
        {
            SetTextColor(reinterpret_cast<HDC>(wParam), m_noonPromptColor);
            SetBkMode(reinterpret_cast<HDC>(wParam), OPAQUE);
            SetBkColor(reinterpret_cast<HDC>(wParam), RGB(0, 0, 0));
            return reinterpret_cast<LRESULT>(GetStockObject(BLACK_BRUSH));
        }
        break;

    case WM_INPUT:
        if (m_gameActive)
        {
            // 游戏中使用 Raw Input 读相对位移，避免反复 SetCursorPos 造成卡顿/消息风暴。
            RAWINPUT input{};
            UINT inputSize = sizeof(input);
            const UINT readSize = GetRawInputData(
                reinterpret_cast<HRAWINPUT>(lParam),
                RID_INPUT,
                &input,
                &inputSize,
                sizeof(RAWINPUTHEADER));
            if (readSize != static_cast<UINT>(-1) && input.header.dwType == RIM_TYPEMOUSE)
            {
                const int dx = static_cast<int>(input.data.mouse.lLastX);
                const int dy = static_cast<int>(input.data.mouse.lLastY);
                if (dx != 0 || dy != 0)
                {
                    HandleGameMouseDelta(dx, dy);
                }
            }
            return 0;
        }
        break;

    case WM_KEYDOWN:
        if (wParam < m_keys.size())
        {
            m_keys[wParam] = true;
        }

        if ((lParam & (1 << 30)) == 0)
        {
            HandleKeyPressed(wParam);
        }
        return 0;

    case WM_KEYUP:
        if (wParam < m_keys.size())
        {
            m_keys[wParam] = false;
        }
        return 0;

    case WM_LBUTTONDOWN:
        m_leftFiring = true;
        if (m_gameActive)
        {
            m_lastAimX = static_cast<int>(m_renderWidth / 2);
            m_lastAimY = static_cast<int>(m_renderHeight / 2);
        }
        else
        {
            m_lastAimX = GET_X_LPARAM(lParam);
            m_lastAimY = GET_Y_LPARAM(lParam);
        }
        if (m_gameActive && m_gameMode != AimMode::Tracking)
        {
            HandleGameClick(m_lastAimX, m_lastAimY);
        }
        return 0;

    case WM_LBUTTONUP:
        m_leftFiring = false;
        return 0;

    case WM_RBUTTONDOWN:
        if (m_gameActive)
        {
            return 0;
        }

        m_mouseLook = true;
        m_lastMouseX = GET_X_LPARAM(lParam);
        m_lastMouseY = GET_Y_LPARAM(lParam);
        SetCapture(hwnd);
        ShowCursor(FALSE);
        return 0;

    case WM_RBUTTONUP:
        if (m_mouseLook)
        {
            m_mouseLook = false;
            ReleaseCapture();
            ShowCursor(TRUE);
        }
        return 0;

    case WM_CAPTURECHANGED:
        if (m_mouseLook && reinterpret_cast<HWND>(lParam) != hwnd)
        {
            m_mouseLook = false;
            ShowCursor(TRUE);
        }
        return 0;

    case WM_MOUSEMOVE:
        m_lastAimX = GET_X_LPARAM(lParam);
        m_lastAimY = GET_Y_LPARAM(lParam);

        if (m_gameActive)
        {
            // 游戏状态的射击点固定在屏幕中心，视角移动交给 WM_INPUT。
            m_lastAimX = static_cast<int>(m_renderWidth / 2);
            m_lastAimY = static_cast<int>(m_renderHeight / 2);
            return 0;
        }

        if (m_mouseLook)
        {
            const int x = m_lastAimX;
            const int y = m_lastAimY;
            const int dx = x - m_lastMouseX;
            const int dy = y - m_lastMouseY;
            m_lastMouseX = x;
            m_lastMouseY = y;

            m_cameraYaw += static_cast<float>(dx) * 0.004f;
            m_cameraPitch -= static_cast<float>(dy) * 0.004f;
            m_cameraPitch = std::clamp(m_cameraPitch, -1.45f, 1.45f);
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

void Dx12App::HandleCommand(int id, int notifyCode, HWND)
{
    // 控件按钮和列表选择最终都集中到这里分发，便于保持 UI 逻辑单向流动。
    if (id == IdModelList && notifyCode == LBN_SELCHANGE)
    {
        const LRESULT selection = SendMessageW(m_modelList, LB_GETCURSEL, 0, 0);
        if (selection >= 0 && static_cast<size_t>(selection) < m_models.size())
        {
            m_selectedModel = static_cast<size_t>(selection);
            SyncEditorToSelectedModel();
        }
        return;
    }

    if (notifyCode != BN_CLICKED)
    {
        return;
    }

    switch (id)
    {
    case IdVisible:
        if (!m_models.empty())
        {
            m_models[m_selectedModel].visible = SendMessageW(m_visibleCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
        }
        break;

    case IdResetModel:
        ResetSelectedModel();
        break;

    case IdToggleFullscreen:
        ToggleFullscreen();
        break;

    case IdToggleUi:
        SetEditorVisible(!m_editorVisible);
        break;

    case IdToggleWireframe:
        RebuildPipelineWith([this](PipelineSettings& settings) {
            settings.wireframe = !m_pipelineSettings.wireframe;
        });
        break;

    case IdReloadPipeline:
        ReloadEditablePipeline();
        break;

    case IdModeBalloon:
        SelectAimMode(AimMode::Balloon);
        break;

    case IdModeEnemy:
        SelectAimMode(AimMode::Enemy);
        break;

    case IdModeTracking:
        SelectAimMode(AimMode::Tracking);
        break;

    case IdModeNoon:
        SelectAimMode(AimMode::Noon);
        break;

    case IdDifficultyEasy:
        SelectAimDifficulty(AimDifficulty::Easy);
        break;

    case IdDifficultyMedium:
        SelectAimDifficulty(AimDifficulty::Medium);
        break;

    case IdDifficultyHard:
        SelectAimDifficulty(AimDifficulty::Hard);
        break;

    case IdSectionModel:
        ToggleEditorSection(EditorSection::Model);
        break;

    case IdSectionAim:
        ToggleEditorSection(EditorSection::Aim);
        break;

    case IdSectionLight:
        ToggleEditorSection(EditorSection::Light);
        break;

    case IdToggleLightFluctuation:
        m_light.fluctuation = SendMessageW(m_lightFluctuationButton, BM_GETCHECK, 0, 0) == BST_CHECKED;
        UpdateLightLabels();
        break;

    case IdStartGame:
        StartAimGame();
        break;

    case IdStopGame:
        StopAimGame();
        break;

    case IdResetHighScores:
        ResetHighScores();
        break;
    }
}

void Dx12App::HandleTrackbar(HWND trackbar)
{
    // 滑条先匹配全局设置，再处理当前模型属性。
    if (trackbar == m_backgroundGrayTrackbar)
    {
        HandleBackgroundGrayTrackbar();
        return;
    }

    for (int index = 0; index < LightTrackCount; ++index)
    {
        if (trackbar == m_lightTrackbars[index])
        {
            HandleLightTrackbar(index);
            return;
        }
    }

    for (int index = 0; index < CrosshairTrackCount; ++index)
    {
        if (trackbar == m_crosshairTrackbars[index])
        {
            HandleCrosshairTrackbar(index);
            return;
        }
    }

    if (m_syncingEditor || m_models.empty() || !trackbar)
    {
        return;
    }

    ModelInstance& model = m_models[m_selectedModel];

    for (int index = 0; index < TrackCount; ++index)
    {
        if (trackbar != m_trackbars[index])
        {
            continue;
        }

        const int value = static_cast<int>(SendMessageW(trackbar, TBM_GETPOS, 0, 0));
        if (model.rotationLocked && index >= TrackRotX && index <= TrackRotZ)
        {
            SendMessageW(trackbar, TBM_SETPOS, TRUE, 0);
            UpdateTrackLabels();
            return;
        }

        switch (index)
        {
        case TrackPosX:
            model.position.x = static_cast<float>(value) / 10.0f;
            break;
        case TrackPosY:
            model.position.y = static_cast<float>(value) / 10.0f;
            break;
        case TrackPosZ:
            model.position.z = static_cast<float>(value) / 10.0f;
            break;
        case TrackRotX:
            model.rotation.x = static_cast<float>(value) / 100.0f;
            break;
        case TrackRotY:
            model.rotation.y = static_cast<float>(value) / 100.0f;
            break;
        case TrackRotZ:
            model.rotation.z = static_cast<float>(value) / 100.0f;
            break;
        case TrackScale:
            model.scale = XMFLOAT3{
                static_cast<float>(value) / 10.0f,
                static_cast<float>(value) / 10.0f,
                static_cast<float>(value) / 10.0f};
            break;
        case TrackColorR:
            model.tint.x = static_cast<float>(value) / 100.0f;
            break;
        case TrackColorG:
            model.tint.y = static_cast<float>(value) / 100.0f;
            break;
        case TrackColorB:
            model.tint.z = static_cast<float>(value) / 100.0f;
            break;
        }

        UpdateTrackLabels();
        return;
    }
}

void Dx12App::HandleCrosshairTrackbar(int index)
{
    if (!m_crosshairTrackbars[index])
    {
        return;
    }

    const int value = static_cast<int>(SendMessageW(m_crosshairTrackbars[index], TBM_GETPOS, 0, 0));
    switch (index)
    {
    case CrosshairSensitivity:
        m_crosshair.gameSensitivity = static_cast<float>(value) / 100000.0f;
        break;
    case CrosshairSize:
        m_crosshair.size = CrosshairSizeFromTrack(value);
        break;
    case CrosshairColorR:
        m_crosshair.color.x = static_cast<float>(value) / 100.0f;
        break;
    case CrosshairColorG:
        m_crosshair.color.y = static_cast<float>(value) / 100.0f;
        break;
    case CrosshairColorB:
        m_crosshair.color.z = static_cast<float>(value) / 100.0f;
        break;
    }

    UpdateCrosshairLabels();
}

int Dx12App::BackgroundGrayTrackValue() const
{
    const float gray = std::clamp(
        (m_pipelineSettings.clearColor[0] + m_pipelineSettings.clearColor[1] + m_pipelineSettings.clearColor[2]) / 3.0f,
        0.0f,
        1.0f);
    return static_cast<int>(std::lround((1.0f - gray) * 100.0f));
}

void Dx12App::HandleBackgroundGrayTrackbar()
{
    if (!m_backgroundGrayTrackbar)
    {
        return;
    }

    const int value = static_cast<int>(SendMessageW(m_backgroundGrayTrackbar, TBM_GETPOS, 0, 0));
    const float gray = 1.0f - (static_cast<float>(value) / 100.0f);
    m_pipelineSettings.clearColor = {gray, gray, gray, 1.0f};
    UpdateBackgroundGrayLabel();
    UpdateWindowTitle();
}

void Dx12App::HandleLightTrackbar(int index)
{
    if (!m_lightTrackbars[index])
    {
        return;
    }

    const int value = static_cast<int>(SendMessageW(m_lightTrackbars[index], TBM_GETPOS, 0, 0));
    switch (index)
    {
    case LightYaw:
        m_light.yawDegrees = static_cast<float>(value);
        break;
    case LightPitch:
        m_light.pitchDegrees = static_cast<float>(value);
        break;
    case LightIntensity:
        m_light.intensity = static_cast<float>(value) / 100.0f;
        break;
    case LightAmbient:
        m_light.ambient = static_cast<float>(value) / 100.0f;
        break;
    case LightColorR:
        m_light.color.x = static_cast<float>(value) / 100.0f;
        break;
    case LightColorG:
        m_light.color.y = static_cast<float>(value) / 100.0f;
        break;
    case LightColorB:
        m_light.color.z = static_cast<float>(value) / 100.0f;
        break;
    }

    UpdateLightLabels();
}

void Dx12App::SyncBackgroundGrayTrackbar()
{
    if (!m_backgroundGrayTrackbar)
    {
        return;
    }

    SendMessageW(m_backgroundGrayTrackbar, TBM_SETPOS, TRUE, BackgroundGrayTrackValue());
    UpdateBackgroundGrayLabel();
}

void Dx12App::UpdateBackgroundGrayLabel()
{
    if (!m_backgroundGrayLabel)
    {
        return;
    }

    wchar_t buffer[96]{};
    swprintf_s(buffer, L"背景灰度：%d%% 黑", BackgroundGrayTrackValue());
    SetWindowTextW(m_backgroundGrayLabel, buffer);
}

void Dx12App::UpdateLightLabels()
{
    if (!m_lightLabels[0])
    {
        return;
    }

    wchar_t buffer[96]{};
    swprintf_s(buffer, L"光源水平角：%.0f°", m_light.yawDegrees);
    SetWindowTextW(m_lightLabels[LightYaw], buffer);
    swprintf_s(buffer, L"光源俯仰角：%.0f°", m_light.pitchDegrees);
    SetWindowTextW(m_lightLabels[LightPitch], buffer);
    swprintf_s(buffer, L"主光强度：%.2f", m_light.intensity);
    SetWindowTextW(m_lightLabels[LightIntensity], buffer);
    swprintf_s(buffer, L"环境光：%.2f", m_light.ambient);
    SetWindowTextW(m_lightLabels[LightAmbient], buffer);
    swprintf_s(buffer, L"光源颜色 R：%.2f", m_light.color.x);
    SetWindowTextW(m_lightLabels[LightColorR], buffer);
    swprintf_s(buffer, L"光源颜色 G：%.2f", m_light.color.y);
    SetWindowTextW(m_lightLabels[LightColorG], buffer);
    swprintf_s(buffer, L"光源颜色 B：%.2f", m_light.color.z);
    SetWindowTextW(m_lightLabels[LightColorB], buffer);

    if (m_lightFluctuationButton)
    {
        SetWindowTextW(m_lightFluctuationButton, m_light.fluctuation ? L"光照波动：开启" : L"光照波动：关闭");
        SendMessageW(m_lightFluctuationButton, BM_SETCHECK, m_light.fluctuation ? BST_CHECKED : BST_UNCHECKED, 0);
    }
}

void Dx12App::HandleKeyPressed(WPARAM key)
{
    switch (key)
    {
    case VK_ESCAPE:
        DestroyWindow(m_hwnd);
        break;

    case VK_F11:
        ToggleFullscreen();
        break;

    case VK_TAB:
        SetEditorVisible(!m_editorVisible);
        break;

    case 'T':
        if (m_gameActive)
        {
            StopAimGame();
        }
        else
        {
            m_pipelineSettings.autoRotate = !m_pipelineSettings.autoRotate;
            UpdateWindowTitle();
        }
        break;

    case 'F':
        RebuildPipelineWith([this](PipelineSettings& settings) {
            settings.wireframe = !m_pipelineSettings.wireframe;
        });
        break;

    case 'C':
        RebuildPipelineWith([this](PipelineSettings& settings) {
            settings.backfaceCulling = !m_pipelineSettings.backfaceCulling;
        });
        break;

    case 'R':
        ReloadEditablePipeline();
        break;
    }
}

template <typename Mutator>
void Dx12App::RebuildPipelineWith(Mutator mutate)
{
    // 只在新 PSO 创建成功后替换旧状态，失败时当前画面仍可继续使用。
    PipelineSettings nextSettings = m_pipelineSettings;
    mutate(nextSettings);

    try
    {
        WaitForGpu();
        ComPtr<ID3D12PipelineState> nextPipelineState = CreatePipelineState(nextSettings);
        m_pipelineSettings = nextSettings;
        m_pipelineState = nextPipelineState;
        SyncBackgroundGrayTrackbar();
        UpdateWindowTitle();
    }
    catch (const std::exception& error)
    {
        MessageBoxA(m_hwnd, error.what(), "Pipeline rebuild failed", MB_OK | MB_ICONERROR);
    }
}

void Dx12App::ReloadEditablePipeline()
{
    try
    {
        WaitForGpu();
        const PipelineSettings nextSettings = LoadPipelineSettings();
        ComPtr<ID3D12PipelineState> nextPipelineState = CreatePipelineState(nextSettings);
        m_pipelineSettings = nextSettings;
        m_pipelineState = nextPipelineState;
        SyncBackgroundGrayTrackbar();
        UpdateWindowTitle();
    }
    catch (const std::exception& error)
    {
        MessageBoxA(m_hwnd, error.what(), "Reload failed", MB_OK | MB_ICONERROR);
    }
}

void Dx12App::CreateAppWindow(HINSTANCE instance, int showCommand)
{
    const wchar_t* className = L"Dx12ModelEditorWindow";

    // 默认全屏启动，窗口样式只负责宿主 DX12 画布和 Win32 子控件。
    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(WNDCLASSEXW);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    windowClass.lpszClassName = className;
    ThrowIfFailed(RegisterClassExW(&windowClass) ? S_OK : HRESULT_FROM_WIN32(GetLastError()));

    MONITORINFO monitorInfo{};
    monitorInfo.cbSize = sizeof(monitorInfo);
    GetMonitorInfoW(MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTOPRIMARY), &monitorInfo);

    m_windowedRect = RECT{80, 80, 80 + static_cast<LONG>(WindowWidth), 80 + static_cast<LONG>(WindowHeight)};
    m_fullscreenRect = monitorInfo.rcMonitor;
    m_isFullscreen = true;

    m_hwnd = CreateWindowExW(
        0,
        className,
        AppDisplayName,
        WS_POPUP | WS_CLIPCHILDREN,
        m_fullscreenRect.left,
        m_fullscreenRect.top,
        m_fullscreenRect.right - m_fullscreenRect.left,
        m_fullscreenRect.bottom - m_fullscreenRect.top,
        nullptr,
        nullptr,
        instance,
        this);

    ThrowIfFailed(m_hwnd ? S_OK : HRESULT_FROM_WIN32(GetLastError()));

    RECT clientRect{};
    GetClientRect(m_hwnd, &clientRect);
    m_renderWidth = std::max<UINT>(1, static_cast<UINT>(clientRect.right - clientRect.left));
    m_renderHeight = std::max<UINT>(1, static_cast<UINT>(clientRect.bottom - clientRect.top));

    RAWINPUTDEVICE rawMouse{};
    // 即使鼠标被裁剪/隐藏，也能持续收到相对移动量。
    rawMouse.usUsagePage = 0x01;
    rawMouse.usUsage = 0x02;
    rawMouse.dwFlags = RIDEV_INPUTSINK;
    rawMouse.hwndTarget = m_hwnd;
    ThrowIfFailed(RegisterRawInputDevices(&rawMouse, 1, sizeof(rawMouse))
        ? S_OK
        : HRESULT_FROM_WIN32(GetLastError()));

    ShowWindow(m_hwnd, showCommand);
    UpdateWindow(m_hwnd);
}

HWND Dx12App::CreateChild(
    const wchar_t* className,
    const wchar_t* text,
    DWORD style,
    int x,
    int y,
    int width,
    int height,
    int id)
{
    // 所有编辑控件都记录所属模块，折叠/滚动时只显示当前模块。
    HWND control = CreateWindowExW(
        0,
        className,
        text,
        WS_CHILD | WS_VISIBLE | style,
        x,
        y,
        width,
        height,
        m_hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        m_instance,
        nullptr);

    ThrowIfFailed(control ? S_OK : HRESULT_FROM_WIN32(GetLastError()));
    m_editorControls.push_back(control);
    if (m_currentCreatedSection != EditorSection::None)
    {
        m_sectionControls[static_cast<size_t>(m_currentCreatedSection)].push_back(control);
        m_sectionControlPlacements.push_back(EditorControlPlacement{
            control,
            RECT{x, y, x + width, y + height},
            m_currentCreatedSection});
    }
    else
    {
        m_fixedEditorControls.push_back(control);
    }
    return control;
}

HWND Dx12App::CreateHudLabel(const wchar_t* text, int x, int y, int width)
{
    HWND control = CreateWindowExW(
        0,
        L"STATIC",
        text,
        WS_CHILD | SS_LEFT,
        x,
        y,
        width,
        24,
        m_hwnd,
        nullptr,
        m_instance,
        nullptr);

    ThrowIfFailed(control ? S_OK : HRESULT_FROM_WIN32(GetLastError()));
    m_gameHudControls.push_back(control);
    return control;
}

void Dx12App::CreateGameHudControls()
{
    // 游戏中默认只保留最关键 HUD：帧数、分数、命中率和剩余时间。
    const int x = 18;
    const int y = 16;
    const int width = 180;
    m_hudFpsLabel = CreateHudLabel(L"FPS：0.0", x, y, width);
    m_hudScoreLabel = CreateHudLabel(L"分数：0", x, y + 26, width);
    m_hudHitRateLabel = CreateHudLabel(L"命中率：0.0%", x, y + 52, width);
    m_hudTimeLabel = CreateHudLabel(L"时间：60.0", x, y + 78, width);
    m_hudNoonPromptLabel = CreateWindowExW(
        0,
        L"STATIC",
        L"",
        WS_CHILD | SS_CENTER | SS_CENTERIMAGE,
        0,
        44,
        680,
        76,
        m_hwnd,
        nullptr,
        m_instance,
        nullptr);
    ThrowIfFailed(m_hudNoonPromptLabel ? S_OK : HRESULT_FROM_WIN32(GetLastError()));
    m_gameHudControls.push_back(m_hudNoonPromptLabel);
    m_noonPromptFont = CreateFontW(
        46,
        0,
        0,
        0,
        FW_SEMIBOLD,
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_SWISS,
        L"Microsoft YaHei UI");
    if (m_noonPromptFont)
    {
        SendMessageW(m_hudNoonPromptLabel, WM_SETFONT, reinterpret_cast<WPARAM>(m_noonPromptFont), TRUE);
    }
    UpdateGameHudVisibility();
}
