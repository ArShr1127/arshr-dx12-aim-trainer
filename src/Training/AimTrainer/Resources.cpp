#include "../../App/Dx12App.h"


namespace
{
    // 目前用内嵌 HLSL 保证示例工程开箱即跑，后续可替换成外部 shader 文件。
    constexpr std::string_view BasicShaderSource = R"(
cbuffer SceneConstants : register(b0)
{
    row_major float4x4 gWorldViewProjection;
    row_major float4x4 gWorld;
    float4 gCameraPosition;
    float4 gModelTint;
    float4 gOptions;
    float4 gLightDirectionIntensity;
    float4 gLightColorAmbient;
};

struct VSInput
{
    float3 position : POSITION;
    float4 color : COLOR;
    float3 normal : NORMAL;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float3 normal : NORMAL;
    float3 worldPosition : TEXCOORD0;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    const float4 worldPosition = mul(float4(input.position, 1.0f), gWorld);

    output.position = mul(float4(input.position, 1.0f), gWorldViewProjection);
    output.color = input.color * gModelTint;
    output.normal = normalize(mul(float4(input.normal, 0.0f), gWorld).xyz);
    output.worldPosition = worldPosition.xyz;
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    if (gOptions.w < -0.5f)
    {
        return float4(input.color.rgb, input.color.a);
    }

    const float3 normal = normalize(input.normal);
    const float3 lightDirection = normalize(gLightDirectionIntensity.xyz);
    const float3 viewDirection = normalize(gCameraPosition.xyz - input.worldPosition);
    const float3 halfVector = normalize(lightDirection + viewDirection);

    const float diffuse = saturate(dot(normal, lightDirection)) * gLightDirectionIntensity.w;
    const float specular = pow(saturate(dot(normal, halfVector)), 32.0f) * 0.22f * gLightDirectionIntensity.w;
    const float wave = gOptions.y > 0.5f ? 1.0f : 0.0f;
    const float selectedPulse = gOptions.x * (0.16f + wave * 0.08f * sin(gOptions.z * 5.0f));
    const float pulse = wave * 0.04f * sin(gOptions.z * 2.0f);

    const float3 lightColor = gLightColorAmbient.rgb;
    const float ambient = gLightColorAmbient.w;
    const float3 litColor = input.color.rgb * ((ambient + diffuse * 0.76f + pulse + selectedPulse) * lightColor) + specular.xxx;
    return float4(litColor, input.color.a);
}
)";
}

