#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include <DirectXMath.h>
#include <d3d12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;
using namespace DirectX;

enum class AimMode : int
{
    Balloon = 0,
    Enemy = 1,
    Tracking = 2,
    Noon = 3,
    Count = 4
};

enum class AimDifficulty : int
{
    Easy = 0,
    Medium = 1,
    Hard = 2,
    Count = 3
};

inline constexpr size_t AimModeCount = static_cast<size_t>(AimMode::Count);
inline constexpr size_t AimDifficultyCount = static_cast<size_t>(AimDifficulty::Count);
inline constexpr size_t AimHighScoreSlotCount = AimModeCount * AimDifficultyCount;

struct Ray
{
    XMFLOAT3 origin;
    XMFLOAT3 direction;
};

struct PipelineSettings
{
    // 可热重载的渲染开关，主要来自 assets/pipeline_state.txt 和编辑器按钮。
    std::array<float, 4> clearColor{0.02f, 0.03f, 0.06f, 1.0f};
    bool wireframe = false;
    bool backfaceCulling = false;
    bool autoRotate = true;
    float fovDegrees = 60.0f;
};

struct Vertex
{
    XMFLOAT3 position;
    XMFLOAT4 color;
    XMFLOAT3 normal;
};

struct SceneConstants
{
    // 每个可绘制物体一份常量；必须和内嵌 HLSL cbuffer 布局保持一致。
    XMFLOAT4X4 worldViewProjection;
    XMFLOAT4X4 world;
    XMFLOAT4 cameraPosition;
    XMFLOAT4 modelTint;
    XMFLOAT4 options;
    XMFLOAT4 lightDirectionIntensity;
    XMFLOAT4 lightColorAmbient;
};

struct MeshData
{
    std::vector<Vertex> vertices;
    std::vector<std::uint16_t> indices;
};

struct ModelInstance
{
    // 模型数据、编辑状态和 GPU Buffer 放在一起，方便编辑器直接驱动渲染。
    std::wstring name;
    MeshData mesh;

    XMFLOAT3 position{};
    XMFLOAT3 rotation{};
    XMFLOAT3 scale{1.0f, 1.0f, 1.0f};
    XMFLOAT4 tint{1.0f, 1.0f, 1.0f, 1.0f};
    bool visible = true;
    bool rotationLocked = false;
    bool visibleDuringGame = false;

    XMFLOAT3 defaultPosition{};
    XMFLOAT3 defaultRotation{};
    XMFLOAT3 defaultScale{1.0f, 1.0f, 1.0f};
    XMFLOAT4 defaultTint{1.0f, 1.0f, 1.0f, 1.0f};
    bool defaultVisible = true;

    ComPtr<ID3D12Resource> vertexBuffer;
    ComPtr<ID3D12Resource> indexBuffer;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
    D3D12_INDEX_BUFFER_VIEW indexBufferView{};
    UINT indexCount = 0;
};

struct AimTarget
{
    // 训练目标的运行时状态：半径/血量/速度会随模式实时变化。
    bool active = true;
    AimMode mode = AimMode::Balloon;
    XMFLOAT3 position{};
    XMFLOAT3 velocity{};
    XMFLOAT4 tint{1.0f, 0.3f, 0.3f, 1.0f};
    XMFLOAT4 bodyTint{0.88f, 0.88f, 0.92f, 1.0f};
    float radius = 0.25f;
    float bodyRadius = 0.0f;
    float bodyHalfHeight = 0.0f;
    float age = 0.0f;
    float growDuration = 0.75f;
    float shrinkDuration = 0.75f;
    float turnTimer = 0.0f;
    float nextTurnTime = 1.0f;
    float feedbackTimer = 0.0f;
    int health = 1;
    bool eliminated = false;
};

enum class NoonPhase : int
{
    Inactive,
    Preparing,
    Waiting,
    Live
};

struct HighScores
{
    std::array<int, AimHighScoreSlotCount> values{};
};

struct CrosshairSettings
{
    // 准心只影响显示和视角灵敏度，射线命中仍然是屏幕中心的无限小点。
    float gameSensitivity = 0.00020f;
    float size = 0.004f;
    XMFLOAT4 color{0.05f, 1.0f, 0.35f, 1.0f};
};

struct GameSavedState
{
    // 进入 60 秒游戏前保存编辑状态，结束后完整恢复。
    XMFLOAT3 cameraPosition{};
    XMFLOAT3 aimWallScale{1.0f, 1.0f, 1.0f};
    float cameraYaw = 0.0f;
    float cameraPitch = 0.0f;
    bool editorVisible = true;
    bool valid = false;
};
