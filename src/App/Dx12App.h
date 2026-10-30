#pragma once

#include "AppConfig.h"

class Dx12App
{
public:
    ~Dx12App();
    void Initialize(HINSTANCE instance, int showCommand);
    int Run();
    void UpdateFrameStats(float deltaSeconds);

private:
    // Win32 消息入口和编辑器控件事件。
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    void HandleCommand(int id, int notifyCode, HWND);
    void HandleTrackbar(HWND trackbar);
    void HandleCrosshairTrackbar(int index);
    int BackgroundGrayTrackValue() const;
    void HandleBackgroundGrayTrackbar();
    void HandleLightTrackbar(int index);
    void SyncBackgroundGrayTrackbar();
    void UpdateBackgroundGrayLabel();
    void UpdateLightLabels();
    void HandleKeyPressed(WPARAM key);

    // 管线热重载与窗口/控件创建。
    template <typename Mutator>
    void RebuildPipelineWith(Mutator mutate);
    void ReloadEditablePipeline();
    void CreateAppWindow(HINSTANCE instance, int showCommand);
    HWND CreateChild(
        const wchar_t* className,
        const wchar_t* text,
        DWORD style,
        int x,
        int y,
        int width,
        int height,
        int id);
    HWND CreateHudLabel(const wchar_t* text, int x, int y, int width);
    void CreateGameHudControls();
    void CreateEditorControls();
    void CreateAimTrainerControls(int x, int y, int width);
    void CreateCrosshairTrack(int x, int y, int width, int index, const wchar_t* label, int minValue, int maxValue, int initialValue);
    void CreateLightControls(int x, int y, int width);
    void CreateLightTrack(int x, int y, int width, int index, const wchar_t* label, int minValue, int maxValue, int initialValue);
    void SetEditorVisible(bool visible);
    void SetActiveEditorSection(EditorSection section);
    void ToggleEditorSection(EditorSection section);
    const wchar_t* EditorSectionTitle(EditorSection section) const;
    void UpdateEditorSectionButtons();
    int EditorViewportBottom() const;
    int ActiveEditorContentBottom() const;
    void UpdateEditorScrollbar();
    void SetEditorScrollOffset(int offset);
    void ScrollEditorBy(int delta);
    void HandleEditorScroll(WORD request, WORD thumbPosition);
    bool HandleEditorMouseWheel(int x, int y, int wheelDelta);
    void ApplyEditorSectionVisibility();
    void UpdateGameHudVisibility();
    void SetCrosshairEditorEnabled(bool enabled);
    void UpdateCrosshairLabels();

    // 练枪模块：模式、计分、准心移动和最高分存档。
    void SelectAimMode(AimMode mode);
    void SelectAimDifficulty(AimDifficulty difficulty);
    const AimModeConfig& CurrentAimConfig() const;
    void StartAimGame();
    void StopAimGame();
    void FinishAimGame();
    void RestoreAfterGame();
    void CenterMouseForGame();
    void HandleGameMouseLook(int mouseX, int mouseY);
    void HandleGameMouseDelta(int dx, int dy);
    bool CurrentAimPointOnTargetPlane(XMFLOAT2& point) const;
    bool HasEnemyChaseSpeedBonus(const AimTarget& target) const;
    void ResetHighScores();
    void AddScore(int amount);
    size_t CountActiveTargets() const;
    std::wstring AimModeName(AimMode mode) const;
    std::wstring AimDifficultyName(AimDifficulty difficulty) const;
    std::wstring AimModeHint(AimMode mode) const;
    float CurrentHitRatePercent() const;
    void UpdateAimTrainerLabels();
    std::filesystem::path HighScorePath() const;
    std::string AimModeKey(AimMode mode) const;
    std::string AimDifficultyKey(AimDifficulty difficulty) const;
    void LoadHighScores();
    void SaveHighScoresNoThrow();

    // DX12 初始化、资源上传和基础场景生成。
    void ToggleFullscreen();
    std::filesystem::path FindAssetRoot() const;
    std::filesystem::path FindUserDataRoot() const;
    PipelineSettings LoadPipelineSettings() const;
    void LoadPipeline();
    void LoadAssets();
    void CreateRootSignature();
    ComPtr<ID3D12PipelineState> CreatePipelineState(
        const PipelineSettings& settings,
        D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        bool depthEnabled = true);
    void FinalizeDefaults(ModelInstance& model);
    void CreateSceneModels();
    void CreateModelBuffers();
    void CreateBuffersForModel(
        ModelInstance& model,
        const D3D12_HEAP_PROPERTIES& uploadHeap,
        const D3D12_RANGE& readRange);
    void PopulateModelList();
    void CreateConstantBuffer();

