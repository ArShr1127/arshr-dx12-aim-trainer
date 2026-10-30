#include "../../App/Dx12App.h"

float Dx12App::RenderAspect() const
{
    return static_cast<float>(std::max<UINT>(1, m_renderWidth)) /
        static_cast<float>(std::max<UINT>(1, m_renderHeight));
}

float Dx12App::CurrentAimWallHalfWidth() const
{
    return (AimWallHeight * 0.5f) * RenderAspect();
}

float Dx12App::GameWallScaleX() const
{
    return CurrentAimWallHalfWidth() / AimWallHalfWidth;
}

float Dx12App::GameCameraZ() const
{
    // 根据 FOV 和墙体高度反推相机距离，保证游戏墙面稳定铺满屏幕。
    const float fovDegrees = std::clamp(m_pipelineSettings.fovDegrees, 25.0f, 100.0f);
    const float tanHalfFov = std::tan(fovDegrees * Pi / 360.0f);
    float distanceScale = GameCameraDistanceScale;
    if (m_gameMode == AimMode::Balloon)
    {
        distanceScale = BalloonGameCameraDistanceScale;
    }
    else if (m_gameMode == AimMode::Noon)
    {
        distanceScale = NoonGameCameraDistanceScale;
    }
    const float cameraDistance = ((AimWallHeight * 0.5f) / tanHalfFov) * distanceScale;
    return AimWallZ - cameraDistance;
}

float Dx12App::TargetPlaneScale() const
{
    const float cameraZ = GameCameraZ();
    return std::max(0.1f, (TargetZ - cameraZ) / (AimWallZ - cameraZ));
}

float Dx12App::CurrentTargetPlaneHalfWidth() const
{
    return CurrentAimWallHalfWidth() * TargetPlaneScale();
}

float Dx12App::CurrentTargetPlaneHalfHeight() const
{
    return (AimWallHeight * 0.5f) * TargetPlaneScale();
}

XMFLOAT3 Dx12App::RandomWallPosition(float targetRadius)
{
    // 按当前视野缩放后的目标平面随机位置，边缘留出目标半径避免半个球出界。
    const float halfWidth = CurrentTargetPlaneHalfWidth();
    const float halfHeight = CurrentTargetPlaneHalfHeight();
    const float margin = std::max(TargetSpawnMargin, targetRadius);
    return XMFLOAT3{
        RandomRange(m_rng, -halfWidth + margin, halfWidth - margin),
        RandomRange(m_rng, AimWallCenterY - halfHeight + margin, AimWallCenterY + halfHeight - margin),
        TargetZ};
}

XMFLOAT3 Dx12App::RandomWallVelocity(float speed)
{
    const float angle = RandomRange(m_rng, 0.0f, Pi * 2.0f);
    return XMFLOAT3{std::cos(angle) * speed, std::sin(angle) * speed, 0.0f};
}

void Dx12App::MoveTargetOnWall(AimTarget& target, float deltaSeconds)
{
    // 目标碰到训练区域边界就反弹，保证始终留在可击中的墙面范围内。
    target.position.x += target.velocity.x * deltaSeconds;
    target.position.y += target.velocity.y * deltaSeconds;

    const float halfWidth = CurrentTargetPlaneHalfWidth();
    const float halfHeight = CurrentTargetPlaneHalfHeight();
    const float minX = -halfWidth + target.radius;
    const float maxX = halfWidth - target.radius;
    const float minY = AimWallCenterY - halfHeight + target.radius;
    const float maxY = AimWallCenterY + halfHeight - target.radius;

    if (target.position.x < minX || target.position.x > maxX)
    {
        target.position.x = std::clamp(target.position.x, minX, maxX);
        target.velocity.x = -target.velocity.x;
    }

    if (target.position.y < minY || target.position.y > maxY)
    {
        target.position.y = std::clamp(target.position.y, minY, maxY);
        target.velocity.y = -target.velocity.y;
    }
}

void Dx12App::MoveNoonTargetOnWall(AimTarget& target, float deltaSeconds)
{
    target.position.x += target.velocity.x * deltaSeconds;
    target.position.y += target.velocity.y * deltaSeconds;

    const float halfWidth = CurrentTargetPlaneHalfWidth();
    const float halfHeight = CurrentTargetPlaneHalfHeight();
    const float minX = -halfWidth + target.bodyRadius + TargetSpawnMargin;
    const float maxX = halfWidth - target.bodyRadius - TargetSpawnMargin;
    const float minY = AimWallCenterY - halfHeight + target.radius + target.bodyHalfHeight * 2.0f + TargetSpawnMargin;
    const float maxY = AimWallCenterY + halfHeight - target.radius - TargetSpawnMargin;

    if (target.position.x < minX || target.position.x > maxX)
    {
        target.position.x = std::clamp(target.position.x, minX, maxX);
        target.velocity.x = -target.velocity.x;
    }

    if (target.position.y < minY || target.position.y > maxY)
    {
        target.position.y = std::clamp(target.position.y, minY, maxY);
        target.velocity.y = -target.velocity.y;
    }
}

