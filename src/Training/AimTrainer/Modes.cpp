#include "../../App/Dx12App.h"

void Dx12App::Update(float deltaSeconds)
{
    // 非游戏状态允许自由相机和模型编辑；游戏状态只更新固定视角训练逻辑。
    const float moveSpeed = 4.5f * deltaSeconds;
    const float turnSpeed = 1.8f * deltaSeconds;

    const float cosPitch = std::cos(m_cameraPitch);
    XMVECTOR forward = XMVector3Normalize(XMVectorSet(
        std::sin(m_cameraYaw) * cosPitch,
        std::sin(m_cameraPitch),
        std::cos(m_cameraYaw) * cosPitch,
        0.0f));
    XMVECTOR right = XMVector3Normalize(XMVector3Cross(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), forward));
    const XMVECTOR worldUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMVECTOR eye = XMLoadFloat3(&m_cameraPosition);
    if (!m_gameActive)
    {
        if (m_keys['W'])
        {
            eye += forward * moveSpeed;
        }
        if (m_keys['S'])
        {
            eye -= forward * moveSpeed;
        }
        if (m_keys['D'])
        {
            eye += right * moveSpeed;
        }
        if (m_keys['A'])
        {
            eye -= right * moveSpeed;
        }
        if (m_keys['E'])
        {
            eye += worldUp * moveSpeed;
        }
        if (m_keys[VK_SPACE])
        {
            eye += worldUp * moveSpeed;
        }
        if (m_keys['Q'])
        {
            eye -= worldUp * moveSpeed;
        }
        if (m_keys[VK_CONTROL] || m_keys[VK_LCONTROL] || m_keys[VK_RCONTROL])
        {
            eye -= worldUp * moveSpeed;
        }
    }
    XMStoreFloat3(&m_cameraPosition, eye);

    if (!m_gameActive && !m_models.empty())
    {
        ModelInstance& selected = m_models[m_selectedModel];
        if (!selected.rotationLocked)
        {
            if (m_keys[VK_LEFT])
            {
                selected.rotation.y -= turnSpeed;
            }
            if (m_keys[VK_RIGHT])
            {
                selected.rotation.y += turnSpeed;
            }
            if (m_keys[VK_UP])
            {
                selected.rotation.x -= turnSpeed;
            }
            if (m_keys[VK_DOWN])
            {
                selected.rotation.x += turnSpeed;
            }
        }
    }

    UpdateAimGame(deltaSeconds);
    UpdateConstantBuffers(forward, right);
}

