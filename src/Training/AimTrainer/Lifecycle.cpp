#include "../../App/Dx12App.h"

void Dx12App::SelectAimMode(AimMode mode)
{
    if (m_gameActive)
    {
        StopAimGame();
    }

    m_gameMode = mode;
    m_gameDifficulty = m_modeDifficulties[ModeIndex(m_gameMode)];
    UpdateAimTrainerLabels();
}

void Dx12App::SelectAimDifficulty(AimDifficulty difficulty)
{
    if (m_gameActive)
    {
        StopAimGame();
    }

    m_gameDifficulty = difficulty;
    m_modeDifficulties[ModeIndex(m_gameMode)] = difficulty;
    UpdateAimTrainerLabels();
}

const AimModeConfig& Dx12App::CurrentAimConfig() const
{
    return AimConfigs[ModeIndex(m_gameMode)][DifficultyIndex(m_gameDifficulty)];
}

void Dx12App::StartAimGame()
{
    // 进入游戏前保存编辑状态，60 秒结束或手动退出时完整还原。
    if (!m_gameSavedState.valid)
    {
        m_gameSavedState.cameraPosition = m_cameraPosition;
        m_gameSavedState.aimWallScale = m_aimWallModelIndex < m_models.size()
            ? m_models[m_aimWallModelIndex].scale
            : XMFLOAT3{1.0f, 1.0f, 1.0f};
        m_gameSavedState.cameraYaw = m_cameraYaw;
        m_gameSavedState.cameraPitch = m_cameraPitch;
        m_gameSavedState.editorVisible = m_editorVisible;
        m_gameSavedState.valid = true;
    }

    m_gameActive = true;
    m_leftFiring = false;
    m_ignoreNextGameMouseMove = false;
    m_hasRecentAimMove = false;
    m_recentAimVelocityOnPlane = XMFLOAT2{};
    m_recentAimMoveTime = -1000.0f;
    m_lastAimMoveClock = std::chrono::steady_clock::now();
    m_hasAimMoveClock = true;
    m_gameScore = 0;
    m_fractionalScore = 0.0f;
    m_shotsFired = 0;
    m_shotsHit = 0;
    m_trackingFireTime = 0.0f;
    m_trackingHitTime = 0.0f;
    m_balloonSpawnTimer = 0.0f;
    m_gameRemainingSeconds = m_gameMode == AimMode::Noon ? 0.0f : 60.0f;
    m_nextAimTrainerLabelUpdateTime = 0.0f;
    m_gameTargets.clear();
    ResetNoonState();
    if (m_aimWallModelIndex < m_models.size())
    {
        m_models[m_aimWallModelIndex].scale.x = GameWallScaleX();
    }
    m_cameraPosition = XMFLOAT3{0.0f, AimWallCenterY, GameCameraZ()};
    m_cameraYaw = 0.0f;
    m_cameraPitch = 0.0f;
    m_keys = {};
    m_mouseLook = false;
    SetCrosshairEditorEnabled(false);
    SetEditorVisible(false);
    SetCapture(m_hwnd);
    // 游戏内隐藏并限制鼠标，视角只由相对位移驱动。
    RECT clipRect{};
    GetClientRect(m_hwnd, &clipRect);
    POINT clipTopLeft{clipRect.left, clipRect.top};
    POINT clipBottomRight{clipRect.right, clipRect.bottom};
    ClientToScreen(m_hwnd, &clipTopLeft);
    ClientToScreen(m_hwnd, &clipBottomRight);
    clipRect = RECT{clipTopLeft.x, clipTopLeft.y, clipBottomRight.x, clipBottomRight.y};
    ClipCursor(&clipRect);
    ShowCursor(FALSE);
    CenterMouseForGame();

    if (m_gameMode == AimMode::Balloon)
    {
        SpawnBalloonTarget();
    }
    else if (m_gameMode == AimMode::Enemy)
    {
        while (CountActiveTargets() < CurrentAimConfig().targetCount)
        {
            SpawnEnemyTarget();
        }
    }
    else if (m_gameMode == AimMode::Noon)
    {
        BeginNoonRound();
    }
    else
    {
        SpawnTrackingTarget();
    }

    UpdateAimTrainerLabels();
}

void Dx12App::StopAimGame()
{
    if (!m_gameActive && !m_gameSavedState.valid)
    {
        UpdateAimTrainerLabels();
        return;
    }

    m_gameActive = false;
    m_leftFiring = false;
    m_ignoreNextGameMouseMove = false;
    m_gameTargets.clear();
    ResetNoonState();
    SaveHighScoresNoThrow();
    RestoreAfterGame();
    UpdateAimTrainerLabels();
}

void Dx12App::FinishAimGame()
{
    m_gameActive = false;
    m_leftFiring = false;
    m_ignoreNextGameMouseMove = false;
    m_gameTargets.clear();
    ResetNoonState();
    SaveHighScoresNoThrow();
    RestoreAfterGame();
    UpdateAimTrainerLabels();
}

void Dx12App::RestoreAfterGame()
{
    // 恢复鼠标、准心编辑和进入游戏前的相机/墙体缩放。
    ClipCursor(nullptr);
    if (GetCapture() == m_hwnd)
    {
        ReleaseCapture();
    }
    ShowCursor(TRUE);
    SetCrosshairEditorEnabled(true);

    if (m_gameSavedState.valid)
    {
        m_cameraPosition = m_gameSavedState.cameraPosition;
        m_cameraYaw = m_gameSavedState.cameraYaw;
        m_cameraPitch = m_gameSavedState.cameraPitch;
        if (m_aimWallModelIndex < m_models.size())
        {
            m_models[m_aimWallModelIndex].scale = m_gameSavedState.aimWallScale;
        }
        SetEditorVisible(m_gameSavedState.editorVisible);
        m_gameSavedState.valid = false;
    }
}

void Dx12App::CenterMouseForGame()
{
    // 旧鼠标消息路径会用到居中；Raw Input 路径仍保持中心射击坐标。
    POINT center{
        static_cast<LONG>(m_renderWidth / 2),
        static_cast<LONG>(m_renderHeight / 2)};
    ClientToScreen(m_hwnd, &center);

    POINT cursor{};
    GetCursorPos(&cursor);
    if ((cursor.x != center.x || cursor.y != center.y) && SetCursorPos(center.x, center.y))
    {
        m_ignoreNextGameMouseMove = true;
    }
    else
    {
        m_ignoreNextGameMouseMove = false;
    }
    m_lastAimX = static_cast<int>(m_renderWidth / 2);
    m_lastAimY = static_cast<int>(m_renderHeight / 2);
}
