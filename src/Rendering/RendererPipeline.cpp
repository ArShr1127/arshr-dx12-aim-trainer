#include "../App/Dx12App.h"

XMMATRIX Dx12App::ModelWorldMatrix(const ModelInstance& model, size_t index) const
{
    // 自动旋转只作用于未选中且未锁定的模型，编辑时当前模型保持稳定。
    const float autoSpin = m_pipelineSettings.autoRotate && index != m_selectedModel && !model.rotationLocked
        ? m_totalTime * (0.18f + static_cast<float>(index) * 0.06f)
        : 0.0f;

    return XMMatrixScaling(model.scale.x, model.scale.y, model.scale.z) *
        XMMatrixRotationRollPitchYaw(model.rotation.x, model.rotation.y + autoSpin, model.rotation.z) *
        XMMatrixTranslation(model.position.x, model.position.y, model.position.z);
}

XMMATRIX Dx12App::TargetWorldMatrix(const AimTarget& target) const
{
    return XMMatrixScaling(target.radius, target.radius, target.radius) *
        XMMatrixTranslation(target.position.x, target.position.y, target.position.z);
}

XMMATRIX Dx12App::NoonBodyWorldMatrix(const AimTarget& target) const
{
    const XMFLOAT3 center = NoonBodyCenter(target);
    return XMMatrixScaling(target.bodyRadius, target.bodyHalfHeight, target.bodyRadius) *
        XMMatrixTranslation(center.x, center.y, center.z);
}

XMMATRIX Dx12App::CrosshairWorldMatrix(FXMVECTOR forward, FXMVECTOR right, FXMVECTOR up) const
{
    // 准心是相机前方的 billboard，不参与深度瞄准大小，只负责视觉反馈。
    const XMVECTOR eye = XMLoadFloat3(&m_cameraPosition);
    const XMVECTOR center = eye + forward * 1.1f;
    XMFLOAT3 r{};
    XMFLOAT3 u{};
    XMFLOAT3 f{};
    XMFLOAT3 p{};
    XMStoreFloat3(&r, right);
    XMStoreFloat3(&u, up);
    XMStoreFloat3(&f, forward);
    XMStoreFloat3(&p, center);

    const XMMATRIX billboard = XMMatrixSet(
        r.x, r.y, r.z, 0.0f,
        u.x, u.y, u.z, 0.0f,
        f.x, f.y, f.z, 0.0f,
        p.x, p.y, p.z, 1.0f);

    return XMMatrixScaling(m_crosshair.size, m_crosshair.size, m_crosshair.size) * billboard;
}

XMMATRIX Dx12App::LightRangeWorldMatrix() const
{
    // 光照辅助线框跟随光源方向和强度缩放，方便在编辑模块里观察范围。
    const XMFLOAT3 lightDirection = ComputeLightDirection(m_light);
    const float yaw = (m_light.yawDegrees + 180.0f) * Pi / 180.0f;
    const float pitch = -m_light.pitchDegrees * Pi / 180.0f;
    const float guideLength = 7.0f;
    const float guideRadius = 3.0f + std::clamp(m_light.intensity, 0.0f, 2.0f) * 0.75f;
    const XMFLOAT3 sceneCenter{0.0f, 2.25f, 1.55f};
    const XMFLOAT3 apex{
        sceneCenter.x + lightDirection.x * guideLength * 0.45f,
        sceneCenter.y + lightDirection.y * guideLength * 0.45f,
        sceneCenter.z + lightDirection.z * guideLength * 0.45f};

    return XMMatrixScaling(guideRadius, guideRadius, guideLength) *
        XMMatrixRotationRollPitchYaw(pitch, yaw, 0.0f) *
        XMMatrixTranslation(apex.x, apex.y, apex.z);
}

bool Dx12App::ShouldShowLightGuide() const
{
    return m_editorVisible && !m_gameActive && m_activeEditorSection == EditorSection::Light;
}

