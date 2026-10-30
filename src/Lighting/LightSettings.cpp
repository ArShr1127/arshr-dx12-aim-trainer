#include "LightSettings.h"

#include <cmath>

namespace
{
    constexpr float Pi = 3.14159265358979323846f;

    float DegreesToRadians(float degrees)
    {
        return degrees * Pi / 180.0f;
    }
}

XMFLOAT3 ComputeLightDirection(const LightSettings& settings)
{
    // yaw 控制水平旋转，pitch 控制上下倾角，最终交给 HLSL 做漫反射/高光。
    const float yaw = DegreesToRadians(settings.yawDegrees);
    const float pitch = DegreesToRadians(settings.pitchDegrees);
    const float cosPitch = std::cos(pitch);

    return XMFLOAT3{
        std::sin(yaw) * cosPitch,
        std::sin(pitch),
        std::cos(yaw) * cosPitch};
}