Ray Dx12App::BuildMouseRay(int mouseX, int mouseY) const
{
    // 从屏幕坐标构造世界空间射线，射击命中和准星投影都依赖它。
    const float cosPitch = std::cos(m_cameraPitch);
    XMVECTOR forward = XMVector3Normalize(XMVectorSet(
        std::sin(m_cameraYaw) * cosPitch,
        std::sin(m_cameraPitch),
        std::cos(m_cameraYaw) * cosPitch,
        0.0f));
    XMVECTOR right = XMVector3Normalize(XMVector3Cross(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), forward));
    XMVECTOR up = XMVector3Normalize(XMVector3Cross(forward, right));

    const float ndcX = (2.0f * static_cast<float>(mouseX) / static_cast<float>(m_renderWidth)) - 1.0f;
    const float ndcY = 1.0f - (2.0f * static_cast<float>(mouseY) / static_cast<float>(m_renderHeight));
    const float tanHalfFov = std::tan(m_pipelineSettings.fovDegrees * Pi / 360.0f);
    const float aspect = static_cast<float>(m_renderWidth) / static_cast<float>(m_renderHeight);

    XMVECTOR direction = XMVector3Normalize(
        forward +
        right * (ndcX * aspect * tanHalfFov) +
        up * (ndcY * tanHalfFov));

    Ray ray{};
    ray.origin = m_cameraPosition;
    XMStoreFloat3(&ray.direction, direction);
    return ray;
}

bool Dx12App::RaySphereHit(
    const Ray& ray,
    const XMFLOAT3& center,
    float radius,
    float& closestDistance,
    float& rayT) const
{
    // 最近点法判断射线是否穿过球体，同时返回命中离球心距离和射线深度。
    const XMVECTOR origin = XMLoadFloat3(&ray.origin);
    const XMVECTOR direction = XMVector3Normalize(XMLoadFloat3(&ray.direction));
    const XMVECTOR targetCenter = XMLoadFloat3(&center);
    const XMVECTOR toCenter = targetCenter - origin;
    const float t = XMVectorGetX(XMVector3Dot(toCenter, direction));
    if (t < 0.0f)
    {
        return false;
    }

    const XMVECTOR closest = origin + direction * t;
    const float distance = XMVectorGetX(XMVector3Length(targetCenter - closest));
    if (distance > radius)
    {
        return false;
    }

    closestDistance = distance;
    rayT = t;
    return true;
}

bool Dx12App::RayNoonBodyHit(
    const Ray& ray,
    const AimTarget& target,
    float& closestDistance,
    float& rayT) const
{
    const XMFLOAT3 center = NoonBodyCenter(target);
    const XMVECTOR origin = XMLoadFloat3(&ray.origin);
    const XMVECTOR direction = XMVector3Normalize(XMLoadFloat3(&ray.direction));
    XMFLOAT3 o{};
    XMFLOAT3 d{};
    XMStoreFloat3(&o, origin);
    XMStoreFloat3(&d, direction);

    const float ox = o.x - center.x;
    const float oz = o.z - center.z;
    const float a = d.x * d.x + d.z * d.z;
    const float b = 2.0f * (ox * d.x + oz * d.z);
    const float c = ox * ox + oz * oz - target.bodyRadius * target.bodyRadius;
    if (a <= 0.000001f)
    {
        return false;
    }

    const float discriminant = b * b - 4.0f * a * c;
    if (discriminant < 0.0f)
    {
        return false;
    }

    const float sqrtDiscriminant = std::sqrt(discriminant);
    const float invDenominator = 1.0f / (2.0f * a);
    const float candidates[2] = {
        (-b - sqrtDiscriminant) * invDenominator,
        (-b + sqrtDiscriminant) * invDenominator};

    for (const float candidateT : candidates)
    {
        if (candidateT < 0.0f)
        {
            continue;
        }

        const float hitY = o.y + d.y * candidateT;
        if (hitY < center.y - target.bodyHalfHeight || hitY > center.y + target.bodyHalfHeight)
        {
            continue;
        }

        const float hitX = o.x + d.x * candidateT;
        const float hitZ = o.z + d.z * candidateT;
        const float dx = hitX - center.x;
        const float dz = hitZ - center.z;
        closestDistance = std::sqrt(dx * dx + dz * dz);
        rayT = candidateT;
        return true;
    }

    return false;
}

