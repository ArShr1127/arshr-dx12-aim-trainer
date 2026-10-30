#include "../../App/Dx12App.h"

bool Dx12App::CurrentAimPointOnTargetPlane(XMFLOAT2& point) const
{
    // 把屏幕中心射线投影到目标平面，用于计算准星在墙面上的移动速度。
    const Ray ray = BuildMouseRay(
        static_cast<int>(m_renderWidth / 2),
        static_cast<int>(m_renderHeight / 2));

    if (std::abs(ray.direction.z) < 0.0001f)
    {
        return false;
    }

    const float t = (TargetZ - ray.origin.z) / ray.direction.z;
    if (t <= 0.0f)
    {
        return false;
    }

    point = XMFLOAT2{
        ray.origin.x + ray.direction.x * t,
        ray.origin.y + ray.direction.y * t};
    return true;
}

void Dx12App::HandleGameMouseLook(int mouseX, int mouseY)
{
    // 保留旧的绝对鼠标路径作为备用；主路径在窗口消息里使用 Raw Input。
    const int centerX = static_cast<int>(m_renderWidth / 2);
    const int centerY = static_cast<int>(m_renderHeight / 2);
    const int dx = mouseX - centerX;
    const int dy = mouseY - centerY;

    if (m_ignoreNextGameMouseMove && dx == 0 && dy == 0)
    {
        m_ignoreNextGameMouseMove = false;
        return;
    }
    m_ignoreNextGameMouseMove = false;

    if (dx != 0 || dy != 0)
    {
        const auto moveClock = std::chrono::steady_clock::now();
        XMFLOAT2 aimBefore{};
        const bool hadAimBefore = CurrentAimPointOnTargetPlane(aimBefore);

        m_cameraYaw += static_cast<float>(dx) * m_crosshair.gameSensitivity;
        m_cameraPitch -= static_cast<float>(dy) * m_crosshair.gameSensitivity;
        m_cameraPitch = std::clamp(m_cameraPitch, -1.45f, 1.45f);

        XMFLOAT2 aimAfter{};
        const bool hadAimAfter = CurrentAimPointOnTargetPlane(aimAfter);
        const int screenMoveSquared = dx * dx + dy * dy;
        if (hadAimBefore && hadAimAfter && screenMoveSquared >= 4)
        {
            const XMFLOAT2 move{
                aimAfter.x - aimBefore.x,
                aimAfter.y - aimBefore.y};
            const float moveLengthSquared = move.x * move.x + move.y * move.y;
            if (moveLengthSquared > 0.00000025f)
            {
                float deltaSeconds = 1.0f / 240.0f;
                if (m_hasAimMoveClock)
                {
                    deltaSeconds = std::chrono::duration<float>(moveClock - m_lastAimMoveClock).count();
                }
                deltaSeconds = std::clamp(deltaSeconds, 1.0f / 1000.0f, EnemyAimMoveBonusWindow);

                m_recentAimVelocityOnPlane = XMFLOAT2{
                    move.x / deltaSeconds,
                    move.y / deltaSeconds};
                m_recentAimMoveTime = m_totalTime;
                m_hasRecentAimMove = true;
            }
        }

        m_lastAimMoveClock = moveClock;
        m_hasAimMoveClock = true;
        CenterMouseForGame();
    }
}

void Dx12App::HandleGameMouseDelta(int dx, int dy)
{
    // Raw Input 给的是相对位移，不需要回正鼠标，因此游戏中不会因鼠标回中卡死。
    if (dx == 0 && dy == 0)
    {
        return;
    }

    const auto moveClock = std::chrono::steady_clock::now();
    XMFLOAT2 aimBefore{};
    const bool hadAimBefore = CurrentAimPointOnTargetPlane(aimBefore);

    m_cameraYaw += static_cast<float>(dx) * m_crosshair.gameSensitivity;
    m_cameraPitch -= static_cast<float>(dy) * m_crosshair.gameSensitivity;
    m_cameraPitch = std::clamp(m_cameraPitch, -1.45f, 1.45f);

    XMFLOAT2 aimAfter{};
    const bool hadAimAfter = CurrentAimPointOnTargetPlane(aimAfter);
    const int screenMoveSquared = dx * dx + dy * dy;
    if (hadAimBefore && hadAimAfter && screenMoveSquared >= 4)
    {
        const XMFLOAT2 move{
            aimAfter.x - aimBefore.x,
            aimAfter.y - aimBefore.y};
        const float moveLengthSquared = move.x * move.x + move.y * move.y;
        if (moveLengthSquared > 0.00000025f)
        {
            float deltaSeconds = 1.0f / 240.0f;
            if (m_hasAimMoveClock)
            {
                deltaSeconds = std::chrono::duration<float>(moveClock - m_lastAimMoveClock).count();
            }
            deltaSeconds = std::clamp(deltaSeconds, 1.0f / 1000.0f, EnemyAimMoveBonusWindow);

            m_recentAimVelocityOnPlane = XMFLOAT2{
                move.x / deltaSeconds,
                move.y / deltaSeconds};
            m_recentAimMoveTime = m_totalTime;
            m_hasRecentAimMove = true;
        }
    }

    m_lastAimMoveClock = moveClock;
    m_hasAimMoveClock = true;
    m_lastAimX = static_cast<int>(m_renderWidth / 2);
    m_lastAimY = static_cast<int>(m_renderHeight / 2);
}

bool Dx12App::HasEnemyChaseSpeedBonus(const AimTarget& target) const
{
    // 把准星速度投影到小球移动方向；只有追击速度超过小球时才给额外分。
    if (!m_hasRecentAimMove || m_totalTime - m_recentAimMoveTime > EnemyAimMoveBonusWindow)
    {
        return false;
    }

    const float targetSpeedSquared = target.velocity.x * target.velocity.x + target.velocity.y * target.velocity.y;
    const float aimVelocitySquared =
        m_recentAimVelocityOnPlane.x * m_recentAimVelocityOnPlane.x +
        m_recentAimVelocityOnPlane.y * m_recentAimVelocityOnPlane.y;
    if (targetSpeedSquared <= 0.0001f || aimVelocitySquared <= 0.0001f)
    {
        return false;
    }

    const float targetSpeed = std::sqrt(targetSpeedSquared);
    const XMFLOAT2 targetDirection{
        target.velocity.x / targetSpeed,
        target.velocity.y / targetSpeed};
    const float projectedAimSpeed =
        m_recentAimVelocityOnPlane.x * targetDirection.x +
        m_recentAimVelocityOnPlane.y * targetDirection.y;

    return projectedAimSpeed > targetSpeed;
}