void Dx12App::UpdateAimGame(float deltaSeconds)
{
    // 三个训练模式共用 60 秒倒计时，差异主要在目标生成和计分方式。
    if (!m_gameActive)
    {
        return;
    }

    if (m_gameMode != AimMode::Noon)
    {
        m_gameRemainingSeconds -= deltaSeconds;
    }
    if (m_gameMode != AimMode::Noon && m_gameRemainingSeconds <= 0.0f)
    {
        FinishAimGame();
        return;
    }

    if (m_gameMode == AimMode::Balloon)
    {
        // 快枪手：目标按固定间隔出现，半径先放大再缩小，越早击中分越高。
        const AimModeConfig& config = CurrentAimConfig();
        m_balloonSpawnTimer += deltaSeconds;
        while (m_balloonSpawnTimer >= config.spawnInterval)
        {
            m_balloonSpawnTimer -= config.spawnInterval;
            SpawnBalloonTarget();
        }

        for (AimTarget& target : m_gameTargets)
        {
            target.age += deltaSeconds;
            const float totalDuration = target.growDuration + target.shrinkDuration;
            if (target.age > totalDuration)
            {
                target.active = false;
                continue;
            }

            if (target.age <= target.growDuration)
            {
                const float t = target.age / target.growDuration;
                target.radius = config.balloonMinRadius + (config.balloonMaxRadius - config.balloonMinRadius) * t;
            }
            else
            {
                const float t = (target.age - target.growDuration) / target.shrinkDuration;
                const float shrinkTarget = std::max(0.006f, config.balloonMinRadius * 0.2f);
                target.radius = config.balloonMaxRadius + (shrinkTarget - config.balloonMaxRadius) * t;
            }
        }
    }
    else if (m_gameMode == AimMode::Enemy)
    {
        // 游侠之眼：多个三血目标在墙面内移动，随机变向并支持追击加分。
        const AimModeConfig& config = CurrentAimConfig();
        for (AimTarget& target : m_gameTargets)
        {
            if (!target.active)
            {
                continue;
            }

            target.age += deltaSeconds;
            target.turnTimer += deltaSeconds;
            if (target.turnTimer >= target.nextTurnTime)
            {
                target.turnTimer = 0.0f;
                target.nextTurnTime = RandomRange(m_rng, config.turnTimeMin, config.turnTimeMax);
                target.velocity = RandomWallVelocity(config.movingSpeed);
            }

            MoveTargetOnWall(target, deltaSeconds);
        }

        while (CountActiveTargets() < config.targetCount && m_gameTargets.size() < MaxGameTargets)
        {
            SpawnEnemyTarget();
        }
    }
    else if (m_gameMode == AimMode::Tracking)
    {
        // 聚焦火力：按住左键持续检测中心射线，命中时间越长得分越高。
        const AimModeConfig& config = CurrentAimConfig();
        if (m_gameTargets.empty())
        {
            SpawnTrackingTarget();
        }

        for (AimTarget& target : m_gameTargets)
        {
            target.turnTimer += deltaSeconds;
            if (target.turnTimer >= target.nextTurnTime)
            {
                target.turnTimer = 0.0f;
                target.nextTurnTime = RandomRange(m_rng, config.turnTimeMin, config.turnTimeMax);
                target.velocity = RandomWallVelocity(config.movingSpeed);
            }

            MoveTargetOnWall(target, deltaSeconds);
        }

        if (m_leftFiring && !m_gameTargets.empty())
        {
            m_trackingFireTime += deltaSeconds;
            const Ray ray = BuildMouseRay(
                static_cast<int>(m_renderWidth / 2),
                static_cast<int>(m_renderHeight / 2));
            float hitDistance = 0.0f;
            float rayT = 0.0f;
            if (RaySphereHit(ray, m_gameTargets.front().position, m_gameTargets.front().radius, hitDistance, rayT))
            {
                m_trackingHitTime += deltaSeconds;
                m_fractionalScore += 140.0f * deltaSeconds;
                const int wholeScore = static_cast<int>(m_fractionalScore);
                if (wholeScore > 0)
                {
                    m_fractionalScore -= static_cast<float>(wholeScore);
                    AddScore(wholeScore);
                }
            }
        }
    }
    else
    {
        UpdateNoonGame(deltaSeconds);
    }

    m_gameTargets.erase(
        std::remove_if(m_gameTargets.begin(), m_gameTargets.end(), [](const AimTarget& target) {
            return !target.active;
        }),
        m_gameTargets.end());
    if (m_totalTime >= m_nextAimTrainerLabelUpdateTime)
    {
        m_nextAimTrainerLabelUpdateTime = m_totalTime + 0.05f;
        UpdateAimTrainerLabels();
    }
}

void Dx12App::SpawnBalloonTarget()
{
    if (m_gameTargets.size() >= MaxGameTargets)
    {
        return;
    }

    const AimModeConfig& config = CurrentAimConfig();
    AimTarget target{};
    target.mode = AimMode::Balloon;
    target.position = RandomWallPosition(config.balloonMaxRadius);
    target.radius = config.balloonMinRadius;
    target.growDuration = config.balloonGrowDuration;
    target.shrinkDuration = config.balloonShrinkDuration;
    target.tint = XMFLOAT4{1.0f, 0.26f, 0.30f, 1.0f};
    target.health = 1;
    m_gameTargets.push_back(target);
}