void Dx12App::ToggleFullscreen()
{
    m_isFullscreen = !m_isFullscreen;

    if (m_isFullscreen)
    {
        SetWindowLongPtrW(m_hwnd, GWL_STYLE, WS_POPUP | WS_CLIPCHILDREN);
        SetWindowPos(
            m_hwnd,
            HWND_TOP,
            m_fullscreenRect.left,
            m_fullscreenRect.top,
            m_fullscreenRect.right - m_fullscreenRect.left,
            m_fullscreenRect.bottom - m_fullscreenRect.top,
            SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    }
    else
    {
        SetWindowLongPtrW(m_hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN);
        SetWindowPos(
            m_hwnd,
            HWND_TOP,
            m_windowedRect.left,
            m_windowedRect.top,
            m_windowedRect.right - m_windowedRect.left,
            m_windowedRect.bottom - m_windowedRect.top,
            SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    }

    UpdateWindowTitle();
}

std::filesystem::path Dx12App::FindAssetRoot() const
{
    // 同时支持从工程目录、VS/VSCode 构建目录和发布目录启动。
    std::array<std::filesystem::path, 5> candidates{};
    candidates[0] = std::filesystem::current_path();

    wchar_t modulePath[MAX_PATH]{};
    GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
    const std::filesystem::path exeDir = std::filesystem::path{modulePath}.parent_path();

    candidates[1] = exeDir;
    candidates[2] = exeDir.parent_path();
    candidates[3] = exeDir.parent_path().parent_path();
    candidates[4] = exeDir.parent_path().parent_path().parent_path();

    for (const auto& candidate : candidates)
    {
        if (std::filesystem::exists(candidate / "assets" / "pipeline_state.txt"))
        {
            return candidate;
        }
    }

    return exeDir;
}

std::filesystem::path Dx12App::FindUserDataRoot() const
{
    wchar_t localAppData[MAX_PATH]{};
    const DWORD length = GetEnvironmentVariableW(L"LOCALAPPDATA", localAppData, MAX_PATH);
    if (length > 0 && length < MAX_PATH)
    {
        return std::filesystem::path{localAppData} / AppDisplayName;
    }

    wchar_t modulePath[MAX_PATH]{};
    GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
    return std::filesystem::path{modulePath}.parent_path();
}

PipelineSettings Dx12App::LoadPipelineSettings() const
{
    // 简单 key=value 配置，便于用户直接编辑渲染开关。
    PipelineSettings settings{};
    const std::filesystem::path path = m_assetRoot / "assets" / "pipeline_state.txt";
    std::ifstream file{path};

    if (!file)
    {
        return settings;
    }

    std::string line;
    while (std::getline(file, line))
    {
        const auto comment = line.find('#');
        if (comment != std::string::npos)
        {
            line.erase(comment);
        }

        const auto equals = line.find('=');
        if (equals == std::string::npos)
        {
            continue;
        }

        const std::string key = Trim(std::string_view{line}.substr(0, equals));
        std::string value = Trim(std::string_view{line}.substr(equals + 1));

        if (key == "clearColor")
        {
            for (char& ch : value)
            {
                if (ch == ',')
                {
                    ch = ' ';
                }
            }

            std::istringstream stream{value};
            stream >> settings.clearColor[0] >> settings.clearColor[1] >> settings.clearColor[2] >> settings.clearColor[3];
        }
        else if (key == "wireframe")
        {
            settings.wireframe = ParseBool(value, settings.wireframe);
        }
        else if (key == "backfaceCulling")
        {
            settings.backfaceCulling = ParseBool(value, settings.backfaceCulling);
        }
        else if (key == "autoRotate")
        {
            settings.autoRotate = ParseBool(value, settings.autoRotate);
        }
        else if (key == "fovDegrees")
        {
            settings.fovDegrees = std::clamp(std::stof(value), 25.0f, 100.0f);
        }
    }

    return settings;
}

void Dx12App::LoadPipeline()
{
    // DX12 设备、交换链、RTV/DSV 和基础视口都在这里初始化。
    UINT factoryFlags = 0;
#if defined(_DEBUG)
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
    {
        debugController->EnableDebugLayer();
        factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    }
#endif

    ComPtr<IDXGIFactory6> factory;
    ThrowIfFailed(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&factory)));

    BOOL allowTearing = FALSE;
    ComPtr<IDXGIFactory5> factory5;
    if (SUCCEEDED(factory.As(&factory5)))
    {
        if (SUCCEEDED(factory5->CheckFeatureSupport(
            DXGI_FEATURE_PRESENT_ALLOW_TEARING,
            &allowTearing,
            sizeof(allowTearing))))
        {
            m_tearingSupported = allowTearing == TRUE;
        }
    }

    ThrowIfFailed(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)));

    D3D12_COMMAND_QUEUE_DESC queueDesc{};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
    swapChainDesc.BufferCount = FrameCount;
    swapChainDesc.Width = m_renderWidth;
    swapChainDesc.Height = m_renderHeight;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.Flags = m_tearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

    ComPtr<IDXGISwapChain1> swapChain;
    ThrowIfFailed(factory->CreateSwapChainForHwnd(
        m_commandQueue.Get(),
        m_hwnd,
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain));

    ThrowIfFailed(factory->MakeWindowAssociation(m_hwnd, DXGI_MWA_NO_ALT_ENTER));
    ThrowIfFailed(swapChain.As(&m_swapChain));
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
    rtvHeapDesc.NumDescriptors = FrameCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    ThrowIfFailed(m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)));

    m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    for (UINT index = 0; index < FrameCount; ++index)
    {
        ThrowIfFailed(m_swapChain->GetBuffer(index, IID_PPV_ARGS(&m_renderTargets[index])));
        m_device->CreateRenderTargetView(m_renderTargets[index].Get(), nullptr, rtvHandle);
        rtvHandle.ptr += m_rtvDescriptorSize;

        ThrowIfFailed(m_device->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(&m_commandAllocators[index])));
    }

    D3D12_CLEAR_VALUE depthClearValue{};
    depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;
    depthClearValue.DepthStencil.Depth = 1.0f;

    const D3D12_HEAP_PROPERTIES defaultHeap = HeapProperties(D3D12_HEAP_TYPE_DEFAULT);
    const D3D12_RESOURCE_DESC depthDesc = DepthResourceDesc(m_renderWidth, m_renderHeight);
    ThrowIfFailed(m_device->CreateCommittedResource(
        &defaultHeap,
        D3D12_HEAP_FLAG_NONE,
        &depthDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &depthClearValue,
        IID_PPV_ARGS(&m_depthStencil)));

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
    dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    m_device->CreateDepthStencilView(m_depthStencil.Get(), &dsvDesc, m_dsvHeap->GetCPUDescriptorHandleForHeapStart());

    m_viewport = D3D12_VIEWPORT{
        0.0f,
        0.0f,
        static_cast<float>(m_renderWidth),
        static_cast<float>(m_renderHeight),
        0.0f,
        1.0f};
    m_scissorRect = RECT{0, 0, static_cast<LONG>(m_renderWidth), static_cast<LONG>(m_renderHeight)};
}

