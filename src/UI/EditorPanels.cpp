#include "../App/Dx12App.h"

void Dx12App::CreateEditorControls()
{
    // 左侧编辑器分为固定头部和可折叠正文，正文控件会记录原始布局。
    const int x = 16;
    int y = 14;
    const int width = static_cast<int>(EditorPanelWidth) - 32;

    CreateChild(L"STATIC", L"渲染管线编辑器", 0, x, y, 152, 22, 0);
    m_fpsLabel = CreateChild(L"STATIC", L"当前帧数：0.0 FPS", 0, x + 154, y, width - 154, 22, 0);
    y += 28;

    m_sectionButtons[static_cast<size_t>(EditorSection::Model)] =
        CreateChild(L"BUTTON", L"▼ 模型编辑", BS_PUSHBUTTON | BS_LEFT, x, y, width, 26, IdSectionModel);
    y += 28;
    m_sectionButtons[static_cast<size_t>(EditorSection::Aim)] =
        CreateChild(L"BUTTON", L"▶ 训练设置", BS_PUSHBUTTON | BS_LEFT, x, y, width, 26, IdSectionAim);
    y += 28;
    m_sectionButtons[static_cast<size_t>(EditorSection::Light)] =
        CreateChild(L"BUTTON", L"▶ 光源控制", BS_PUSHBUTTON | BS_LEFT, x, y, width, 26, IdSectionLight);
    y += 34;

    const int bodyY = y;
    m_editorBodyTop = bodyY;
    m_editorScrollbar = CreateChild(
        L"SCROLLBAR",
        L"",
        SBS_VERT,
        x + width + 2,
        m_editorBodyTop,
        14,
        std::max(96, static_cast<int>(m_renderHeight) - m_editorBodyTop - 16),
        IdEditorScroll);
    ShowWindow(m_editorScrollbar, SW_HIDE);

    m_currentCreatedSection = EditorSection::Model;
    CreateChild(L"STATIC", L"模型列表", 0, x, y, width, 20, 0);
    y += 22;

    m_modelList = CreateChild(
        L"LISTBOX",
        L"",
        LBS_NOTIFY | WS_BORDER | WS_VSCROLL,
        x,
        y,
        width,
        112,
        IdModelList);
    y += 122;

    m_visibleCheck = CreateChild(
        L"BUTTON",
        L"显示",
        BS_AUTOCHECKBOX,
        x,
        y,
        96,
        24,
        IdVisible);
    CreateChild(L"BUTTON", L"重置模型", BS_PUSHBUTTON, x + 112, y, 118, 24, IdResetModel);
    y += 32;

    for (int index = 0; index < TrackCount; ++index)
    {
        m_trackLabels[index] = CreateChild(L"STATIC", TrackConfigs[index].label, 0, x, y, width, 18, 0);
        y += 18;

        m_trackbars[index] = CreateChild(
            TRACKBAR_CLASSW,
            L"",
            TBS_AUTOTICKS,
            x,
            y,
            width,
            30,
            IdTrackBase + index);
        SendMessageW(m_trackbars[index], TBM_SETRANGE, TRUE, MAKELPARAM(TrackConfigs[index].minValue, TrackConfigs[index].maxValue));
        SendMessageW(m_trackbars[index], TBM_SETLINESIZE, 0, 1);
        SendMessageW(m_trackbars[index], TBM_SETPAGESIZE, 0, 5);
        y += 34;
    }

    CreateChild(L"BUTTON", L"线框", BS_PUSHBUTTON, x, y, 100, 26, IdToggleWireframe);
    CreateChild(L"BUTTON", L"重载", BS_PUSHBUTTON, x + 108, y, 88, 26, IdReloadPipeline);
    CreateChild(L"BUTTON", L"隐藏界面", BS_PUSHBUTTON, x + 204, y, 88, 26, IdToggleUi);
    y += 34;

    CreateChild(L"BUTTON", L"全屏切换", BS_PUSHBUTTON, x, y, 132, 26, IdToggleFullscreen);
    m_backgroundGrayLabel = CreateChild(L"STATIC", L"背景灰度：0% 黑", 0, x + 144, y + 4, width - 144, 20, 0);
    y += 30;

    m_backgroundGrayTrackbar = CreateChild(
        TRACKBAR_CLASSW,
        L"",
        TBS_AUTOTICKS,
        x,
        y,
        width,
        30,
        IdBackgroundGray);
    SendMessageW(m_backgroundGrayTrackbar, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
    SendMessageW(m_backgroundGrayTrackbar, TBM_SETLINESIZE, 0, 1);
    SendMessageW(m_backgroundGrayTrackbar, TBM_SETPAGESIZE, 0, 5);
    SendMessageW(m_backgroundGrayTrackbar, TBM_SETPOS, TRUE, BackgroundGrayTrackValue());
    y += 38;

    CreateChild(
        L"STATIC",
        L"右键拖动：视角 | WASD：移动 | 空格/Ctrl：升降 | 方向键：旋转选中模型 | F11：全屏 | Tab：界面",
        0,
        x,
        y,
        width,
        58,
        0);

    m_currentCreatedSection = EditorSection::None;
    CreateAimTrainerControls(x, bodyY, width);
    CreateLightControls(x, bodyY, width);
    SetActiveEditorSection(EditorSection::Model);
}

void Dx12App::CreateAimTrainerControls(int x, int y, int width)
{
    // 训练模块包含模式/难度选择、分数显示和非游戏状态下的准心编辑。
    m_currentCreatedSection = EditorSection::Aim;

    CreateChild(L"STATIC", L"练枪训练", 0, x, y, width, 22, 0);
    y += 30;

    m_gameModeLabel = CreateChild(L"STATIC", L"当前模式：快枪手", 0, x, y, width, 20, 0);
    y += 28;

    CreateChild(L"BUTTON", L"快枪手", BS_PUSHBUTTON, x, y, 86, 28, IdModeBalloon);
    CreateChild(L"BUTTON", L"游侠之眼", BS_PUSHBUTTON, x + 94, y, 86, 28, IdModeEnemy);
    CreateChild(L"BUTTON", L"聚焦火力", BS_PUSHBUTTON, x + 188, y, 86, 28, IdModeTracking);
    y += 34;
    CreateChild(L"BUTTON", L"午时已到", BS_PUSHBUTTON, x, y, 118, 28, IdModeNoon);
    y += 38;

    m_gameDifficultyLabel = CreateChild(L"STATIC", L"当前难度：困难", 0, x, y, width, 20, 0);
    y += 26;

    CreateChild(L"BUTTON", L"简单", BS_PUSHBUTTON, x, y, 86, 28, IdDifficultyEasy);
    CreateChild(L"BUTTON", L"中等", BS_PUSHBUTTON, x + 94, y, 86, 28, IdDifficultyMedium);
    CreateChild(L"BUTTON", L"困难", BS_PUSHBUTTON, x + 188, y, 86, 28, IdDifficultyHard);
    y += 38;

    CreateChild(L"BUTTON", L"开始训练", BS_PUSHBUTTON, x, y, 96, 30, IdStartGame);
    CreateChild(L"BUTTON", L"停止训练", BS_PUSHBUTTON, x + 106, y, 96, 30, IdStopGame);
    CreateChild(L"BUTTON", L"清空最高", BS_PUSHBUTTON, x + 212, y, 82, 30, IdResetHighScores);
    y += 42;

    m_gameStateLabel = CreateChild(L"STATIC", L"状态：未开始", 0, x, y, width, 20, 0);
    y += 24;
    m_gameScoreLabel = CreateChild(L"STATIC", L"当前分数：0", 0, x, y, width, 20, 0);
    y += 24;
    m_gameHighScoreLabel = CreateChild(L"STATIC", L"历史最高：0", 0, x, y, width, 20, 0);
    y += 34;

    CreateChild(L"STATIC", L"模式说明", 0, x, y, width, 20, 0);
    y += 24;
    m_gameHintLabel = CreateChild(
        L"STATIC",
        L"目标快速出现并膨胀收缩，越早击破分越高。",
        0,
        x,
        y,
        width,
        86,
        0);
    y += 96;

    CreateChild(
        L"STATIC",
        L"左键射击/跟枪；右键拖动视角。分数会保存到 assets\\aim_high_scores.txt。",
        0,
        x,
        y,
        width,
        58,
        0);
    y += 70;

    CreateChild(L"STATIC", L"准心编辑（非游戏状态）", 0, x, y, width, 20, 0);
    y += 24;

    CreateCrosshairTrack(x, y, width, CrosshairSensitivity, L"游戏视角灵敏度", 1, 450, static_cast<int>(m_crosshair.gameSensitivity * 100000.0f));
    y += 52;
    CreateCrosshairTrack(x, y, width, CrosshairSize, L"准心大小", CrosshairSizeTrackMin, CrosshairSizeTrackMax, CrosshairTrackFromSize(m_crosshair.size));
    y += 52;
    CreateCrosshairTrack(x, y, width, CrosshairColorR, L"准心颜色 R", 0, 100, static_cast<int>(m_crosshair.color.x * 100.0f));
    y += 52;
    CreateCrosshairTrack(x, y, width, CrosshairColorG, L"准心颜色 G", 0, 100, static_cast<int>(m_crosshair.color.y * 100.0f));
    y += 52;
    CreateCrosshairTrack(x, y, width, CrosshairColorB, L"准心颜色 B", 0, 100, static_cast<int>(m_crosshair.color.z * 100.0f));

    UpdateAimTrainerLabels();
    UpdateCrosshairLabels();
    m_currentCreatedSection = EditorSection::None;
}

void Dx12App::CreateCrosshairTrack(int x, int y, int width, int index, const wchar_t* label, int minValue, int maxValue, int initialValue)
{
    m_crosshairLabels[index] = CreateChild(L"STATIC", label, 0, x, y, width, 18, 0);
    m_crosshairTrackbars[index] = CreateChild(
        TRACKBAR_CLASSW,
        L"",
        TBS_AUTOTICKS,
        x,
        y + 18,
        width,
        30,
        IdCrosshairTrackBase + index);
    SendMessageW(m_crosshairTrackbars[index], TBM_SETRANGE, TRUE, MAKELPARAM(minValue, maxValue));
    SendMessageW(m_crosshairTrackbars[index], TBM_SETLINESIZE, 0, 1);
    SendMessageW(m_crosshairTrackbars[index], TBM_SETPAGESIZE, 0, 5);
    SendMessageW(m_crosshairTrackbars[index], TBM_SETPOS, TRUE, initialValue);
}

void Dx12App::CreateLightControls(int x, int y, int width)
{
    // 光源模块打开时，渲染层会显示对应的范围线框辅助观察。
    m_currentCreatedSection = EditorSection::Light;

    CreateChild(L"STATIC", L"光源控制", 0, x, y, width, 22, 0);
    y += 30;
    CreateChild(
        L"STATIC",
        L"调整方向、强度、环境光和颜色；打开本模块时会显示光照范围线框。",
        0,
        x,
        y,
        width,
        42,
        0);
    y += 54;

    m_lightFluctuationButton = CreateChild(
        L"BUTTON",
        m_light.fluctuation ? L"光照波动：开启" : L"光照波动：关闭",
        BS_AUTOCHECKBOX,
        x,
        y,
        160,
        26,
        IdToggleLightFluctuation);
    SendMessageW(m_lightFluctuationButton, BM_SETCHECK, m_light.fluctuation ? BST_CHECKED : BST_UNCHECKED, 0);
    y += 38;

    CreateLightTrack(x, y, width, LightYaw, L"光源水平角", 0, 360, static_cast<int>(std::lround(m_light.yawDegrees)));
    y += 52;
    CreateLightTrack(x, y, width, LightPitch, L"光源俯仰角", -80, 80, static_cast<int>(std::lround(m_light.pitchDegrees)));
    y += 52;
    CreateLightTrack(x, y, width, LightIntensity, L"主光强度", 0, 200, static_cast<int>(std::lround(m_light.intensity * 100.0f)));
    y += 52;
    CreateLightTrack(x, y, width, LightAmbient, L"环境光", 0, 100, static_cast<int>(std::lround(m_light.ambient * 100.0f)));
    y += 52;
    CreateLightTrack(x, y, width, LightColorR, L"光源颜色 R", 0, 100, static_cast<int>(std::lround(m_light.color.x * 100.0f)));
    y += 52;
    CreateLightTrack(x, y, width, LightColorG, L"光源颜色 G", 0, 100, static_cast<int>(std::lround(m_light.color.y * 100.0f)));
    y += 52;
    CreateLightTrack(x, y, width, LightColorB, L"光源颜色 B", 0, 100, static_cast<int>(std::lround(m_light.color.z * 100.0f)));

    UpdateLightLabels();
    m_currentCreatedSection = EditorSection::None;
}

void Dx12App::CreateLightTrack(int x, int y, int width, int index, const wchar_t* label, int minValue, int maxValue, int initialValue)
{
    m_lightLabels[index] = CreateChild(L"STATIC", label, 0, x, y, width, 18, 0);
    m_lightTrackbars[index] = CreateChild(
        TRACKBAR_CLASSW,
        L"",
        TBS_AUTOTICKS,
        x,
        y + 18,
        width,
        30,
        IdLightTrackBase + index);
    SendMessageW(m_lightTrackbars[index], TBM_SETRANGE, TRUE, MAKELPARAM(minValue, maxValue));
    SendMessageW(m_lightTrackbars[index], TBM_SETLINESIZE, 0, 1);
    SendMessageW(m_lightTrackbars[index], TBM_SETPAGESIZE, 0, 5);
    SendMessageW(m_lightTrackbars[index], TBM_SETPOS, TRUE, initialValue);
}

void Dx12App::SetEditorVisible(bool visible)
{
    m_editorVisible = visible;
    ApplyEditorSectionVisibility();
    UpdateGameHudVisibility();
}

void Dx12App::SetActiveEditorSection(EditorSection section)
{
    // 切换模块时回到顶部，避免新模块因为上一个模块的滚动量而看起来空白。
    if (m_activeEditorSection != section)
    {
        m_editorScrollOffset = 0;
    }
    m_activeEditorSection = section;
    ApplyEditorSectionVisibility();
}

void Dx12App::ToggleEditorSection(EditorSection section)
{
    if (m_activeEditorSection != section)
    {
        m_editorScrollOffset = 0;
    }
    m_activeEditorSection = section;
    ApplyEditorSectionVisibility();
}

const wchar_t* Dx12App::EditorSectionTitle(EditorSection section) const
{
    switch (section)
    {
    case EditorSection::Model:
        return L"模型编辑";
    case EditorSection::Aim:
        return L"训练设置";
    case EditorSection::Light:
        return L"光源控制";
    default:
        return L"";
    }
}

void Dx12App::UpdateEditorSectionButtons()
{
    for (size_t sectionIndex = 0; sectionIndex < static_cast<size_t>(EditorSection::Count); ++sectionIndex)
    {
        HWND button = m_sectionButtons[sectionIndex];
        if (!button)
        {
            continue;
        }

        const EditorSection section = static_cast<EditorSection>(sectionIndex);
        const bool expanded = m_activeEditorSection == section;
        std::wstring title = expanded ? L"▼ " : L"▶ ";
        title += EditorSectionTitle(section);
        SetWindowTextW(button, title.c_str());
    }
}

int Dx12App::EditorViewportBottom() const
{
    return std::max(m_editorBodyTop + 96, static_cast<int>(m_renderHeight) - 16);
}

int Dx12App::ActiveEditorContentBottom() const
{
    if (m_activeEditorSection == EditorSection::None)
    {
        return m_editorBodyTop;
    }

    int bottom = m_editorBodyTop;
    for (const EditorControlPlacement& placement : m_sectionControlPlacements)
    {
        if (placement.section == m_activeEditorSection)
        {
            bottom = std::max(bottom, static_cast<int>(placement.rect.bottom));
        }
    }
    return bottom;
}

void Dx12App::UpdateEditorScrollbar()
{
    if (!m_editorScrollbar)
    {
        return;
    }

    const int viewportHeight = std::max(1, EditorViewportBottom() - m_editorBodyTop);
    // 滚动条范围只按当前展开模块计算，其他模块不会参与高度。
    const int contentHeight = std::max(viewportHeight, ActiveEditorContentBottom() - m_editorBodyTop);
    m_editorScrollMax = std::max(0, contentHeight - viewportHeight);
    m_editorScrollOffset = std::clamp(m_editorScrollOffset, 0, m_editorScrollMax);

    SCROLLINFO scrollInfo{};
    scrollInfo.cbSize = sizeof(scrollInfo);
    scrollInfo.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    scrollInfo.nMin = 0;
    scrollInfo.nMax = std::max(0, contentHeight - 1);
    scrollInfo.nPage = static_cast<UINT>(viewportHeight);
    scrollInfo.nPos = m_editorScrollOffset;
    SetScrollInfo(m_editorScrollbar, SB_CTL, &scrollInfo, TRUE);
    ShowWindow(m_editorScrollbar, m_editorVisible && m_editorScrollMax > 0 ? SW_SHOW : SW_HIDE);
}

void Dx12App::SetEditorScrollOffset(int offset)
{
    UpdateEditorScrollbar();
    const int nextOffset = std::clamp(offset, 0, m_editorScrollMax);
    if (nextOffset == m_editorScrollOffset)
    {
        return;
    }

    m_editorScrollOffset = nextOffset;
    ApplyEditorSectionVisibility();
}

void Dx12App::ScrollEditorBy(int delta)
{
    SetEditorScrollOffset(m_editorScrollOffset + delta);
}

void Dx12App::HandleEditorScroll(WORD request, WORD thumbPosition)
{
    const int viewportHeight = std::max(1, EditorViewportBottom() - m_editorBodyTop);
    const int lineStep = 36;
    const int pageStep = std::max(lineStep, viewportHeight - 48);
    int nextOffset = m_editorScrollOffset;

    switch (request)
    {
    case SB_LINEUP:
        nextOffset -= lineStep;
        break;
    case SB_LINEDOWN:
        nextOffset += lineStep;
        break;
    case SB_PAGEUP:
        nextOffset -= pageStep;
        break;
    case SB_PAGEDOWN:
        nextOffset += pageStep;
        break;
    case SB_THUMBPOSITION:
    case SB_THUMBTRACK:
        nextOffset = thumbPosition;
        break;
    case SB_TOP:
        nextOffset = 0;
        break;
    case SB_BOTTOM:
        nextOffset = m_editorScrollMax;
        break;
    default:
        return;
    }

    SetEditorScrollOffset(nextOffset);
}

bool Dx12App::HandleEditorMouseWheel(int x, int y, int wheelDelta)
{
    // 鼠标滚轮只在左侧面板内生效，避免影响场景视角操作。
    if (!m_editorVisible || x < 0 || x > static_cast<int>(EditorPanelWidth) || y < m_editorBodyTop)
    {
        return false;
    }

    UpdateEditorScrollbar();
    if (m_editorScrollMax <= 0)
    {
        return false;
    }

    ScrollEditorBy(wheelDelta > 0 ? -72 : 72);
    return true;
}

void Dx12App::ApplyEditorSectionVisibility()
{
    // 先隐藏全部编辑控件，再只恢复固定头部和当前模块，解决模块重叠问题。
    for (HWND control : m_editorControls)
    {
        ShowWindow(control, SW_HIDE);
    }

    if (m_editorVisible)
    {
        for (HWND control : m_fixedEditorControls)
        {
            if (control != m_editorScrollbar)
            {
                ShowWindow(control, SW_SHOW);
            }
        }
    }

    UpdateEditorScrollbar();
    const int viewportBottom = EditorViewportBottom();

    for (const EditorControlPlacement& placement : m_sectionControlPlacements)
    {
        const int width = placement.rect.right - placement.rect.left;
        const int height = placement.rect.bottom - placement.rect.top;
        const bool isActiveSection = m_editorVisible && placement.section == m_activeEditorSection;

        if (isActiveSection)
        {
            const int y = placement.rect.top - m_editorScrollOffset;
            const int bottom = y + height;
            MoveWindow(placement.control, placement.rect.left, y, width, height, TRUE);
            ShowWindow(placement.control, y >= m_editorBodyTop && bottom <= viewportBottom ? SW_SHOW : SW_HIDE);
        }
    }

    UpdateEditorSectionButtons();
    RECT panelRect{0, 0, static_cast<LONG>(EditorPanelWidth), static_cast<LONG>(m_renderHeight)};
    InvalidateRect(m_hwnd, &panelRect, TRUE);
}

void Dx12App::UpdateGameHudVisibility()
{
    const bool showHud = m_gameActive;
    for (HWND control : m_gameHudControls)
    {
        if (control == m_hudNoonPromptLabel)
        {
            const bool showPrompt = showHud && m_gameMode == AimMode::Noon && !m_noonPromptText.empty();
            ShowWindow(control, showPrompt ? SW_SHOW : SW_HIDE);
            continue;
        }
        ShowWindow(control, showHud ? SW_SHOW : SW_HIDE);
    }
}

void Dx12App::SetCrosshairEditorEnabled(bool enabled)
{
    for (HWND control : m_crosshairTrackbars)
    {
        if (control)
        {
            EnableWindow(control, enabled);
        }
    }
}

void Dx12App::UpdateCrosshairLabels()
{
    if (!m_crosshairLabels[0])
    {
        return;
    }

    wchar_t buffer[96]{};
    swprintf_s(buffer, L"游戏视角灵敏度：%.5f", m_crosshair.gameSensitivity);
    SetWindowTextW(m_crosshairLabels[CrosshairSensitivity], buffer);
    swprintf_s(buffer, L"准心大小：%.3f", m_crosshair.size);
    SetWindowTextW(m_crosshairLabels[CrosshairSize], buffer);
    swprintf_s(buffer, L"准心颜色 R：%.2f", m_crosshair.color.x);
    SetWindowTextW(m_crosshairLabels[CrosshairColorR], buffer);
    swprintf_s(buffer, L"准心颜色 G：%.2f", m_crosshair.color.y);
    SetWindowTextW(m_crosshairLabels[CrosshairColorG], buffer);
    swprintf_s(buffer, L"准心颜色 B：%.2f", m_crosshair.color.z);
    SetWindowTextW(m_crosshairLabels[CrosshairColorB], buffer);
}
