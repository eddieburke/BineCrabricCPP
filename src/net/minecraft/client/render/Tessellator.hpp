#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "net/minecraft/client/gl/GL11.hpp"

namespace net::minecraft::client::render {

struct TessellatorVertex {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    double u = 0.0;
    double v = 0.0;
    std::uint32_t color = 0xFFFFFFFFU;
    std::int32_t normal = 0;
};

class Tessellator {
public:
    static Tessellator INSTANCE;

    explicit Tessellator(std::size_t bufferSize = 2'097'152)
        : bufferSize_(bufferSize)
    {
        vertices_.reserve(bufferSize_);
    }

    void startQuads()
    {
        start(kGlQuads);
    }

    void start(int mode)
    {
        if (drawing_) {
            draw();
        }
        drawing_ = true;
        mode_ = mode;
        hasTexture_ = false;
        hasColor_ = false;
        hasNormals_ = false;
        colorDisabled_ = false;
        reset();
    }

    void texture(double u, double v)
    {
        hasTexture_ = true;
        u_ = u;
        v_ = v;
    }

    void color(float r, float g, float b)
    {
        color(static_cast<int>(std::clamp(r, 0.0f, 1.0f) * 255.0f),
            static_cast<int>(std::clamp(g, 0.0f, 1.0f) * 255.0f),
            static_cast<int>(std::clamp(b, 0.0f, 1.0f) * 255.0f));
    }

    void color(float r, float g, float b, float a)
    {
        color(static_cast<int>(std::clamp(r, 0.0f, 1.0f) * 255.0f),
            static_cast<int>(std::clamp(g, 0.0f, 1.0f) * 255.0f),
            static_cast<int>(std::clamp(b, 0.0f, 1.0f) * 255.0f),
            static_cast<int>(std::clamp(a, 0.0f, 1.0f) * 255.0f));
    }

    void color(int r, int g, int b)
    {
        color(r, g, b, 255);
    }

    void color(int r, int g, int b, int a)
    {
        if (colorDisabled_) {
            return;
        }

        r = std::clamp(r, 0, 255);
        g = std::clamp(g, 0, 255);
        b = std::clamp(b, 0, 255);
        a = std::clamp(a, 0, 255);

        hasColor_ = true;
        currentColor_ = (static_cast<std::uint32_t>(a) << 24U)
            | (static_cast<std::uint32_t>(b) << 16U)
            | (static_cast<std::uint32_t>(g) << 8U)
            | static_cast<std::uint32_t>(r);
    }