void Dx12App::LoadAssets()
{
    // 先建根签名和 PSO，再创建模型 Buffer、常量 Buffer 和同步对象。
    CreateRootSignature();

    m_pipelineSettings = LoadPipelineSettings();
    m_pipelineState = CreatePipelineState(m_pipelineSettings);
    m_lightGuidePipelineState = CreatePipelineState(
        m_pipelineSettings,
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE,
        false);

    ThrowIfFailed(m_device->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        m_commandAllocators[m_frameIndex].Get(),
        m_pipelineState.Get(),
        IID_PPV_ARGS(&m_commandList)));
    ThrowIfFailed(m_commandList->Close());

    CreateSceneModels();
    CreateModelBuffers();
    PopulateModelList();
    CreateConstantBuffer();

    ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
    m_fenceValues[m_frameIndex] = 1;

    m_fenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (!m_fenceEvent)
    {
        ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
    }
}

void Dx12App::CreateRootSignature()
{
    // 当前基础管线只有一个 b0 常量缓冲，所有绘制物体共用同一根签名。
    D3D12_ROOT_PARAMETER rootParameters[1]{};
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[0].Descriptor.ShaderRegister = 0;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
    rootSignatureDesc.NumParameters = 1;
    rootSignatureDesc.pParameters = rootParameters;
    rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    const HRESULT serializeResult = D3D12SerializeRootSignature(
        &rootSignatureDesc,
        D3D_ROOT_SIGNATURE_VERSION_1,
        &signature,
        &error);

    if (FAILED(serializeResult))
    {
        std::string message = "Root signature serialization failed.";
        if (error)
        {
            message.append("\n");
            message.append(static_cast<const char*>(error->GetBufferPointer()), error->GetBufferSize());
        }
        throw std::runtime_error(message);
    }

    ThrowIfFailed(m_device->CreateRootSignature(
        0,
        signature->GetBufferPointer(),
        signature->GetBufferSize(),
        IID_PPV_ARGS(&m_rootSignature)));
}

