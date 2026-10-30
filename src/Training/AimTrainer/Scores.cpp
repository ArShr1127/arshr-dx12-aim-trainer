#include "../../App/Dx12App.h"

void Dx12App::ResetHighScores()
{
    m_highScores.values = {};
    m_highScoreDirty = true;
    SaveHighScoresNoThrow();
    UpdateAimTrainerLabels();
}

void Dx12App::AddScore(int amount)
{
    // 最高分按“模式 + 难度”分别保存，扣分也会立即影响当前分数。
    if (amount == 0)
    {
        return;
    }

    m_gameScore += amount;
    const size_t index = HighScoreIndex(m_gameMode, m_gameDifficulty);
    if (m_gameScore > m_highScores.values[index])
    {
        m_highScores.values[index] = m_gameScore;
        m_highScoreDirty = true;
    }
}

size_t Dx12App::CountActiveTargets() const
{
    return static_cast<size_t>(std::count_if(
        m_gameTargets.begin(),
        m_gameTargets.end(),
        [](const AimTarget& target) { return target.active && !target.eliminated; }));
}

std::wstring Dx12App::AimModeName(AimMode mode) const
{
    switch (mode)
    {
    case AimMode::Balloon:
        return L"快枪手";
    case AimMode::Enemy:
        return L"游侠之眼";
    case AimMode::Tracking:
        return L"聚焦火力";
    case AimMode::Noon:
        return L"午时已到";
    default:
        return L"未知";
    }
}

std::wstring Dx12App::AimDifficultyName(AimDifficulty difficulty) const
{
    switch (difficulty)
    {
    case AimDifficulty::Easy:
        return L"简单";
    case AimDifficulty::Medium:
        return L"中等";
    case AimDifficulty::Hard:
        return L"困难";
    default:
        return L"未知";
    }
}

std::wstring Dx12App::AimModeHint(AimMode mode) const
{
    switch (mode)
    {
    case AimMode::Balloon:
        return L"快枪手：目标更高频出现并较慢膨胀收缩，越早击破分越高，最大时为初始分的一半。";
    case AimMode::Enemy:
        return L"游侠之眼：多个目标 3 点血并频繁变向。中心命中秒杀，外圈命中扣 1 血；只有击杀得分，未命中扣分。";
    case AimMode::Tracking:
        return L"聚焦火力：按住左键持续跟枪，准星保持在目标内会持续加分。";
    case AimMode::Noon:
        return L"午时已到：每局 5 轮、每轮 6 发。目标一起出现，头部 400 分，躯干 200 分；清靶越快，对数倍率越高。";
    default:
        return L"";
    }
}

float Dx12App::CurrentHitRatePercent() const
{
    // 聚焦火力使用按住开火时间计算命中率，其他模式使用射击次数。
    if (m_gameMode == AimMode::Tracking)
    {
        if (m_trackingFireTime <= 0.0f)
        {
            return 0.0f;
        }
        return std::clamp((m_trackingHitTime / m_trackingFireTime) * 100.0f, 0.0f, 100.0f);
    }

    if (m_shotsFired <= 0)
    {
        return 0.0f;
    }

    return (static_cast<float>(m_shotsHit) / static_cast<float>(m_shotsFired)) * 100.0f;
}

void Dx12App::UpdateAimTrainerLabels()
{
    if (!m_gameModeLabel)
    {
        return;
    }

    const std::wstring modeText = L"当前模式：" + AimModeName(m_gameMode);
    SetWindowTextW(m_gameModeLabel, modeText.c_str());

    if (m_gameDifficultyLabel)
    {
        const std::wstring difficultyText = L"当前难度：" + AimDifficultyName(m_gameDifficulty);
        SetWindowTextW(m_gameDifficultyLabel, difficultyText.c_str());
    }

    wchar_t stateText[96]{};
    if (m_gameActive && m_gameMode == AimMode::Noon)
    {
        const wchar_t* phaseText = L"准备";
        if (m_noonPhase == NoonPhase::Waiting)
        {
            phaseText = L"等待";
        }
        else if (m_noonPhase == NoonPhase::Live)
        {
            phaseText = L"交火";
        }
        swprintf_s(
            stateText,
            L"状态：第 %d/%d 轮 %s | 子弹：%d",
            std::clamp(m_noonRoundIndex, 1, NoonRoundCount),
            NoonRoundCount,
            phaseText,
            std::max(0, m_noonBulletsRemaining));
    }
    else if (m_gameActive)
    {
        swprintf_s(stateText, L"状态：训练中  剩余：%.1f 秒", std::max(0.0f, m_gameRemainingSeconds));
    }
    else
    {
        swprintf_s(stateText, L"状态：未开始");
    }
    SetWindowTextW(m_gameStateLabel, stateText);

    wchar_t scoreText[96]{};
    swprintf_s(scoreText, L"当前分数：%d", m_gameScore);
    SetWindowTextW(m_gameScoreLabel, scoreText);

    wchar_t highScoreText[96]{};
    swprintf_s(highScoreText, L"历史最高：%d", m_highScores.values[HighScoreIndex(m_gameMode, m_gameDifficulty)]);
    SetWindowTextW(m_gameHighScoreLabel, highScoreText);

    const std::wstring hint = AimModeHint(m_gameMode);
    SetWindowTextW(m_gameHintLabel, hint.c_str());

    if (m_hudScoreLabel)
    {
        wchar_t hudScore[64]{};
        swprintf_s(hudScore, L"分数：%d", m_gameScore);
        SetWindowTextW(m_hudScoreLabel, hudScore);
    }
    if (m_hudHitRateLabel)
    {
        wchar_t hudHitRate[64]{};
        swprintf_s(hudHitRate, L"命中率：%.1f%%", CurrentHitRatePercent());
        SetWindowTextW(m_hudHitRateLabel, hudHitRate);
    }
    if (m_hudTimeLabel)
    {
        wchar_t hudTime[64]{};
        if (m_gameMode == AimMode::Noon)
        {
            swprintf_s(
                hudTime,
                L"轮次：%d/%d | 弹：%d",
                std::clamp(m_noonRoundIndex, 1, NoonRoundCount),
                NoonRoundCount,
                std::max(0, m_noonBulletsRemaining));
        }
        else
        {
            swprintf_s(hudTime, L"时间：%.1f", std::max(0.0f, m_gameRemainingSeconds));
        }
        SetWindowTextW(m_hudTimeLabel, hudTime);
    }

    if (m_hudNoonPromptLabel)
    {
        const int promptWidth = 680;
        const int promptHeight = 76;
        MoveWindow(
            m_hudNoonPromptLabel,
            std::max(0, static_cast<int>(m_renderWidth) / 2 - promptWidth / 2),
            44,
            promptWidth,
            promptHeight,
            TRUE);
        SetWindowTextW(m_hudNoonPromptLabel, m_noonPromptText.c_str());
        ShowWindow(
            m_hudNoonPromptLabel,
            m_gameActive && m_gameMode == AimMode::Noon && !m_noonPromptText.empty() ? SW_SHOW : SW_HIDE);
        InvalidateRect(m_hudNoonPromptLabel, nullptr, TRUE);
    }
}