    void color(int rgb)
    {
        color((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
    }

    void color(int rgb, int a)
    {
        color((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF, a);
    }

    void disableColor()
    {
        colorDisabled_ = true;
    }

    void normal(float x, float y, float z)
    {
        hasNormals_ = true;
        const auto nx = static_cast<std::int8_t>(std::clamp(x, -1.0f, 1.0f) * 127.0f);
        const auto ny = static_cast<std::int8_t>(std::clamp(y, -1.0f, 1.0f) * 127.0f);
        const auto nz = static_cast<std::int8_t>(std::clamp(z, -1.0f, 1.0f) * 127.0f);
        currentNormal_ = static_cast<std::int32_t>(static_cast<std::uint8_t>(nx))
            | (static_cast<std::int32_t>(static_cast<std::uint8_t>(ny)) << 8U)
            | (static_cast<std::int32_t>(static_cast<std::uint8_t>(nz)) << 16U);
    }

    void translate(double x, double y, double z)
    {
        xOffset_ = x;
        yOffset_ = y;
        zOffset_ = z;
    }

    void translate(float x, float y, float z)
    {
        xOffset_ += static_cast<double>(x);
        yOffset_ += static_cast<double>(y);
        zOffset_ += static_cast<double>(z);
    }

    void vertex(double x, double y, double z, double u, double v)
    {
        texture(u, v);
        vertex(x, y, z);
    }

    void vertex(double x, double y, double z)
    {
        if (!drawing_) {
            return;
        }

        ++addedVertexCount_;
        TessellatorVertex vertex;
        vertex.x = x + xOffset_;
        vertex.y = y + yOffset_;
        vertex.z = z + zOffset_;
        vertex.u = hasTexture_ ? u_ : 0.0;
        vertex.v = hasTexture_ ? v_ : 0.0;
        vertex.color = hasColor_ ? currentColor_ : 0xFFFFFFFFU;
        vertex.normal = hasNormals_ ? currentNormal_ : 0;

        if (mode_ == kGlQuads && addedVertexCount_ % 4 == 0 && vertices_.size() >= 3) {
            const std::size_t quadStart = vertices_.size() - 3;
            const TessellatorVertex first = vertices_[quadStart];
            const TessellatorVertex third = vertices_[quadStart + 2];
            pushVertex(first);
            pushVertex(third);
        }
        pushVertex(vertex);

        if (vertexCount_ % 4 == 0 && bufferPosition_ >= static_cast<int>(bufferSize_) - 32) {
            flush();
        }
    }

    void draw()
    {
        if (!drawing_) {
            return;
        }

        drawing_ = false;
        if (!vertices_.empty()) {
            const int drawMode = mode_ == kGlQuads ? kGlTriangles : mode_;
            gl::GL11::glBegin(drawMode);
            for (const TessellatorVertex& vtx : vertices_) {
                if (hasColor_) {
                    const std::uint32_t c = vtx.color;
                    const auto r = static_cast<std::uint8_t>(c & 0xFFU);
                    const auto g = static_cast<std::uint8_t>((c >> 8U) & 0xFFU);
                    const auto b = static_cast<std::uint8_t>((c >> 16U) & 0xFFU);
                    const auto a = static_cast<std::uint8_t>((c >> 24U) & 0xFFU);
                    gl::GL11::glColor4ub(r, g, b, a);
                }
                if (hasNormals_) {
                    const std::int32_t n = vtx.normal;
                    const auto nx = static_cast<std::int8_t>(n & 0xFF);
                    const auto ny = static_cast<std::int8_t>((n >> 8) & 0xFF);
                    const auto nz = static_cast<std::int8_t>((n >> 16) & 0xFF);
                    gl::GL11::glNormal3b(nx, ny, nz);
                }
                if (hasTexture_) {
                    gl::GL11::glTexCoord2d(vtx.u, vtx.v);
                }
                gl::GL11::glVertex3d(vtx.x, vtx.y, vtx.z);
            }
            gl::GL11::glEnd();
        }

        lastDrawnVertices_ = vertices_;
        reset();
    }

    // Finalize CPU vertex buffer without issuing immediate-mode GL (used for VBO upload).
    void finishWithoutDraw()
    {
        if (!drawing_) {
            return;
        }
        drawing_ = false;
        lastDrawnVertices_ = vertices_;
        reset();
    }

    [[nodiscard]] bool drawing() const noexcept
    {
        return drawing_;
    }

    [[nodiscard]] bool hasTexture() const noexcept { return hasTexture_; }
    [[nodiscard]] bool hasColor() const noexcept { return hasColor_; }
    [[nodiscard]] bool hasNormals() const noexcept { return hasNormals_; }

    [[nodiscard]] int mode() const noexcept
    {
        return mode_;
    }

    [[nodiscard]] const std::vector<TessellatorVertex>& vertices() const noexcept
    {
        return drawing_ ? vertices_ : lastDrawnVertices_;
    }

private:
    static constexpr int kGlTriangles = 4;
    static constexpr int kGlQuads = 7;
    static constexpr int kVertexStride = 8;

    void pushVertex(const TessellatorVertex& vertex)
    {
        vertices_.push_back(vertex);
        ++vertexCount_;
        bufferPosition_ += kVertexStride;
    }

    void flush()
    {
        const bool wasDrawing = drawing_;
        draw();
        drawing_ = wasDrawing;
    }

    void reset()
    {
        vertexCount_ = 0;
        bufferPosition_ = 0;
        addedVertexCount_ = 0;
        vertices_.clear();
    }

    std::size_t bufferSize_ = 0;
    bool drawing_ = false;
    bool hasTexture_ = false;
    bool hasColor_ = false;
    bool hasNormals_ = false;
    bool colorDisabled_ = false;
    int mode_ = 7;
    double u_ = 0.0;
    double v_ = 0.0;
    double xOffset_ = 0.0;
    double yOffset_ = 0.0;
    double zOffset_ = 0.0;
    std::uint32_t currentColor_ = 0xFFFFFFFFU;
    std::int32_t currentNormal_ = 0;
    int vertexCount_ = 0;
    int bufferPosition_ = 0;
    int addedVertexCount_ = 0;
    std::vector<TessellatorVertex> vertices_;
    std::vector<TessellatorVertex> lastDrawnVertices_;
};

inline Tessellator Tessellator::INSTANCE{};

// Java Tessellator.instance — namespace alias used by older port call sites.
inline Tessellator& INSTANCE = Tessellator::INSTANCE;

} // namespace net::minecraft::client::render