void Dx12App::ApplyLightConstants(SceneConstants& constants) const
{
    const XMFLOAT3 direction = ComputeLightDirection(m_light);
    constants.lightDirectionIntensity = XMFLOAT4{
        direction.x,
        direction.y,
        direction.z,
        m_light.intensity};
    constants.lightColorAmbient = XMFLOAT4{
        m_light.color.x,
        m_light.color.y,
        m_light.color.z,
        m_light.ambient};
}

void Dx12App::UpdateConstantBuffers(FXMVECTOR forward, FXMVECTOR right)
{
    // 每帧为模型、训练目标、准心和光照线框分别写入常量缓冲区。
    const XMVECTOR eye = XMLoadFloat3(&m_cameraPosition);
    const XMVECTOR up = XMVector3Normalize(XMVector3Cross(forward, right));
    const XMMATRIX view = XMMatrixLookToLH(eye, forward, up);
    const XMMATRIX projection = XMMatrixPerspectiveFovLH(
        m_pipelineSettings.fovDegrees * Pi / 180.0f,
        static_cast<float>(m_renderWidth) / static_cast<float>(m_renderHeight),
        0.1f,
        100.0f);

    const UINT modelCount = static_cast<UINT>(m_models.size());
    for (UINT index = 0; index < modelCount; ++index)
    {
        const ModelInstance& model = m_models[index];
        const XMMATRIX world = ModelWorldMatrix(model, index);

        SceneConstants constants{};
        XMStoreFloat4x4(&constants.world, world);
        XMStoreFloat4x4(&constants.worldViewProjection, world * view * projection);
        constants.cameraPosition = XMFLOAT4{
            m_cameraPosition.x,
            m_cameraPosition.y,
            m_cameraPosition.z,
            1.0f};
        constants.modelTint = model.tint;
        constants.options = XMFLOAT4{
            index == m_selectedModel ? 1.0f : 0.0f,
            m_light.fluctuation ? 1.0f : 0.0f,
            m_totalTime,
            0.0f};
        ApplyLightConstants(constants);

        std::memcpy(
            m_constantBufferDataBegin + ConstantBufferOffset(m_frameIndex, index),
            &constants,
            sizeof(constants));
    }

    const UINT targetBase = static_cast<UINT>(m_models.size());
    const UINT noonBodyBase = targetBase + MaxGameTargets;
    for (UINT index = 0; index < static_cast<UINT>(m_gameTargets.size()) && index < MaxGameTargets; ++index)
    {
        const AimTarget& target = m_gameTargets[index];
        if (!target.active)
        {
            continue;
        }

        const XMMATRIX world = TargetWorldMatrix(target);
        SceneConstants constants{};
        XMStoreFloat4x4(&constants.world, world);
        XMStoreFloat4x4(&constants.worldViewProjection, world * view * projection);
        constants.cameraPosition = XMFLOAT4{
            m_cameraPosition.x,
            m_cameraPosition.y,
            m_cameraPosition.z,
            1.0f};
        constants.modelTint = target.tint;
        constants.options = XMFLOAT4{
            target.mode == AimMode::Tracking ? 0.7f : 0.0f,
            m_light.fluctuation ? 1.0f : 0.0f,
            m_totalTime,
            static_cast<float>(target.health)};
        ApplyLightConstants(constants);

        std::memcpy(
            m_constantBufferDataBegin + ConstantBufferOffset(m_frameIndex, targetBase + index),
            &constants,
            sizeof(constants));

        if (target.mode == AimMode::Noon)
        {
            const XMMATRIX bodyWorld = NoonBodyWorldMatrix(target);
            SceneConstants bodyConstants{};
            XMStoreFloat4x4(&bodyConstants.world, bodyWorld);
            XMStoreFloat4x4(&bodyConstants.worldViewProjection, bodyWorld * view * projection);
            bodyConstants.cameraPosition = XMFLOAT4{
                m_cameraPosition.x,
                m_cameraPosition.y,
                m_cameraPosition.z,
                1.0f};
            bodyConstants.modelTint = target.bodyTint;
            bodyConstants.options = XMFLOAT4{0.0f, m_light.fluctuation ? 1.0f : 0.0f, m_totalTime, 0.0f};
            ApplyLightConstants(bodyConstants);

            std::memcpy(
                m_constantBufferDataBegin + ConstantBufferOffset(m_frameIndex, noonBodyBase + index),
                &bodyConstants,
                sizeof(bodyConstants));
        }
    }

    if (m_gameActive)
    {
        const UINT crosshairIndex = static_cast<UINT>(m_models.size() + MaxGameTargets * 2);
        const XMMATRIX world = CrosshairWorldMatrix(forward, right, up);
        SceneConstants constants{};
        XMStoreFloat4x4(&constants.world, world);
        XMStoreFloat4x4(&constants.worldViewProjection, world * view * projection);
        constants.cameraPosition = XMFLOAT4{
            m_cameraPosition.x,
            m_cameraPosition.y,
            m_cameraPosition.z,
            1.0f};
        constants.modelTint = m_crosshair.color;
        constants.options = XMFLOAT4{0.0f, 0.0f, m_totalTime, -1.0f};
        ApplyLightConstants(constants);

        std::memcpy(
            m_constantBufferDataBegin + ConstantBufferOffset(m_frameIndex, crosshairIndex),
            &constants,
            sizeof(constants));
    }

    if (ShouldShowLightGuide())
    {
        const UINT lightGuideIndex = static_cast<UINT>(m_models.size() + MaxGameTargets * 2 + 1);
        const XMMATRIX world = LightRangeWorldMatrix();
        SceneConstants constants{};
        XMStoreFloat4x4(&constants.world, world);
        XMStoreFloat4x4(&constants.worldViewProjection, world * view * projection);
        constants.cameraPosition = XMFLOAT4{
            m_cameraPosition.x,
            m_cameraPosition.y,
            m_cameraPosition.z,
            1.0f};
        constants.modelTint = m_lightRangeMesh.tint;
        constants.options = XMFLOAT4{0.0f, 0.0f, m_totalTime, -1.0f};
        ApplyLightConstants(constants);

        std::memcpy(
            m_constantBufferDataBegin + ConstantBufferOffset(m_frameIndex, lightGuideIndex),
            &constants,
            sizeof(constants));
    }
}