std::filesystem::path Dx12App::HighScorePath() const
   {
    return m_userDataRoot / "aim_high_scores.txt";
   }

std::string Dx12App::AimModeKey(AimMode mode) const
{
    switch (mode)
    {
    case AimMode::Balloon:
        return "balloon";
    case AimMode::Enemy:
        return "enemy";
    case AimMode::Tracking:
        return "tracking";
    case AimMode::Noon:
        return "noon";
    default:
        return "unknown";
    }
}

std::string Dx12App::AimDifficultyKey(AimDifficulty difficulty) const
{
    switch (difficulty)
    {
    case AimDifficulty::Easy:
        return "easy";
    case AimDifficulty::Medium:
        return "medium";
    case AimDifficulty::Hard:
        return "hard";
    default:
        return "unknown";
    }
}

void Dx12App::LoadHighScores()
{
    // 兼容旧版只有模式名的存档，同时读取新版“模式_难度”格式。
    m_highScores.values = {};
    std::ifstream file{HighScorePath()};
    if (!file)
    {
        return;
    }

    std::string key;
    int value = 0;
    bool legacyScoresLoaded = false;
    while (file >> key >> value)
    {
        if (key == "balloon")
        {
            m_highScores.values[HighScoreIndex(AimMode::Balloon, AimDifficulty::Hard)] = value;
            legacyScoresLoaded = true;
        }
        else if (key == "enemy")
        {
            m_highScores.values[HighScoreIndex(AimMode::Enemy, AimDifficulty::Medium)] = value;
            legacyScoresLoaded = true;
        }
        else if (key == "tracking")
        {
            m_highScores.values[HighScoreIndex(AimMode::Tracking, AimDifficulty::Hard)] = value;
            legacyScoresLoaded = true;
        }

        for (size_t modeIndex = 0; modeIndex < AimModeCount; ++modeIndex)
        {
            for (size_t difficultyIndex = 0; difficultyIndex < AimDifficultyCount; ++difficultyIndex)
            {
                const AimMode mode = static_cast<AimMode>(modeIndex);
                const AimDifficulty difficulty = static_cast<AimDifficulty>(difficultyIndex);
                const std::string scoreKey = AimModeKey(mode) + "_" + AimDifficultyKey(difficulty);
                if (key == scoreKey)
                {
                    m_highScores.values[HighScoreIndex(mode, difficulty)] = value;
                }
            }
        }
    }

    m_highScoreDirty = legacyScoresLoaded;
}

void Dx12App::SaveHighScoresNoThrow()
{
    // 存档失败不应影响主程序退出，因此这里只做尽力保存。
    if (!m_highScoreDirty || m_userDataRoot.empty())
    {
        return;
    }

    try
    {
        std::filesystem::create_directories(m_userDataRoot);
        std::ofstream file{HighScorePath(), std::ios::trunc};
        if (file)
        {
            for (size_t modeIndex = 0; modeIndex < AimModeCount; ++modeIndex)
            {
                for (size_t difficultyIndex = 0; difficultyIndex < AimDifficultyCount; ++difficultyIndex)
                {
                    const AimMode mode = static_cast<AimMode>(modeIndex);
                    const AimDifficulty difficulty = static_cast<AimDifficulty>(difficultyIndex);
                    file << AimModeKey(mode) << "_" << AimDifficultyKey(difficulty) << " "
                        << m_highScores.values[HighScoreIndex(mode, difficulty)] << "\n";
                }
            }
            m_highScoreDirty = false;
        }
    }
    catch (...)
    {
    }
}
