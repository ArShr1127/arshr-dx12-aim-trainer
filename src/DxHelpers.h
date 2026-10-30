#pragma once

#include <filesystem>
#include <string>
#include <string_view>

#include <d3d12.h>
#include <d3dcompiler.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

void ThrowIfFailed(HRESULT hr);
UINT Align256(UINT value);

std::string Trim(std::string_view text);
bool ParseBool(std::string value, bool fallback);

D3D12_HEAP_PROPERTIES HeapProperties(D3D12_HEAP_TYPE type);
D3D12_RESOURCE_DESC BufferResourceDesc(UINT64 byteSize);
D3D12_RESOURCE_DESC DepthResourceDesc(UINT width, UINT height);
D3D12_RESOURCE_BARRIER TransitionBarrier(
    ID3D12Resource* resource,
    D3D12_RESOURCE_STATES before,
    D3D12_RESOURCE_STATES after);

ComPtr<ID3DBlob> CompileShaderFromFile(
    const std::filesystem::path& path,
    const char* entryPoint,
    const char* target);

ComPtr<ID3DBlob> CompileShaderFromSource(
    std::string_view source,
    const char* sourceName,
    const char* entryPoint,
    const char* target);
