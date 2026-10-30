#include "Dx12App.h"

void Dx12App::WaitForGpu()
{
    // 强制等待当前帧完成，适合重建管线或程序退出前使用。
    ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), m_fenceValues[m_frameIndex]));
    ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent));
    WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);
    ++m_fenceValues[m_frameIndex];
}

void Dx12App::MoveToNextFrame()
{
    // 双缓冲帧同步：CPU 只在即将写入 GPU 尚未完成的帧时等待。
    const UINT64 currentFenceValue = m_fenceValues[m_frameIndex];
    ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), currentFenceValue));

    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    if (m_fence->GetCompletedValue() < m_fenceValues[m_frameIndex])
    {
        ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent));
        WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);
    }

    m_fenceValues[m_frameIndex] = currentFenceValue + 1;
}

void Dx12App::UpdateWindowTitle()
{
    std::wstring title = AppDisplayName;
    title += L" | ";
    title += m_isFullscreen ? L"全屏 " : L"窗口 ";
    title += m_pipelineSettings.wireframe ? L"| 线框 " : L"| 实体 ";
    title += m_pipelineSettings.backfaceCulling ? L"| 背面剔除 " : L"| 不剔除 ";
    title += m_pipelineSettings.autoRotate ? L"| 自动旋转 " : L"| 手动 ";
    title += L"| F11 全屏 | Tab 界面 | R 重载 | T 自动旋转";
    SetWindowTextW(m_hwnd, title.c_str());
}
