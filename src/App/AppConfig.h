#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

#include <DirectXMath.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <limits>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include "../DxHelpers.h"
#include "../Geometry.h"
#include "../Lighting/LightSettings.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;

    inline constexpr wchar_t AppDisplayName[] = L"ArShr的渲染管线与练枪软件";

    // 工程级常量：窗口、训练墙、游戏目标和准心都从这里统一调参。
    constexpr UINT FrameCount = 2;
    constexpr UINT WindowWidth = 1280;
    constexpr UINT WindowHeight = 720;
    constexpr UINT EditorPanelWidth = 340;
    constexpr UINT GamePanelWidth = 320;
    constexpr UINT MaxGameTargets = 64;
    constexpr float AimWallZ = 4.15f;
    constexpr float TargetZ = 3.98f;
    constexpr float AimWallHalfWidth = 4.0f;
    constexpr float AimWallBottom = 0.35f;
    constexpr float AimWallTop = 4.85f;
    constexpr float AimWallCenterY = (AimWallBottom + AimWallTop) * 0.5f;
    constexpr float AimWallHeight = AimWallTop - AimWallBottom;
    constexpr float GameCameraDistanceScale = 1.18f;
    constexpr float BalloonGameCameraDistanceScale = 1.58f;
    constexpr float NoonGameCameraDistanceScale = 1.42f;
    constexpr float TargetSpawnMargin = 0.18f;
    constexpr float Pi = 3.14159265358979323846f;
    constexpr int BalloonSpawnScore = 220;
    constexpr int BalloonMaxRadiusScore = BalloonSpawnScore / 2;
    constexpr int EnemyKillScore = 180;
    constexpr int EnemyMissPenalty = EnemyKillScore / 4;
    constexpr int EnemyTrackingKillBonus = 40;
    constexpr float EnemyAimMoveBonusWindow = 0.25f;
    constexpr int NoonRoundCount = 5;
    constexpr int NoonBulletsPerRound = 6;
    constexpr int NoonBodyScore = 200;
    constexpr int NoonHeadScore = 400;
    constexpr int NoonAllHeadshotBonus = 500;
    constexpr float NoonPrepareSeconds = 2.0f;
    constexpr float NoonFirstWaitMin = 2.0f;
    constexpr float NoonFirstWaitMax = 8.0f;
    constexpr float NoonNextWaitMin = 0.0f;
    constexpr float NoonNextWaitMax = 6.0f;
    constexpr float NoonHeadRadius = 0.062f;
    constexpr float NoonBodyRadius = 0.105f;
    constexpr float NoonBodyHalfHeight = 0.235f;
    constexpr float NoonFeedbackSeconds = 0.58f;
    constexpr float NoonEasyBaseTime = 0.75f;
    constexpr float NoonEasyZeroTime = 1.50f;
    constexpr float NoonMediumBaseTime = 1.50f;
    constexpr float NoonMediumZeroTime = 3.00f;
    constexpr float NoonHardBaseTime = 2.50f;
    constexpr float NoonHardZeroTime = 4.00f;
    constexpr int CrosshairSizeTrackMin = 1;
    constexpr int CrosshairSizeTrackMax = 200;
    constexpr float CrosshairSizeMinimum = 0.001f;
    constexpr float CrosshairSizeTrackStep = 1.0f / 4500.0f;

    struct AimModeConfig
    {
        // 训练模式共用配置结构；不用到的字段保持 0。
        size_t targetCount;
        float spawnInterval;
        float balloonMinRadius;
        float balloonMaxRadius;
        float balloonGrowDuration;
        float balloonShrinkDuration;
        float movingRadius;
        float movingSpeed;
        float turnTimeMin;
        float turnTimeMax;
    };

    // 难度表的行是模式，列是简单/中等/困难。
    constexpr std::array<std::array<AimModeConfig, AimDifficultyCount>, AimModeCount> AimConfigs{{
        {{
            {1, 0.58f, 0.035f, 0.22f, 1.85f, 1.85f, 0.0f, 0.0f, 0.0f, 0.0f},
            {1, 0.46f, 0.026f, 0.17f, 1.58f, 1.58f, 0.0f, 0.0f, 0.0f, 0.0f},
            {1, 0.34f, 0.018f, 0.13f, 1.35f, 1.35f, 0.0f, 0.0f, 0.0f, 0.0f},
        }},
        {{
            {3, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.13f, 1.10f, 0.85f, 1.70f},
            {5, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.09f, 1.55f, 0.45f, 1.20f},
            {6, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.07f, 1.95f, 0.30f, 0.85f},
        }},
        {{
            {1, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.16f, 0.90f, 1.05f, 2.10f},
            {1, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.125f, 1.12f, 0.78f, 1.60f},
            {1, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.082f, 1.45f, 0.36f, 0.82f},
        }},
        {{
            {1, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, NoonHeadRadius, 0.0f, 0.0f, 0.0f},
            {4, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, NoonHeadRadius, 0.0f, 0.0f, 0.0f},
            {6, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, NoonHeadRadius, 0.32f, 1.25f, 2.45f},
        }},
    }};

    constexpr size_t ModeIndex(AimMode mode)
    {
        return static_cast<size_t>(mode);
    }

    constexpr size_t DifficultyIndex(AimDifficulty difficulty)
    {
        return static_cast<size_t>(difficulty);
    }

    constexpr size_t HighScoreIndex(AimMode mode, AimDifficulty difficulty)
    {
        return ModeIndex(mode) * AimDifficultyCount + DifficultyIndex(difficulty);
    }

    enum ControlId : int
    {
        // Win32 控件 ID 需要全局唯一，消息分发时依靠这些 ID 找到操作来源。
        IdModelList = 1000,
        IdVisible = 1001,
        IdResetModel = 1002,
        IdToggleFullscreen = 1003,
        IdToggleUi = 1004,
        IdToggleWireframe = 1005,
        IdReloadPipeline = 1006,
        IdBackgroundGray = 1007,
        IdModeBalloon = 1009,
        IdModeEnemy = 1010,
        IdModeTracking = 1011,
        IdModeNoon = 1012,
        IdStartGame = 1013,
        IdStopGame = 1014,
        IdResetHighScores = 1015,
        IdDifficultyEasy = 1016,
        IdDifficultyMedium = 1017,
        IdDifficultyHard = 1018,
        IdSectionModel = 1019,
        IdSectionAim = 1020,
        IdSectionLight = 1021,
        IdEditorScroll = 1022,
        IdToggleLightFluctuation = 1023,
        IdTrackBase = 1100,
        IdCrosshairTrackBase = 1200,
        IdLightTrackBase = 1300,
    };

    enum class EditorSection : int
    {
        None = -1,
        Model = 0,
        Aim = 1,
        Light = 2,
        Count = 3
    };

    struct EditorControlPlacement
    {
        // 折叠式编辑器用原始位置配合滚动偏移来显示当前模块。
        HWND control = nullptr;
        RECT rect{};
        EditorSection section = EditorSection::None;
    };

    enum TrackIndex : int
    {
        TrackPosX = 0,
        TrackPosY,
        TrackPosZ,
        TrackRotX,
        TrackRotY,
        TrackRotZ,
        TrackScale,
        TrackColorR,
        TrackColorG,
        TrackColorB,
        TrackCount
    };

    enum CrosshairTrackIndex : int
    {
        CrosshairSensitivity = 0,
        CrosshairSize,
        CrosshairColorR,
        CrosshairColorG,
        CrosshairColorB,
        CrosshairTrackCount
    };

    enum LightTrackIndex : int
    {
        LightYaw = 0,
        LightPitch,
        LightIntensity,
        LightAmbient,
        LightColorR,
        LightColorG,
        LightColorB,
        LightTrackCount
    };

    struct TrackConfig
    {
        // 滑条只保存整数范围，读写模型时再映射成真实浮点值。
        const wchar_t* label;
        int minValue;
        int maxValue;
    };

    constexpr std::array<TrackConfig, TrackCount> TrackConfigs{{
        {L"位置 X", -80, 80},
        {L"位置 Y", -20, 80},
        {L"位置 Z", -80, 80},
        {L"旋转 X", 0, 628},
        {L"旋转 Y", 0, 628},
        {L"旋转 Z", 0, 628},
        {L"缩放", 2, 40},
        {L"颜色 R", 0, 100},
        {L"颜色 G", 0, 100},
        {L"颜色 B", 0, 100},
    }};

    inline float WrapAngle(float radians)
    {
        // 保证 UI 上显示的旋转角始终落在 0 到 2π。
        float result = std::fmod(radians, Pi * 2.0f);
        if (result < 0.0f)
        {
            result += Pi * 2.0f;
        }
        return result;
    }

    inline float RandomRange(std::mt19937& rng, float minValue, float maxValue)
    {
        std::uniform_real_distribution<float> distribution{minValue, maxValue};
        return distribution(rng);
    }

    inline int RandomRange(std::mt19937& rng, int minValue, int maxValue)
    {
        std::uniform_int_distribution<int> distribution{minValue, maxValue};
        return distribution(rng);
    }

    inline float CrosshairSizeFromTrack(int value)
    {
        // 最小值保持很小，后续增长放缓，方便细调实心准心大小。
        const int clampedValue = std::clamp(value, CrosshairSizeTrackMin, CrosshairSizeTrackMax);
        return CrosshairSizeMinimum +
            static_cast<float>(clampedValue - CrosshairSizeTrackMin) * CrosshairSizeTrackStep;
    }

    inline int CrosshairTrackFromSize(float size)
    {
        const float trackValue = static_cast<float>(CrosshairSizeTrackMin) +
            ((size - CrosshairSizeMinimum) / CrosshairSizeTrackStep);
        return std::clamp(
            static_cast<int>(std::lround(trackValue)),
            CrosshairSizeTrackMin,
            CrosshairSizeTrackMax);
    }
