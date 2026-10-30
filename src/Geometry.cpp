#include "Geometry.h"

#include <cmath>

namespace
{
    constexpr float Pi = 3.14159265358979323846f;
    constexpr float AimWallZ = 4.15f;
    constexpr float AimWallHalfWidth = 4.0f;
    constexpr float AimWallBottom = 0.35f;
    constexpr float AimWallTop = 4.85f;

    XMFLOAT3 FaceNormal(const XMFLOAT3& a, const XMFLOAT3& b, const XMFLOAT3& c)
    {
        // 用三角形叉乘得到面法线，简单模型保持平面光照效果。
        const XMVECTOR va = XMLoadFloat3(&a);
        const XMVECTOR vb = XMLoadFloat3(&b);
        const XMVECTOR vc = XMLoadFloat3(&c);
        XMFLOAT3 normal{};
        XMStoreFloat3(&normal, XMVector3Normalize(XMVector3Cross(vb - va, vc - va)));
        return normal;
    }

    void AddTriangle(MeshData& mesh, const XMFLOAT3& a, const XMFLOAT3& b, const XMFLOAT3& c, const XMFLOAT4& color)
    {
        // 基础图元直接写顶点和索引，调用方不用关心顶点起始位置。
        const XMFLOAT3 normal = FaceNormal(a, b, c);
        const std::uint16_t start = static_cast<std::uint16_t>(mesh.vertices.size());
        mesh.vertices.push_back(Vertex{a, color, normal});
        mesh.vertices.push_back(Vertex{b, color, normal});
        mesh.vertices.push_back(Vertex{c, color, normal});
        mesh.indices.push_back(start);
        mesh.indices.push_back(static_cast<std::uint16_t>(start + 1));
        mesh.indices.push_back(static_cast<std::uint16_t>(start + 2));
    }

    void AddQuad(MeshData& mesh, const XMFLOAT3& a, const XMFLOAT3& b, const XMFLOAT3& c, const XMFLOAT3& d, const XMFLOAT4& color)
    {
        AddTriangle(mesh, a, b, c, color);
        AddTriangle(mesh, a, c, d, color);
    }

    void AddLine(MeshData& mesh, const XMFLOAT3& a, const XMFLOAT3& b, const XMFLOAT4& color)
    {
        const XMFLOAT3 normal{0.0f, 1.0f, 0.0f};
        const std::uint16_t start = static_cast<std::uint16_t>(mesh.vertices.size());
        mesh.vertices.push_back(Vertex{a, color, normal});
        mesh.vertices.push_back(Vertex{b, color, normal});
        mesh.indices.push_back(start);
        mesh.indices.push_back(static_cast<std::uint16_t>(start + 1));
    }
}