UINT64 Dx12App::ConstantBufferOffset(UINT frameIndex, UINT modelIndex) const
{
    // 常量缓冲区按“帧索引 + 绘制对象索引”线性排列，偏移已按 256 字节对齐。
    const UINT modelCount = static_cast<UINT>(std::max<size_t>(1, m_models.size() + MaxGameTargets * 2 + 2));
    return static_cast<UINT64>((frameIndex * modelCount) + modelIndex) * m_constantBufferStride;
}

void Dx12App::PopulateCommandList()
{
    // 录制一帧：清屏、画场景、画游戏目标/准心/辅助线，最后切回 Present。
    ThrowIfFailed(m_commandAllocators[m_frameIndex]->Reset());
    ThrowIfFailed(m_commandList->Reset(m_commandAllocators[m_frameIndex].Get(), m_pipelineState.Get()));

    m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
    m_commandList->RSSetViewports(1, &m_viewport);
    m_commandList->RSSetScissorRects(1, &m_scissorRect);

    D3D12_RESOURCE_BARRIER toRenderTarget = TransitionBarrier(
        m_renderTargets[m_frameIndex].Get(),
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET);
    m_commandList->ResourceBarrier(1, &toRenderTarget);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    rtvHandle.ptr += static_cast<SIZE_T>(m_frameIndex) * m_rtvDescriptorSize;
    const D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();

    m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
    m_commandList->ClearRenderTargetView(rtvHandle, m_pipelineSettings.clearColor.data(), 0, nullptr);
    m_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    for (UINT index = 0; index < static_cast<UINT>(m_models.size()); ++index)
    {
        const ModelInstance& model = m_models[index];
        if (!model.visible)
        {
            continue;
        }
        if (m_gameActive && !model.visibleDuringGame)
        {
            continue;
        }

        const D3D12_GPU_VIRTUAL_ADDRESS constantBufferAddress =
            m_constantBuffer->GetGPUVirtualAddress() + ConstantBufferOffset(m_frameIndex, index);
        m_commandList->SetGraphicsRootConstantBufferView(0, constantBufferAddress);
        m_commandList->IASetVertexBuffers(0, 1, &model.vertexBufferView);
        m_commandList->IASetIndexBuffer(&model.indexBufferView);
        m_commandList->DrawIndexedInstanced(model.indexCount, 1, 0, 0, 0);
    }

    const UINT targetBase = static_cast<UINT>(m_models.size());
    const UINT noonBodyBase = targetBase + MaxGameTargets;
    for (UINT index = 0; index < static_cast<UINT>(m_gameTargets.size()) && index < MaxGameTargets; ++index)
    {
        const AimTarget& target = m_gameTargets[index];
        if (!target.active)
        {
            continue;
        }

        if (target.mode == AimMode::Noon)
        {
            const D3D12_GPU_VIRTUAL_ADDRESS bodyConstantBufferAddress =
                m_constantBuffer->GetGPUVirtualAddress() + ConstantBufferOffset(m_frameIndex, noonBodyBase + index);
            m_commandList->SetGraphicsRootConstantBufferView(0, bodyConstantBufferAddress);
            m_commandList->IASetVertexBuffers(0, 1, &m_noonBodyMesh.vertexBufferView);
            m_commandList->IASetIndexBuffer(&m_noonBodyMesh.indexBufferView);
            m_commandList->DrawIndexedInstanced(m_noonBodyMesh.indexCount, 1, 0, 0, 0);
        }

        const D3D12_GPU_VIRTUAL_ADDRESS constantBufferAddress =
            m_constantBuffer->GetGPUVirtualAddress() + ConstantBufferOffset(m_frameIndex, targetBase + index);
        m_commandList->SetGraphicsRootConstantBufferView(0, constantBufferAddress);
        m_commandList->IASetVertexBuffers(0, 1, &m_targetMesh.vertexBufferView);
        m_commandList->IASetIndexBuffer(&m_targetMesh.indexBufferView);
        m_commandList->DrawIndexedInstanced(m_targetMesh.indexCount, 1, 0, 0, 0);
    }

    if (m_gameActive)
    {
        const UINT crosshairIndex = static_cast<UINT>(m_models.size() + MaxGameTargets * 2);
        const D3D12_GPU_VIRTUAL_ADDRESS constantBufferAddress =
            m_constantBuffer->GetGPUVirtualAddress() + ConstantBufferOffset(m_frameIndex, crosshairIndex);
        m_commandList->SetGraphicsRootConstantBufferView(0, constantBufferAddress);
        m_commandList->IASetVertexBuffers(0, 1, &m_crosshairMesh.vertexBufferView);
        m_commandList->IASetIndexBuffer(&m_crosshairMesh.indexBufferView);
        m_commandList->DrawIndexedInstanced(m_crosshairMesh.indexCount, 1, 0, 0, 0);
    }

    if (ShouldShowLightGuide() && m_lightGuidePipelineState)
    {
        const UINT lightGuideIndex = static_cast<UINT>(m_models.size() + MaxGameTargets * 2 + 1);
        const D3D12_GPU_VIRTUAL_ADDRESS constantBufferAddress =
            m_constantBuffer->GetGPUVirtualAddress() + ConstantBufferOffset(m_frameIndex, lightGuideIndex);
        m_commandList->SetPipelineState(m_lightGuidePipelineState.Get());
        m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
        m_commandList->SetGraphicsRootConstantBufferView(0, constantBufferAddress);
        m_commandList->IASetVertexBuffers(0, 1, &m_lightRangeMesh.vertexBufferView);
        m_commandList->IASetIndexBuffer(&m_lightRangeMesh.indexBufferView);
        m_commandList->DrawIndexedInstanced(m_lightRangeMesh.indexCount, 1, 0, 0, 0);
    }

    D3D12_RESOURCE_BARRIER toPresent = TransitionBarrier(
        m_renderTargets[m_frameIndex].Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT);
    m_commandList->ResourceBarrier(1, &toPresent);

    ThrowIfFailed(m_commandList->Close());
}