ComPtr<ID3D12PipelineState> Dx12App::CreatePipelineState(
    const PipelineSettings& settings,
    D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopology,
    bool depthEnabled)
{
    // 线框光照范围和实体模型共用 shader，只切换拓扑、深度和光栅化状态。
    ComPtr<ID3DBlob> vertexShader = CompileShaderFromSource(BasicShaderSource, "embedded basic shader", "VSMain", "vs_5_0");
    ComPtr<ID3DBlob> pixelShader = CompileShaderFromSource(BasicShaderSource, "embedded basic shader", "PSMain", "ps_5_0");

    std::array<D3D12_INPUT_ELEMENT_DESC, 3> inputElementDescs{{
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    }};

    D3D12_RASTERIZER_DESC rasterizerDesc{};
    rasterizerDesc.FillMode = settings.wireframe ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;
    rasterizerDesc.CullMode = settings.backfaceCulling ? D3D12_CULL_MODE_BACK : D3D12_CULL_MODE_NONE;
    rasterizerDesc.FrontCounterClockwise = FALSE;
    rasterizerDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    rasterizerDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    rasterizerDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    rasterizerDesc.DepthClipEnable = TRUE;
    rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    D3D12_BLEND_DESC blendDesc{};
    blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
    depthStencilDesc.DepthEnable = depthEnabled ? TRUE : FALSE;
    depthStencilDesc.DepthWriteMask = depthEnabled ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    depthStencilDesc.StencilEnable = FALSE;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc{};
    pipelineStateDesc.InputLayout = {inputElementDescs.data(), static_cast<UINT>(inputElementDescs.size())};
    pipelineStateDesc.pRootSignature = m_rootSignature.Get();
    pipelineStateDesc.VS = {vertexShader->GetBufferPointer(), vertexShader->GetBufferSize()};
    pipelineStateDesc.PS = {pixelShader->GetBufferPointer(), pixelShader->GetBufferSize()};
    pipelineStateDesc.RasterizerState = rasterizerDesc;
    pipelineStateDesc.BlendState = blendDesc;
    pipelineStateDesc.DepthStencilState = depthStencilDesc;
    pipelineStateDesc.SampleMask = UINT_MAX;
    pipelineStateDesc.PrimitiveTopologyType = primitiveTopology;
    pipelineStateDesc.NumRenderTargets = 1;
    pipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    pipelineStateDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    pipelineStateDesc.SampleDesc.Count = 1;

    ComPtr<ID3D12PipelineState> pipelineState;
    ThrowIfFailed(m_device->CreateGraphicsPipelineState(&pipelineStateDesc, IID_PPV_ARGS(&pipelineState)));
    return pipelineState;
}

void Dx12App::FinalizeDefaults(ModelInstance& model)
{
    model.defaultPosition = model.position;
    model.defaultRotation = model.rotation;
    model.defaultScale = model.scale;
    model.defaultTint = model.tint;
    model.defaultVisible = model.visible;
}

