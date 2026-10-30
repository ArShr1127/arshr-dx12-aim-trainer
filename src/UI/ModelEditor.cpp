#include "../App/Dx12App.h"

void Dx12App::ResetSelectedModel()
{
    // 重置只回到模型创建时记录的默认值，不影响其他模型。
    if (m_models.empty())
    {
        return;
    }

    ModelInstance& model = m_models[m_selectedModel];
    model.position = model.defaultPosition;
    model.rotation = model.defaultRotation;
    model.scale = model.defaultScale;
    model.tint = model.defaultTint;
    model.visible = model.defaultVisible;
    SyncEditorToSelectedModel();
}

void Dx12App::SyncEditorToSelectedModel()
{
    // 程序主动更新滑条时打开同步标记，防止触发 HandleTrackbar 反写模型。
    if (m_models.empty())
    {
        return;
    }

    m_syncingEditor = true;

    const ModelInstance& model = m_models[m_selectedModel];
    SendMessageW(m_modelList, LB_SETCURSEL, static_cast<WPARAM>(m_selectedModel), 0);
    SendMessageW(m_visibleCheck, BM_SETCHECK, model.visible ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(m_trackbars[TrackPosX], TBM_SETPOS, TRUE, static_cast<LPARAM>(std::lround(model.position.x * 10.0f)));
    SendMessageW(m_trackbars[TrackPosY], TBM_SETPOS, TRUE, static_cast<LPARAM>(std::lround(model.position.y * 10.0f)));
    SendMessageW(m_trackbars[TrackPosZ], TBM_SETPOS, TRUE, static_cast<LPARAM>(std::lround(model.position.z * 10.0f)));
    SendMessageW(m_trackbars[TrackRotX], TBM_SETPOS, TRUE, static_cast<LPARAM>(std::lround(WrapAngle(model.rotation.x) * 100.0f)));
    SendMessageW(m_trackbars[TrackRotY], TBM_SETPOS, TRUE, static_cast<LPARAM>(std::lround(WrapAngle(model.rotation.y) * 100.0f)));
    SendMessageW(m_trackbars[TrackRotZ], TBM_SETPOS, TRUE, static_cast<LPARAM>(std::lround(WrapAngle(model.rotation.z) * 100.0f)));
    SendMessageW(m_trackbars[TrackScale], TBM_SETPOS, TRUE, static_cast<LPARAM>(std::lround(model.scale.x * 10.0f)));
    SendMessageW(m_trackbars[TrackColorR], TBM_SETPOS, TRUE, static_cast<LPARAM>(std::lround(model.tint.x * 100.0f)));
    SendMessageW(m_trackbars[TrackColorG], TBM_SETPOS, TRUE, static_cast<LPARAM>(std::lround(model.tint.y * 100.0f)));
    SendMessageW(m_trackbars[TrackColorB], TBM_SETPOS, TRUE, static_cast<LPARAM>(std::lround(model.tint.z * 100.0f)));

    UpdateTrackLabels();
    m_syncingEditor = false;
}

void Dx12App::UpdateTrackLabels()
{
    // 标签显示真实浮点值，滑条本身继续使用整数范围以保持 Win32 控件简单。
    if (m_models.empty())
    {
        return;
    }

    const ModelInstance& model = m_models[m_selectedModel];
    std::array<std::wstring, TrackCount> labels{};
    labels[TrackPosX] = FormatFloatLabel(L"位置 X", model.position.x);
    labels[TrackPosY] = FormatFloatLabel(L"位置 Y", model.position.y);
    labels[TrackPosZ] = FormatFloatLabel(L"位置 Z", model.position.z);
    labels[TrackRotX] = FormatDegreeLabel(model.rotationLocked ? L"旋转 X（已锁定）" : L"旋转 X", model.rotation.x);
    labels[TrackRotY] = FormatDegreeLabel(model.rotationLocked ? L"旋转 Y（已锁定）" : L"旋转 Y", model.rotation.y);
    labels[TrackRotZ] = FormatDegreeLabel(model.rotationLocked ? L"旋转 Z（已锁定）" : L"旋转 Z", model.rotation.z);
    labels[TrackScale] = FormatFloatLabel(L"缩放", model.scale.x);
    labels[TrackColorR] = FormatFloatLabel(L"颜色 R", model.tint.x);
    labels[TrackColorG] = FormatFloatLabel(L"颜色 G", model.tint.y);
    labels[TrackColorB] = FormatFloatLabel(L"颜色 B", model.tint.z);

    for (int index = 0; index < TrackCount; ++index)
    {
        SetWindowTextW(m_trackLabels[index], labels[index].c_str());
    }
}

std::wstring Dx12App::FormatFloatLabel(const wchar_t* label, float value)
{
    wchar_t buffer[96]{};
    swprintf_s(buffer, L"%s: %.2f", label, value);
    return buffer;
}

std::wstring Dx12App::FormatDegreeLabel(const wchar_t* label, float radians)
{
    wchar_t buffer[96]{};
    swprintf_s(buffer, L"%s: %.0f deg", label, WrapAngle(radians) * 180.0f / Pi);
    return buffer;
}