    // 每帧更新：相机/模型、训练目标、命中射线和常量缓冲区。
    void Update(float deltaSeconds);
    void UpdateAimGame(float deltaSeconds);
    void SpawnBalloonTarget();
    void SpawnEnemyTarget();
    void SpawnTrackingTarget();
    void BeginNoonRound();
    void UpdateNoonGame(float deltaSeconds);
    void SpawnNoonTargets();
    void FinishNoonRound();
    void ResetNoonState();
    float NoonTimeMultiplier(float seconds) const;
    float NoonBaseTime() const;
    float NoonZeroTime() const;
    XMFLOAT3 NoonBodyCenter(const AimTarget& target) const;
    float RenderAspect() const;
    float CurrentAimWallHalfWidth() const;
    float GameWallScaleX() const;
    float GameCameraZ() const;
    float TargetPlaneScale() const;
    float CurrentTargetPlaneHalfWidth() const;
    float CurrentTargetPlaneHalfHeight() const;
    XMFLOAT3 RandomWallPosition(float targetRadius);
    XMFLOAT3 RandomWallVelocity(float speed);
    void MoveTargetOnWall(AimTarget& target, float deltaSeconds);
    void MoveNoonTargetOnWall(AimTarget& target, float deltaSeconds);
    Ray BuildMouseRay(int mouseX, int mouseY) const;
    bool RaySphereHit(
        const Ray& ray,
        const XMFLOAT3& center,
        float radius,
        float& closestDistance,
        float& rayT) const;
    bool RayNoonBodyHit(
        const Ray& ray,
        const AimTarget& target,
        float& closestDistance,
        float& rayT) const;
    void HandleGameClick(int mouseX, int mouseY);
    XMMATRIX ModelWorldMatrix(const ModelInstance& model, size_t index) const;
    XMMATRIX TargetWorldMatrix(const AimTarget& target) const;
    XMMATRIX NoonBodyWorldMatrix(const AimTarget& target) const;
    XMMATRIX CrosshairWorldMatrix(FXMVECTOR forward, FXMVECTOR right, FXMVECTOR up) const;
    XMMATRIX LightRangeWorldMatrix() const;
    bool ShouldShowLightGuide() const;
    void ApplyLightConstants(SceneConstants& constants) const;
    void UpdateConstantBuffers(FXMVECTOR forward, FXMVECTOR right);
    UINT64 ConstantBufferOffset(UINT frameIndex, UINT modelIndex) const;
    void PopulateCommandList();
    void ResetSelectedModel();
    void SyncEditorToSelectedModel();
    void UpdateTrackLabels();
    static std::wstring FormatFloatLabel(const wchar_t* label, float value);
    static std::wstring FormatDegreeLabel(const wchar_t* label, float radians);
    void WaitForGpu();
    void MoveToNextFrame();
    void UpdateWindowTitle();

    // Win32 窗口和左侧编辑器控件状态。
    HINSTANCE m_instance = nullptr;
    HWND m_hwnd = nullptr;
    HWND m_panelBackground = nullptr;
    HWND m_modelList = nullptr;
    HWND m_visibleCheck = nullptr;
    HWND m_fpsLabel = nullptr;
    HWND m_backgroundGrayTrackbar = nullptr;
    HWND m_backgroundGrayLabel = nullptr;
    HWND m_gameModeLabel = nullptr;
    HWND m_gameDifficultyLabel = nullptr;
    HWND m_gameStateLabel = nullptr;
    HWND m_gameScoreLabel = nullptr;
    HWND m_gameHighScoreLabel = nullptr;
    HWND m_gameHintLabel = nullptr;
    HWND m_editorScrollbar = nullptr;
    HWND m_hudFpsLabel = nullptr;
    HWND m_hudScoreLabel = nullptr;
    HWND m_hudHitRateLabel = nullptr;
    HWND m_hudTimeLabel = nullptr;
    HWND m_hudNoonPromptLabel = nullptr;
    HWND m_lightFluctuationButton = nullptr;
    HFONT m_noonPromptFont = nullptr;
    std::array<HWND, TrackCount> m_trackbars{};
    std::array<HWND, TrackCount> m_trackLabels{};
    std::array<HWND, CrosshairTrackCount> m_crosshairTrackbars{};
    std::array<HWND, CrosshairTrackCount> m_crosshairLabels{};
    std::array<HWND, LightTrackCount> m_lightTrackbars{};
    std::array<HWND, LightTrackCount> m_lightLabels{};
    std::array<HWND, static_cast<size_t>(EditorSection::Count)> m_sectionButtons{};
    std::vector<HWND> m_editorControls;
    std::vector<HWND> m_fixedEditorControls;
    std::vector<HWND> m_gameHudControls;
    std::array<std::vector<HWND>, static_cast<size_t>(EditorSection::Count)> m_sectionControls;
    std::vector<EditorControlPlacement> m_sectionControlPlacements;
    EditorSection m_currentCreatedSection = EditorSection::None;
    EditorSection m_activeEditorSection = EditorSection::Model;
    int m_editorBodyTop = 0;
    int m_editorScrollOffset = 0;
    int m_editorScrollMax = 0;
    bool m_editorVisible = true;
    bool m_syncingEditor = false;