void Dx12App::CreateSceneModels()
{
    // 场景模型、训练墙、目标球、准心和光源线框都在这里生成默认实例。
    m_models.clear();

    ModelInstance cube{};
    cube.name = L"上移方块";
    cube.mesh = CreateCubeMesh();
    cube.position = XMFLOAT3{-2.2f, 1.35f, 0.0f};
    cube.scale = XMFLOAT3{0.72f, 0.72f, 0.72f};
    cube.tint = XMFLOAT4{1.0f, 1.0f, 1.0f, 1.0f};
    FinalizeDefaults(cube);
    m_models.push_back(std::move(cube));

    ModelInstance pyramid{};
    pyramid.name = L"金字塔";
    pyramid.mesh = CreatePyramidMesh();
    pyramid.position = XMFLOAT3{0.2f, 1.05f, 0.0f};
    pyramid.scale = XMFLOAT3{0.85f, 0.85f, 0.85f};
    pyramid.tint = XMFLOAT4{1.0f, 0.96f, 0.86f, 1.0f};
    FinalizeDefaults(pyramid);
    m_models.push_back(std::move(pyramid));

    ModelInstance diamond{};
    diamond.name = L"钻石体";
    diamond.mesh = CreateDiamondMesh();
    diamond.position = XMFLOAT3{2.35f, 1.25f, 0.0f};
    diamond.scale = XMFLOAT3{0.78f, 0.78f, 0.78f};
    diamond.tint = XMFLOAT4{0.90f, 1.0f, 1.0f, 1.0f};
    FinalizeDefaults(diamond);
    m_models.push_back(std::move(diamond));

    ModelInstance prism{};
    prism.name = L"三棱柱";
    prism.mesh = CreatePrismMesh();
    prism.position = XMFLOAT3{0.5f, 0.95f, 2.25f};
    prism.rotation = XMFLOAT3{0.0f, 0.65f, 0.0f};
    prism.scale = XMFLOAT3{0.75f, 0.75f, 0.75f};
    prism.tint = XMFLOAT4{0.92f, 0.96f, 1.0f, 1.0f};
    FinalizeDefaults(prism);
    m_models.push_back(std::move(prism));

    ModelInstance ground{};
    ground.name = L"地基平台";
    ground.mesh = CreateGroundMesh();
    ground.position = XMFLOAT3{0.0f, -0.08f, 0.7f};
    ground.scale = XMFLOAT3{1.18f, 1.18f, 1.18f};
    ground.tint = XMFLOAT4{1.0f, 1.0f, 1.0f, 1.0f};
    ground.rotationLocked = true;
    FinalizeDefaults(ground);
    m_models.push_back(std::move(ground));

    ModelInstance wall{};
    wall.name = L"训练墙体";
    wall.mesh = CreateAimWallMesh();
    wall.position = XMFLOAT3{0.0f, 0.0f, AimWallZ};
    wall.scale = XMFLOAT3{1.0f, 1.0f, 1.0f};
    wall.tint = XMFLOAT4{1.0f, 1.0f, 1.0f, 1.0f};
    wall.rotationLocked = true;
    wall.visibleDuringGame = true;
    FinalizeDefaults(wall);
    m_aimWallModelIndex = m_models.size();
    m_models.push_back(std::move(wall));

    m_targetMesh.name = L"训练目标";
    m_targetMesh.mesh = CreateSphereMesh();
    m_targetMesh.tint = XMFLOAT4{1.0f, 1.0f, 1.0f, 1.0f};
    FinalizeDefaults(m_targetMesh);

    m_noonBodyMesh.name = L"午时身体";
    m_noonBodyMesh.mesh = CreateCylinderMesh();
    m_noonBodyMesh.tint = XMFLOAT4{1.0f, 1.0f, 1.0f, 1.0f};
    FinalizeDefaults(m_noonBodyMesh);

    m_crosshairMesh.name = L"游戏准心";
    m_crosshairMesh.mesh = CreateCrosshairDotMesh();
    m_crosshairMesh.tint = XMFLOAT4{1.0f, 1.0f, 1.0f, 1.0f};
    FinalizeDefaults(m_crosshairMesh);

    m_lightRangeMesh.name = L"光照范围线框";
    m_lightRangeMesh.mesh = CreateLightRangeMesh();
    m_lightRangeMesh.tint = XMFLOAT4{1.0f, 0.95f, 0.32f, 1.0f};
    m_lightRangeMesh.rotationLocked = true;
    FinalizeDefaults(m_lightRangeMesh);

    m_selectedModel = 0;
}

void Dx12App::CreateModelBuffers()
{
    const D3D12_HEAP_PROPERTIES uploadHeap = HeapProperties(D3D12_HEAP_TYPE_UPLOAD);
    D3D12_RANGE readRange{0, 0};

    for (ModelInstance& model : m_models)
    {
        CreateBuffersForModel(model, uploadHeap, readRange);
    }

    CreateBuffersForModel(m_targetMesh, uploadHeap, readRange);
    CreateBuffersForModel(m_noonBodyMesh, uploadHeap, readRange);
    CreateBuffersForModel(m_crosshairMesh, uploadHeap, readRange);
    CreateBuffersForModel(m_lightRangeMesh, uploadHeap, readRange);
}