void Dx12App::SpawnEnemyTarget()
{
    const AimModeConfig& config = CurrentAimConfig();
    AimTarget target{};
    target.mode = AimMode::Enemy;
    target.position = RandomWallPosition(config.movingRadius);
    target.radius = config.movingRadius;
    target.velocity = RandomWallVelocity(config.movingSpeed);
    target.nextTurnTime = RandomRange(m_rng, config.turnTimeMin, config.turnTimeMax);
    target.health = 3;
    target.tint = XMFLOAT4{1.0f, 0.20f, 0.12f, 1.0f};
    m_gameTargets.push_back(target);
}

void Dx12App::SpawnTrackingTarget()
{
    m_gameTargets.clear();

    const AimModeConfig& config = CurrentAimConfig();
    AimTarget target{};
    target.mode = AimMode::Tracking;
    target.position = RandomWallPosition(config.movingRadius);
    target.radius = config.movingRadius;
    target.velocity = RandomWallVelocity(config.movingSpeed);
    target.nextTurnTime = RandomRange(m_rng, config.turnTimeMin, config.turnTimeMax);
    target.health = 1;
    target.tint = XMFLOAT4{0.20f, 0.72f, 1.0f, 1.0f};
    m_gameTargets.push_back(target);
}

void Dx12App::BeginNoonRound()
{
    ++m_noonRoundIndex;
    m_noonPhase = NoonPhase::Preparing;
    m_noonPhaseTimer = NoonPrepareSeconds;
    m_noonRoundElapsed = 0.0f;
    m_noonRoundBaseScore = 0;
    m_noonBulletsRemaining = NoonBulletsPerRound;
    m_noonRoundEnding = false;
    m_noonFeedbackTimer = 0.0f;
    m_noonPromptText = L"2";
    m_noonPromptColor = RGB(245, 245, 245);
    m_gameTargets.clear();
    UpdateAimTrainerLabels();
}

void Dx12App::UpdateNoonGame(float deltaSeconds)
{
    if (m_noonPhase == NoonPhase::Inactive)
    {
        return;
    }

    if (m_noonPhase == NoonPhase::Preparing)
    {
        m_noonPhaseTimer = std::max(0.0f, m_noonPhaseTimer - deltaSeconds);
        const int countdown = std::max(1, static_cast<int>(std::ceil(m_noonPhaseTimer)));
        m_noonPromptText = std::to_wstring(countdown);
        m_noonPromptColor = RGB(245, 245, 245);

        if (m_noonPhaseTimer <= 0.0f)
        {
            const bool firstRound = m_noonRoundIndex == 1;
            m_noonPhase = NoonPhase::Waiting;
            m_noonPhaseTimer = RandomRange(
                m_rng,
                firstRound ? NoonFirstWaitMin : NoonNextWaitMin,
                firstRound ? NoonFirstWaitMax : NoonNextWaitMax);
            m_noonPromptText = L"准备拔枪";
            m_noonPromptColor = RGB(245, 245, 245);
        }
        return;
    }

    if (m_noonPhase == NoonPhase::Waiting)
    {
        m_noonPhaseTimer = std::max(0.0f, m_noonPhaseTimer - deltaSeconds);
        m_noonPromptText = L"准备拔枪";
        m_noonPromptColor = RGB(245, 245, 245);

        if (m_noonPhaseTimer <= 0.0f)
        {
            SpawnNoonTargets();
            m_noonPhase = NoonPhase::Live;
            m_noonRoundElapsed = 0.0f;
            m_noonPromptText = L"午时已到！";
            m_noonPromptColor = RGB(255, 232, 150);
        }
        return;
    }

    if (m_noonPhase != NoonPhase::Live)
    {
        return;
    }

    m_noonRoundElapsed += deltaSeconds;
    if (m_noonFeedbackTimer > 0.0f)
    {
        m_noonFeedbackTimer = std::max(0.0f, m_noonFeedbackTimer - deltaSeconds);
    }
    bool targetFeedbackVisible = false;
    for (AimTarget& target : m_gameTargets)
    {
        if (!target.active || !target.eliminated)
        {
            continue;
        }

        target.feedbackTimer = std::max(0.0f, target.feedbackTimer - deltaSeconds);
        if (target.feedbackTimer <= 0.0f)
        {
            target.active = false;
        }
        else
        {
            targetFeedbackVisible = true;
        }
    }
    if (m_noonFeedbackTimer <= 0.0f)
    {
        m_noonPromptText = L"午时已到！";
        m_noonPromptColor = RGB(255, 232, 150);
    }

    if (m_gameDifficulty == AimDifficulty::Hard)
    {
        const AimModeConfig& config = CurrentAimConfig();
        for (AimTarget& target : m_gameTargets)
        {
            if (!target.active)
            {
                continue;
            }
            if (target.eliminated)
            {
                continue;
            }

            target.turnTimer += deltaSeconds;
            if (target.turnTimer >= target.nextTurnTime)
            {
                target.turnTimer = 0.0f;
                target.nextTurnTime = RandomRange(m_rng, config.turnTimeMin, config.turnTimeMax);
                target.velocity = RandomWallVelocity(config.movingSpeed);
            }
            MoveNoonTargetOnWall(target, deltaSeconds);
        }
    }

    if (CountActiveTargets() == 0)
    {
        m_noonRoundEnding = true;
    }
    if (m_noonRoundEnding && !targetFeedbackVisible && m_noonFeedbackTimer <= 0.0f)
    {
        FinishNoonRound();
    }
}

