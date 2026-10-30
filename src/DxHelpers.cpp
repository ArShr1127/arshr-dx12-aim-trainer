#include "DxHelpers.h"

#include <cctype>
#include <sstream>
#include <stdexcept>

void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        std::ostringstream stream;
        stream << "HRESULT failure: 0x" << std::hex << static_cast<unsigned long>(hr);
        throw std::runtime_error(stream.str());
    }
}

UINT Align256(UINT value)
{
    // D3D12 常量缓冲区要求 256 字节对齐。
    return (value + 255U) & ~255U;
}

std::string Trim(std::string_view text)
{
    const auto begin = text.find_first_not_of(" \t\r\n");
    if (begin == std::string_view::npos)
    {
        return {};
    }

    const auto end = text.find_last_not_of(" \t\r\n");
    return std::string{text.substr(begin, end - begin + 1)};
}

bool ParseBool(std::string value, bool fallback)
{
    for (char& ch : value)
    {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }

    if (value == "true" || value == "1" || value == "yes" || value == "on")
    {
        return true;
    }

    if (value == "false" || value == "0" || value == "no" || value == "off")
    {
        return false;
    }

    return fallback;
}

D3D12_HEAP_PROPERTIES HeapProperties(D3D12_HEAP_TYPE type)
{
    D3D12_HEAP_PROPERTIES properties{};
    properties.Type = type;
    properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    properties.CreationNodeMask = 1;
    properties.VisibleNodeMask = 1;
    return properties;
}

D3D12_RESOURCE_DESC BufferResourceDesc(UINT64 byteSize)
{
    D3D12_RESOURCE_DESC desc{};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Width = byteSize;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    return desc;
}

D3D12_RESOURCE_DESC DepthResourceDesc(UINT width, UINT height)
{
    D3D12_RESOURCE_DESC desc{};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width = width;
    desc.Height = height;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_D32_FLOAT;
    desc.SampleDesc.Count = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    return desc;
}

D3D12_RESOURCE_BARRIER TransitionBarrier(
    ID3D12Resource* resource,
    D3D12_RESOURCE_STATES before,
    D3D12_RESOURCE_STATES after)
{
    // 这里只封装最常用的 Transition barrier，保持命令录制代码更短。
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = resource;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = before;
    barrier.Transition.StateAfter = after;
    return barrier;
}

ComPtr<ID3DBlob> CompileShaderFromFile(
    const std::filesystem::path& path,
    const char* entryPoint,
    const char* target)
{
    // Debug 下保留调试信息，方便看 shader 编译错误和图形调试。
    UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(_DEBUG)
    compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    ComPtr<ID3DBlob> bytecode;
    ComPtr<ID3DBlob> errors;
    const HRESULT hr = D3DCompileFromFile(
        path.c_str(),
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entryPoint,
        target,
        compileFlags,
        0,
        &bytecode,
        &errors);

    if (FAILED(hr))
    {
        std::string message = "Shader compilation failed: " + path.string();
        if (errors)
        {
            message.append("\n");
            message.append(static_cast<const char*>(errors->GetBufferPointer()), errors->GetBufferSize());
        }
        throw std::runtime_error(message);
    }

    return bytecode;
}

ComPtr<ID3DBlob> CompileShaderFromSource(
    std::string_view source,
    const char* sourceName,
    const char* entryPoint,
    const char* target)
{
    // 当前基础管线使用内嵌 shader，之后也可以替换成文件加载。
    UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(_DEBUG)
    compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    ComPtr<ID3DBlob> bytecode;
    ComPtr<ID3DBlob> errors;
    const HRESULT hr = D3DCompile(
        source.data(),
        source.size(),
        sourceName,
        nullptr,
        nullptr,
        entryPoint,
        target,
        compileFlags,
        0,
        &bytecode,
        &errors);

    if (FAILED(hr))
    {
        std::string message = std::string{"Shader compilation failed: "} + sourceName;
        if (errors)
        {
            message.append("\n");
            message.append(static_cast<const char*>(errors->GetBufferPointer()), errors->GetBufferSize());
        }
        throw std::runtime_error(message);
    }

    return bytecode;
}