void Dx12App::CreateBuffersForModel(
    ModelInstance& model,
    const D3D12_HEAP_PROPERTIES& uploadHeap,
    const D3D12_RANGE& readRange)
{
    // 这是教学型管线，顶点/索引直接放 Upload Heap，结构更直观。
    const UINT vertexBufferSize = static_cast<UINT>(sizeof(Vertex) * model.mesh.vertices.size());
    const UINT indexBufferSize = static_cast<UINT>(sizeof(std::uint16_t) * model.mesh.indices.size());

    const D3D12_RESOURCE_DESC vertexBufferDesc = BufferResourceDesc(vertexBufferSize);
    ThrowIfFailed(m_device->CreateCommittedResource(
        &uploadHeap,
        D3D12_HEAP_FLAG_NONE,
        &vertexBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&model.vertexBuffer)));

    UINT8* vertexDataBegin = nullptr;
    ThrowIfFailed(model.vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&vertexDataBegin)));
    std::memcpy(vertexDataBegin, model.mesh.vertices.data(), vertexBufferSize);
    model.vertexBuffer->Unmap(0, nullptr);

    model.vertexBufferView.BufferLocation = model.vertexBuffer->GetGPUVirtualAddress();
    model.vertexBufferView.StrideInBytes = sizeof(Vertex);
    model.vertexBufferView.SizeInBytes = vertexBufferSize;

    const D3D12_RESOURCE_DESC indexBufferDesc = BufferResourceDesc(indexBufferSize);
    ThrowIfFailed(m_device->CreateCommittedResource(
        &uploadHeap,
        D3D12_HEAP_FLAG_NONE,
        &indexBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&model.indexBuffer)));

    UINT8* indexDataBegin = nullptr;
    ThrowIfFailed(model.indexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&indexDataBegin)));
    std::memcpy(indexDataBegin, model.mesh.indices.data(), indexBufferSize);
    model.indexBuffer->Unmap(0, nullptr);

    model.indexBufferView.BufferLocation = model.indexBuffer->GetGPUVirtualAddress();
    model.indexBufferView.Format = DXGI_FORMAT_R16_UINT;
    model.indexBufferView.SizeInBytes = indexBufferSize;
    model.indexCount = static_cast<UINT>(model.mesh.indices.size());
}

void Dx12App::PopulateModelList()
{
    SendMessageW(m_modelList, LB_RESETCONTENT, 0, 0);
    for (const ModelInstance& model : m_models)
    {
        SendMessageW(m_modelList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(model.name.c_str()));
    }
    SendMessageW(m_modelList, LB_SETCURSEL, static_cast<WPARAM>(m_selectedModel), 0);
}

void Dx12App::CreateConstantBuffer()
{
    // 给每个 back buffer 预留一整套对象常量，避免帧间覆盖。
    m_constantBufferStride = Align256(sizeof(SceneConstants));
    const UINT modelCount = static_cast<UINT>(std::max<size_t>(1, m_models.size() + MaxGameTargets * 2 + 2));
    const D3D12_HEAP_PROPERTIES uploadHeap = HeapProperties(D3D12_HEAP_TYPE_UPLOAD);
    const D3D12_RESOURCE_DESC constantBufferDesc = BufferResourceDesc(m_constantBufferStride * FrameCount * modelCount);

    ThrowIfFailed(m_device->CreateCommittedResource(
        &uploadHeap,
        D3D12_HEAP_FLAG_NONE,
        &constantBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_constantBuffer)));

    D3D12_RANGE readRange{0, 0};
    ThrowIfFailed(m_constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_constantBufferDataBegin)));
}