void Dx12App::SpawnNoonTargets()
{
    m_gameTargets.clear();

    const AimModeConfig& config = CurrentAimConfig();
    const size_t targetCount = std::min<size_t>(config.targetCount, MaxGameTargets);
    const float halfWidth = CurrentTargetPlaneHalfWidth();
    const float halfHeight = CurrentTargetPlaneHalfHeight();
    const float marginX = NoonBodyRadius + TargetSpawnMargin;
    const float marginTop = NoonHeadRadius + 0.08f;
    const float marginBottom = NoonHeadRadius + NoonBodyHalfHeight * 2.0f + 0.08f;
    const float minX = -halfWidth + marginX;
    const float maxX = halfWidth - marginX;
    const float minY = AimWallCenterY - halfHeight + marginBottom;
    const float maxY = AimWallCenterY + halfHeight - marginTop;
    const float mediumLaneY = RandomRange(
        m_rng,
        std::max(minY, AimWallCenterY - halfHeight * 0.12f),
        std::min(maxY, AimWallCenterY + halfHeight * 0.12f));
    std::vector<XMFLOAT2> usedPositions;

    for (size_t index = 0; index < targetCount; ++index)
    {
        XMFLOAT2 position{};
        if (m_gameDifficulty == AimDifficulty::Easy)
        {
            position.x = RandomRange(m_rng, std::max(minX, -halfWidth * 0.35f), std::min(maxX, halfWidth * 0.35f));
            position.y = RandomRange(m_rng, std::max(minY, AimWallCenterY - halfHeight * 0.18f), std::min(maxY, AimWallCenterY + halfHeight * 0.18f));
        }
        else if (m_gameDifficulty == AimDifficulty::Medium)
        {
            const float segment = (maxX - minX) / static_cast<float>(targetCount);
            const float segmentLeft = minX + segment * static_cast<float>(index);
            const float segmentRight = segmentLeft + segment;
            position.x = RandomRange(m_rng, segmentLeft + segment * 0.18f, segmentRight - segment * 0.18f);
            position.y = mediumLaneY;
        }
        else
        {
            constexpr int MaxAttempts = 32;
            for (int attempt = 0; attempt < MaxAttempts; ++attempt)
            {
                position.x = RandomRange(m_rng, minX, maxX);
                position.y = RandomRange(m_rng, minY, maxY);
                bool separated = true;
                for (const XMFLOAT2& used : usedPositions)
                {
                    const float dx = position.x - used.x;
                    const float dy = position.y - used.y;
                    if (dx * dx + dy * dy < 0.32f)
                    {
                        separated = false;
                        break;
                    }
                }
                if (separated)
                {
                    break;
                }
            }
        }

        usedPositions.push_back(position);

        AimTarget target{};
        target.mode = AimMode::Noon;
        target.position = XMFLOAT3{position.x, position.y, TargetZ};
        target.radius = NoonHeadRadius;
        target.bodyRadius = NoonBodyRadius;
        target.bodyHalfHeight = NoonBodyHalfHeight;
        target.tint = XMFLOAT4{0.12f, 0.42f, 1.0f, 1.0f};
        target.bodyTint = XMFLOAT4{0.08f, 0.32f, 0.92f, 1.0f};
        target.health = 1;
        if (m_gameDifficulty == AimDifficulty::Hard)
        {
            target.velocity = RandomWallVelocity(config.movingSpeed);
            target.nextTurnTime = RandomRange(m_rng, config.turnTimeMin, config.turnTimeMax);
        }
        m_gameTargets.push_back(target);
    }
}

