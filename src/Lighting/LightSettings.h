#pragma once

#include <DirectXMath.h>

using namespace DirectX;

struct LightSettings
{
    // 光照参数由左侧编辑器实时修改，并写入每个物体的常量缓冲区。
    float yawDegrees = 218.0f;
    float pitchDegrees = 58.0f;
    float intensity = 1.0f;
    float ambient = 0.24f;
    XMFLOAT3 color{1.0f, 1.0f, 1.0f};
    bool fluctuation = true;
};

XMFLOAT3 ComputeLightDirection(const LightSettings& settings);