    // 窗口尺寸、全屏和交换链显示策略。
    RECT m_windowedRect{};
    RECT m_fullscreenRect{};
    bool m_isFullscreen = true;
    UINT m_renderWidth = WindowWidth;
    UINT m_renderHeight = WindowHeight;
    bool m_vsyncEnabled = false;
    bool m_tearingSupported = false;

    std::filesystem::path m_assetRoot;
    std::filesystem::path m_userDataRoot;

    // DX12 核心对象和每帧资源。
    ComPtr<ID3D12Device> m_device;
    ComPtr<ID3D12CommandQueue> m_commandQueue;
    ComPtr<IDXGISwapChain3> m_swapChain;
    ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
    ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
    ComPtr<ID3D12Resource> m_depthStencil;
    ComPtr<ID3D12CommandAllocator> m_commandAllocators[FrameCount];
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_pipelineState;
    ComPtr<ID3D12PipelineState> m_lightGuidePipelineState;
    ComPtr<ID3D12GraphicsCommandList> m_commandList;
    ComPtr<ID3D12Resource> m_constantBuffer;
    ComPtr<ID3D12Fence> m_fence;

    D3D12_VIEWPORT m_viewport{};
    D3D12_RECT m_scissorRect{};

    // 可编辑渲染状态、灯光和场景模型。
    PipelineSettings m_pipelineSettings{};
    LightSettings m_light{};
    std::vector<ModelInstance> m_models;
    ModelInstance m_targetMesh;
    ModelInstance m_noonBodyMesh;
    ModelInstance m_crosshairMesh;
    ModelInstance m_lightRangeMesh;
    size_t m_aimWallModelIndex = std::numeric_limits<size_t>::max();
    size_t m_selectedModel = 0;

    // 练枪运行时状态，开始游戏会清空目标和分数，结束后恢复场景。
    AimMode m_gameMode = AimMode::Balloon;
    AimDifficulty m_gameDifficulty = AimDifficulty::Hard;
    std::array<AimDifficulty, AimModeCount> m_modeDifficulties{
        AimDifficulty::Hard,
        AimDifficulty::Medium,
        AimDifficulty::Hard,
        AimDifficulty::Hard};
    bool m_gameActive = false;
    bool m_leftFiring = false;
    bool m_ignoreNextGameMouseMove = false;
    int m_gameScore = 0;
    int m_shotsFired = 0;
    int m_shotsHit = 0;
    float m_trackingFireTime = 0.0f;
    float m_trackingHitTime = 0.0f;
    float m_fractionalScore = 0.0f;
    float m_balloonSpawnTimer = 0.0f;
    float m_gameRemainingSeconds = 60.0f;
    float m_nextAimTrainerLabelUpdateTime = 0.0f;
    NoonPhase m_noonPhase = NoonPhase::Inactive;
    int m_noonRoundIndex = 0;
    int m_noonBulletsRemaining = 0;
    int m_noonRoundBaseScore = 0;
    int m_noonTotalKills = 0;
    int m_noonExpectedKills = 0;
    bool m_noonPerfectHeadshots = true;
    bool m_noonRoundEnding = false;
    float m_noonPhaseTimer = 0.0f;
    float m_noonRoundElapsed = 0.0f;
    float m_noonFeedbackTimer = 0.0f;
    std::wstring m_noonPromptText;
    COLORREF m_noonPromptColor = RGB(245, 245, 245);
    CrosshairSettings m_crosshair{};
    GameSavedState m_gameSavedState{};
    HighScores m_highScores{};
    bool m_highScoreDirty = false;
    std::vector<AimTarget> m_gameTargets;
    std::mt19937 m_rng{std::random_device{}()};

    // 输入、相机和帧率统计。
    std::array<bool, 256> m_keys{};
    XMFLOAT3 m_cameraPosition{0.0f, 2.2f, -8.5f};
    float m_cameraYaw = 0.0f;
    float m_cameraPitch = -0.10f;
    float m_totalTime = 0.0f;
    float m_fpsElapsed = 0.0f;
    float m_currentFps = 0.0f;
    int m_fpsFrameCount = 0;

    bool m_mouseLook = false;
    int m_lastMouseX = 0;
    int m_lastMouseY = 0;
    int m_lastAimX = static_cast<int>(WindowWidth / 2);
    int m_lastAimY = static_cast<int>(WindowHeight / 2);
    XMFLOAT2 m_recentAimVelocityOnPlane{};
    float m_recentAimMoveTime = -1000.0f;
    bool m_hasRecentAimMove = false;
    std::chrono::steady_clock::time_point m_lastAimMoveClock{};
    bool m_hasAimMoveClock = false;

    // Fence/常量缓冲区偏移，用来保证 CPU/GPU 帧同步。
    UINT m_frameIndex = 0;
    UINT m_rtvDescriptorSize = 0;
    UINT m_constantBufferStride = 0;
    UINT8* m_constantBufferDataBegin = nullptr;
    UINT64 m_fenceValues[FrameCount]{};
    HANDLE m_fenceEvent = nullptr;
};