void Dx12App::FinishNoonRound()
{
    if (m_noonPhase == NoonPhase::Live && m_noonRoundBaseScore > 0)
    {
        const float multiplier = NoonTimeMultiplier(m_noonRoundElapsed);
        AddScore(static_cast<int>(std::lround(static_cast<float>(m_noonRoundBaseScore) * multiplier)));
    }

    m_noonRoundBaseScore = 0;
    m_noonBulletsRemaining = 0;
    m_gameTargets.clear();

    if (m_noonRoundIndex >= NoonRoundCount)
    {
        if (m_noonPerfectHeadshots && m_noonTotalKills == m_noonExpectedKills)
        {
            AddScore(NoonAllHeadshotBonus);
        }
        FinishAimGame();
        return;
    }

    BeginNoonRound();
}

void Dx12App::ResetNoonState()
{
    m_noonPhase = NoonPhase::Inactive;
    m_noonRoundIndex = 0;
    m_noonBulletsRemaining = 0;
    m_noonRoundBaseScore = 0;
    m_noonTotalKills = 0;
    m_noonExpectedKills = static_cast<int>(CurrentAimConfig().targetCount) * NoonRoundCount;
    m_noonPerfectHeadshots = true;
    m_noonRoundEnding = false;
    m_noonPhaseTimer = 0.0f;
    m_noonRoundElapsed = 0.0f;
    m_noonFeedbackTimer = 0.0f;
    m_noonPromptText.clear();
    m_noonPromptColor = RGB(245, 245, 245);
}

float Dx12App::NoonTimeMultiplier(float seconds) const
{
    const float baseTime = NoonBaseTime();
    const float zeroTime = NoonZeroTime();
    const float t = std::clamp(seconds, 0.0f, zeroTime);
    const float ratio = std::pow(zeroTime / baseTime, 0.25f);
    const float offset = (zeroTime - ratio * baseTime) / (ratio - 1.0f);
    const float scale = 4.0f / std::log((offset + zeroTime) / offset);
    return std::max(0.0f, scale * std::log((offset + zeroTime) / (offset + t)));
}

float Dx12App::NoonBaseTime() const
{
    switch (m_gameDifficulty)
    {
    case AimDifficulty::Easy:
        return NoonEasyBaseTime;
    case AimDifficulty::Medium:
        return NoonMediumBaseTime;
    case AimDifficulty::Hard:
        return NoonHardBaseTime;
    default:
        return NoonHardBaseTime;
    }
}

float Dx12App::NoonZeroTime() const
{
    switch (m_gameDifficulty)
    {
    case AimDifficulty::Easy:
        return NoonEasyZeroTime;
    case AimDifficulty::Medium:
        return NoonMediumZeroTime;
    case AimDifficulty::Hard:
        return NoonHardZeroTime;
    default:
        return NoonHardZeroTime;
    }
}

XMFLOAT3 Dx12App::NoonBodyCenter(const AimTarget& target) const
{
    return XMFLOAT3{
        target.position.x,
        target.position.y - target.radius - target.bodyHalfHeight * 0.98f,
        target.position.z};
}