void Dx12App::HandleGameClick(int mouseX, int mouseY)
{
    // 单发模式一次点击只命中射线方向上最近的有效目标。
    if (!m_gameActive)
    {
        return;
    }

    if (m_gameMode == AimMode::Noon)
    {
        if (m_noonPhase != NoonPhase::Live || m_noonBulletsRemaining <= 0)
        {
            return;
        }

        const Ray ray = BuildMouseRay(mouseX, mouseY);
        int bestIndex = -1;
        bool bestIsHead = false;
        float bestT = std::numeric_limits<float>::max();

        for (int index = 0; index < static_cast<int>(m_gameTargets.size()); ++index)
        {
            AimTarget& target = m_gameTargets[static_cast<size_t>(index)];
            if (!target.active || target.eliminated)
            {
                continue;
            }

            float headDistance = 0.0f;
            float headT = 0.0f;
            if (RaySphereHit(ray, target.position, target.radius, headDistance, headT) && headT < bestT)
            {
                bestT = headT;
                bestIndex = index;
                bestIsHead = true;
            }

            float bodyDistance = 0.0f;
            float bodyT = 0.0f;
            if (RayNoonBodyHit(ray, target, bodyDistance, bodyT) && bodyT < bestT)
            {
                bestT = bodyT;
                bestIndex = index;
                bestIsHead = false;
            }
        }

        --m_noonBulletsRemaining;
        ++m_shotsFired;

        if (bestIndex < 0)
        {
            m_noonPerfectHeadshots = false;
            m_noonPromptText = L"未命中";
            m_noonPromptColor = RGB(80, 160, 255);
            m_noonFeedbackTimer = NoonFeedbackSeconds;
        }
        else
        {
            ++m_shotsHit;
            ++m_noonTotalKills;
            AimTarget& target = m_gameTargets[static_cast<size_t>(bestIndex)];
            target.eliminated = true;
            target.feedbackTimer = NoonFeedbackSeconds;

            if (bestIsHead)
            {
                m_noonRoundBaseScore += NoonHeadScore;
                target.tint = XMFLOAT4{1.0f, 0.08f, 0.08f, 1.0f};
                target.bodyTint = XMFLOAT4{1.0f, 0.08f, 0.08f, 1.0f};
                m_noonPromptText = L"爆头";
                m_noonPromptColor = RGB(255, 72, 72);
            }
            else
            {
                m_noonPerfectHeadshots = false;
                m_noonRoundBaseScore += NoonBodyScore;
                target.tint = XMFLOAT4{1.0f, 0.84f, 0.06f, 1.0f};
                target.bodyTint = XMFLOAT4{1.0f, 0.84f, 0.06f, 1.0f};
                m_noonPromptText = L"躯干命中";
                m_noonPromptColor = RGB(255, 220, 72);
            }
            m_noonFeedbackTimer = NoonFeedbackSeconds;
        }

        if (m_noonBulletsRemaining <= 0 || CountActiveTargets() == 0)
        {
            m_noonRoundEnding = true;
            UpdateAimTrainerLabels();
        }
        else
        {
            UpdateAimTrainerLabels();
        }
        return;
    }

    const Ray ray = BuildMouseRay(mouseX, mouseY);
    int bestIndex = -1;
    float bestT = std::numeric_limits<float>::max();
    float bestDistance = 0.0f;

    for (int index = 0; index < static_cast<int>(m_gameTargets.size()); ++index)
    {
        AimTarget& target = m_gameTargets[static_cast<size_t>(index)];
        if (!target.active)
        {
            continue;
        }

        float distance = 0.0f;
        float rayT = 0.0f;
        if (RaySphereHit(ray, target.position, target.radius, distance, rayT) && rayT < bestT)
        {
            bestT = rayT;
            bestDistance = distance;
            bestIndex = index;
        }
    }

    ++m_shotsFired;
    if (bestIndex < 0)
    {
        if (m_gameMode == AimMode::Enemy)
        {
            AddScore(-EnemyMissPenalty);
        }
        UpdateAimTrainerLabels();
        return;
    }

    ++m_shotsHit;
    AimTarget& target = m_gameTargets[static_cast<size_t>(bestIndex)];
    if (m_gameMode == AimMode::Balloon)
    {
        // 收缩阶段统一按最大半径分数，避免反向鼓励等目标变小。
        const bool growing = target.age <= target.growDuration;
        const float growT = std::clamp(target.age / target.growDuration, 0.0f, 1.0f);
        const int score = growing
            ? static_cast<int>(std::lround(
                static_cast<float>(BalloonSpawnScore) +
                static_cast<float>(BalloonMaxRadiusScore - BalloonSpawnScore) * growT))
            : BalloonMaxRadiusScore;
        AddScore(score);
        target.active = false;
    }
    else if (m_gameMode == AimMode::Enemy)
    {
        // 中心三分之一视为暴击秒杀；外圈扣血，只有击杀才给基础分。
        const int healthBeforeHit = target.health;
        const bool trackingBonus = HasEnemyChaseSpeedBonus(target);
        if (bestDistance <= target.radius / 3.0f)
        {
            const int bonus = trackingBonus
                ? std::max(1, healthBeforeHit) * EnemyTrackingKillBonus
                : 0;
            AddScore(EnemyKillScore + bonus);
            target.active = false;
        }
        else
        {
            --target.health;

            if (target.health <= 0)
            {
                AddScore(EnemyKillScore + (trackingBonus ? EnemyTrackingKillBonus : 0));
                target.active = false;
            }
            else if (target.health == 2)
            {
                target.tint = XMFLOAT4{1.0f, 0.52f, 0.12f, 1.0f};
            }
            else
            {
                target.tint = XMFLOAT4{1.0f, 0.88f, 0.18f, 1.0f};
            }
        }
    }
}