MeshData CreateCubeMesh()
{
    MeshData mesh{};
    AddQuad(mesh, {-1.0f, -1.0f, -1.0f}, {-1.0f, 1.0f, -1.0f}, {1.0f, 1.0f, -1.0f}, {1.0f, -1.0f, -1.0f}, {0.95f, 0.20f, 0.24f, 1.0f});
    AddQuad(mesh, {-1.0f, -1.0f, 1.0f}, {1.0f, -1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {-1.0f, 1.0f, 1.0f}, {0.10f, 0.62f, 1.00f, 1.0f});
    AddQuad(mesh, {-1.0f, 1.0f, -1.0f}, {-1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, -1.0f}, {0.20f, 0.86f, 0.36f, 1.0f});
    AddQuad(mesh, {-1.0f, -1.0f, -1.0f}, {1.0f, -1.0f, -1.0f}, {1.0f, -1.0f, 1.0f}, {-1.0f, -1.0f, 1.0f}, {1.00f, 0.76f, 0.20f, 1.0f});
    AddQuad(mesh, {-1.0f, -1.0f, -1.0f}, {-1.0f, -1.0f, 1.0f}, {-1.0f, 1.0f, 1.0f}, {-1.0f, 1.0f, -1.0f}, {0.75f, 0.38f, 1.00f, 1.0f});
    AddQuad(mesh, {1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, -1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, -1.0f, 1.0f}, {0.98f, 0.45f, 0.18f, 1.0f});
    return mesh;
}

MeshData CreatePyramidMesh()
{
    MeshData mesh{};
    const XMFLOAT3 apex{0.0f, 1.2f, 0.0f};
    const XMFLOAT3 a{-1.1f, -1.0f, -1.1f};
    const XMFLOAT3 b{1.1f, -1.0f, -1.1f};
    const XMFLOAT3 c{1.1f, -1.0f, 1.1f};
    const XMFLOAT3 d{-1.1f, -1.0f, 1.1f};
    AddQuad(mesh, a, b, c, d, {0.30f, 0.32f, 0.36f, 1.0f});
    AddTriangle(mesh, a, apex, b, {1.00f, 0.45f, 0.22f, 1.0f});
    AddTriangle(mesh, b, apex, c, {1.00f, 0.74f, 0.20f, 1.0f});
    AddTriangle(mesh, c, apex, d, {0.30f, 0.90f, 0.52f, 1.0f});
    AddTriangle(mesh, d, apex, a, {0.35f, 0.65f, 1.00f, 1.0f});
    return mesh;
}

MeshData CreateDiamondMesh()
{
    MeshData mesh{};
    const XMFLOAT3 top{0.0f, 1.25f, 0.0f};
    const XMFLOAT3 bottom{0.0f, -1.25f, 0.0f};
    const XMFLOAT3 left{-1.0f, 0.0f, 0.0f};
    const XMFLOAT3 right{1.0f, 0.0f, 0.0f};
    const XMFLOAT3 front{0.0f, 0.0f, -1.0f};
    const XMFLOAT3 back{0.0f, 0.0f, 1.0f};
    AddTriangle(mesh, top, left, front, {0.85f, 0.40f, 1.00f, 1.0f});
    AddTriangle(mesh, top, front, right, {0.60f, 0.72f, 1.00f, 1.0f});
    AddTriangle(mesh, top, right, back, {0.25f, 0.95f, 0.95f, 1.0f});
    AddTriangle(mesh, top, back, left, {0.95f, 0.95f, 0.35f, 1.0f});
    AddTriangle(mesh, bottom, front, left, {0.55f, 0.20f, 0.75f, 1.0f});
    AddTriangle(mesh, bottom, right, front, {0.20f, 0.40f, 0.90f, 1.0f});
    AddTriangle(mesh, bottom, back, right, {0.15f, 0.62f, 0.62f, 1.0f});
    AddTriangle(mesh, bottom, left, back, {0.70f, 0.70f, 0.16f, 1.0f});
    return mesh;
}

MeshData CreateGroundMesh()
{
    MeshData mesh{};
    // 棋盘地基由多个小方块拼出，比单色平面更容易判断空间位置。
    constexpr int tileCount = 8;
    constexpr float tileSize = 1.25f;
    constexpr float halfSize = static_cast<float>(tileCount) * tileSize * 0.5f;
    constexpr float thickness = 0.12f;

    for (int z = 0; z < tileCount; ++z)
    {
        for (int x = 0; x < tileCount; ++x)
        {
            const float left = -halfSize + static_cast<float>(x) * tileSize;
            const float right = left + tileSize;
            const float front = -halfSize + static_cast<float>(z) * tileSize;
            const float back = front + tileSize;
            const bool evenTile = ((x + z) % 2) == 0;
            const XMFLOAT4 color = evenTile
                ? XMFLOAT4{0.48f, 0.58f, 0.62f, 1.0f}
                : XMFLOAT4{0.22f, 0.30f, 0.34f, 1.0f};
            AddQuad(mesh, {left, 0.0f, front}, {left, 0.0f, back}, {right, 0.0f, back}, {right, 0.0f, front}, color);
        }
    }

    const XMFLOAT4 sideColor{0.12f, 0.16f, 0.18f, 1.0f};
    const XMFLOAT4 edgeColor{0.78f, 0.86f, 0.88f, 1.0f};
    AddQuad(mesh, {-halfSize, -thickness, -halfSize}, {-halfSize, 0.0f, -halfSize}, {halfSize, 0.0f, -halfSize}, {halfSize, -thickness, -halfSize}, edgeColor);
    AddQuad(mesh, {halfSize, -thickness, halfSize}, {halfSize, 0.0f, halfSize}, {-halfSize, 0.0f, halfSize}, {-halfSize, -thickness, halfSize}, edgeColor);
    AddQuad(mesh, {-halfSize, -thickness, halfSize}, {-halfSize, 0.0f, halfSize}, {-halfSize, 0.0f, -halfSize}, {-halfSize, -thickness, -halfSize}, sideColor);
    AddQuad(mesh, {halfSize, -thickness, -halfSize}, {halfSize, 0.0f, -halfSize}, {halfSize, 0.0f, halfSize}, {halfSize, -thickness, halfSize}, sideColor);
    return mesh;
}

MeshData CreateAimWallMesh()
{
    MeshData mesh{};
    AddQuad(mesh, {-AimWallHalfWidth, AimWallBottom, 0.0f}, {-AimWallHalfWidth, AimWallTop, 0.0f}, {AimWallHalfWidth, AimWallTop, 0.0f}, {AimWallHalfWidth, AimWallBottom, 0.0f}, {0.36f, 0.38f, 0.42f, 1.0f});
    constexpr float border = 0.08f;
    AddQuad(mesh, {-AimWallHalfWidth, AimWallBottom - border, -0.01f}, {AimWallHalfWidth, AimWallBottom - border, -0.01f}, {AimWallHalfWidth, AimWallBottom, -0.01f}, {-AimWallHalfWidth, AimWallBottom, -0.01f}, {0.82f, 0.86f, 0.88f, 1.0f});
    AddQuad(mesh, {-AimWallHalfWidth, AimWallTop, -0.01f}, {AimWallHalfWidth, AimWallTop, -0.01f}, {AimWallHalfWidth, AimWallTop + border, -0.01f}, {-AimWallHalfWidth, AimWallTop + border, -0.01f}, {0.82f, 0.86f, 0.88f, 1.0f});
    AddQuad(mesh, {-AimWallHalfWidth - border, AimWallBottom, -0.01f}, {-AimWallHalfWidth, AimWallBottom, -0.01f}, {-AimWallHalfWidth, AimWallTop, -0.01f}, {-AimWallHalfWidth - border, AimWallTop, -0.01f}, {0.82f, 0.86f, 0.88f, 1.0f});
    AddQuad(mesh, {AimWallHalfWidth, AimWallBottom, -0.01f}, {AimWallHalfWidth + border, AimWallBottom, -0.01f}, {AimWallHalfWidth + border, AimWallTop, -0.01f}, {AimWallHalfWidth, AimWallTop, -0.01f}, {0.82f, 0.86f, 0.88f, 1.0f});
    return mesh;
}

MeshData CreateSphereMesh()
{
    MeshData mesh{};
    // 训练目标使用低面数 UV 球，足够圆且顶点数量可控。
    constexpr int slices = 16;
    constexpr int stacks = 10;
    const XMFLOAT4 color{1.0f, 1.0f, 1.0f, 1.0f};

    for (int stack = 0; stack <= stacks; ++stack)
    {
        const float v = static_cast<float>(stack) / static_cast<float>(stacks);
        const float phi = v * Pi;
        const float y = std::cos(phi);
        const float ringRadius = std::sin(phi);

        for (int slice = 0; slice <= slices; ++slice)
        {
            const float u = static_cast<float>(slice) / static_cast<float>(slices);
            const float theta = u * Pi * 2.0f;
            const XMFLOAT3 position{ringRadius * std::cos(theta), y, ringRadius * std::sin(theta)};
            mesh.vertices.push_back(Vertex{position, color, position});
        }
    }

    for (int stack = 0; stack < stacks; ++stack)
    {
        for (int slice = 0; slice < slices; ++slice)
        {
            const std::uint16_t a = static_cast<std::uint16_t>(stack * (slices + 1) + slice);
            const std::uint16_t b = static_cast<std::uint16_t>(a + slices + 1);
            const std::uint16_t c = static_cast<std::uint16_t>(b + 1);
            const std::uint16_t d = static_cast<std::uint16_t>(a + 1);
            mesh.indices.push_back(a);
            mesh.indices.push_back(b);
            mesh.indices.push_back(c);
            mesh.indices.push_back(a);
            mesh.indices.push_back(c);
            mesh.indices.push_back(d);
        }
    }

    return mesh;
}

MeshData CreateCylinderMesh()
{
    MeshData mesh{};
    constexpr int slices = 18;
    constexpr float halfHeight = 1.0f;
    const XMFLOAT4 color{1.0f, 1.0f, 1.0f, 1.0f};

    for (int slice = 0; slice <= slices; ++slice)
    {
        const float u = static_cast<float>(slice) / static_cast<float>(slices);
        const float theta = u * Pi * 2.0f;
        const float x = std::cos(theta);
        const float z = std::sin(theta);
        const XMFLOAT3 normal{x, 0.0f, z};
        mesh.vertices.push_back(Vertex{{x, -halfHeight, z}, color, normal});
        mesh.vertices.push_back(Vertex{{x, halfHeight, z}, color, normal});
    }

    for (int slice = 0; slice < slices; ++slice)
    {
        const std::uint16_t a = static_cast<std::uint16_t>(slice * 2);
        const std::uint16_t b = static_cast<std::uint16_t>(a + 1);
        const std::uint16_t c = static_cast<std::uint16_t>(a + 3);
        const std::uint16_t d = static_cast<std::uint16_t>(a + 2);
        mesh.indices.push_back(a);
        mesh.indices.push_back(b);
        mesh.indices.push_back(c);
        mesh.indices.push_back(a);
        mesh.indices.push_back(c);
        mesh.indices.push_back(d);
    }

    const std::uint16_t bottomCenter = static_cast<std::uint16_t>(mesh.vertices.size());
    mesh.vertices.push_back(Vertex{{0.0f, -halfHeight, 0.0f}, color, {0.0f, -1.0f, 0.0f}});
    const std::uint16_t topCenter = static_cast<std::uint16_t>(mesh.vertices.size());
    mesh.vertices.push_back(Vertex{{0.0f, halfHeight, 0.0f}, color, {0.0f, 1.0f, 0.0f}});

    for (int slice = 0; slice < slices; ++slice)
    {
        const std::uint16_t current = static_cast<std::uint16_t>(slice * 2);
        const std::uint16_t next = static_cast<std::uint16_t>(((slice + 1) % slices) * 2);
        mesh.indices.push_back(bottomCenter);
        mesh.indices.push_back(next);
        mesh.indices.push_back(current);
        mesh.indices.push_back(topCenter);
        mesh.indices.push_back(static_cast<std::uint16_t>(current + 1));
        mesh.indices.push_back(static_cast<std::uint16_t>(next + 1));
    }

    return mesh;
}

MeshData CreateCrosshairDotMesh()
{
    MeshData mesh{};
    // 准心是贴在相机前方的实心圆盘，命中判定不使用这个半径。
    constexpr int segments = 48;
    const XMFLOAT4 color{1.0f, 1.0f, 1.0f, 1.0f};
    const XMFLOAT3 normal{0.0f, 0.0f, -1.0f};

    mesh.vertices.push_back(Vertex{{0.0f, 0.0f, 0.0f}, color, normal});

    for (int index = 0; index < segments; ++index)
    {
        const float t = static_cast<float>(index) / static_cast<float>(segments);
        const float angle = t * Pi * 2.0f;
        const float x = std::cos(angle);
        const float y = std::sin(angle);
        mesh.vertices.push_back(Vertex{{x, y, 0.0f}, color, normal});
    }

    for (int index = 0; index < segments; ++index)
    {
        const std::uint16_t current = static_cast<std::uint16_t>(index + 1);
        const std::uint16_t next = static_cast<std::uint16_t>((index + 1) % segments + 1);
        mesh.indices.push_back(0);
        mesh.indices.push_back(next);
        mesh.indices.push_back(current);
    }

    return mesh;
}

MeshData CreatePrismMesh()
{
    MeshData mesh{};
    const XMFLOAT3 a{-1.0f, -0.9f, -0.9f};
    const XMFLOAT3 b{1.0f, -0.9f, -0.9f};
    const XMFLOAT3 c{0.0f, 1.0f, -0.9f};
    const XMFLOAT3 d{-1.0f, -0.9f, 0.9f};
    const XMFLOAT3 e{1.0f, -0.9f, 0.9f};
    const XMFLOAT3 f{0.0f, 1.0f, 0.9f};
    AddTriangle(mesh, a, c, b, {0.94f, 0.28f, 0.30f, 1.0f});
    AddTriangle(mesh, d, e, f, {0.20f, 0.70f, 1.00f, 1.0f});
    AddQuad(mesh, a, d, f, c, {0.35f, 0.90f, 0.50f, 1.0f});
    AddQuad(mesh, b, c, f, e, {1.00f, 0.78f, 0.25f, 1.0f});
    AddQuad(mesh, a, b, e, d, {0.65f, 0.45f, 1.00f, 1.0f});
    return mesh;
}

MeshData CreateLightRangeMesh()
{
    MeshData mesh{};
    // 光照范围只作为编辑辅助线框，渲染时使用 LINELIST。
    const XMFLOAT4 color{1.0f, 0.88f, 0.18f, 1.0f};
    const XMFLOAT3 apex{0.0f, 0.0f, 0.0f};

    auto addSquare = [&](float z, float halfSize) {
        const XMFLOAT3 a{-halfSize, -halfSize, z};
        const XMFLOAT3 b{-halfSize, halfSize, z};
        const XMFLOAT3 c{halfSize, halfSize, z};
        const XMFLOAT3 d{halfSize, -halfSize, z};
        AddLine(mesh, a, b, color);
        AddLine(mesh, b, c, color);
        AddLine(mesh, c, d, color);
        AddLine(mesh, d, a, color);
    };

    addSquare(0.35f, 0.35f);
    addSquare(0.70f, 0.70f);
    addSquare(1.00f, 1.00f);

    AddLine(mesh, apex, {-1.0f, -1.0f, 1.0f}, color);
    AddLine(mesh, apex, {-1.0f, 1.0f, 1.0f}, color);
    AddLine(mesh, apex, {1.0f, 1.0f, 1.0f}, color);
    AddLine(mesh, apex, {1.0f, -1.0f, 1.0f}, color);
    AddLine(mesh, {-0.18f, 0.0f, 0.0f}, {0.18f, 0.0f, 0.0f}, color);
    AddLine(mesh, {0.0f, -0.18f, 0.0f}, {0.0f, 0.18f, 0.0f}, color);

    return mesh;
}
