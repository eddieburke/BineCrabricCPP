#include "net/minecraft/client/render/Tessellator.hpp"

#include <algorithm>

#include "net/minecraft/client/gl/GL11.hpp"

namespace net::minecraft::client::render {

Tessellator Tessellator::INSTANCE {};
Tessellator& INSTANCE = Tessellator::INSTANCE;

Tessellator::Tessellator(const std::size_t bufferSize)
    : bufferSize_(bufferSize)
{
    vertices_.reserve(bufferSize_);
}

void Tessellator::startQuads()
{
    start(kGlQuads);
}

void Tessellator::start(const int mode)
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

void Tessellator::texture(const double u, const double v)
{
    hasTexture_ = true;
    u_ = u;
    v_ = v;
}

void Tessellator::color(const float r, const float g, const float b)
{
    color(static_cast<int>(std::clamp(r, 0.0f, 1.0f) * 255.0f),
        static_cast<int>(std::clamp(g, 0.0f, 1.0f) * 255.0f),
        static_cast<int>(std::clamp(b, 0.0f, 1.0f) * 255.0f));
}

void Tessellator::color(const float r, const float g, const float b, const float a)
{
    color(static_cast<int>(std::clamp(r, 0.0f, 1.0f) * 255.0f),
        static_cast<int>(std::clamp(g, 0.0f, 1.0f) * 255.0f),
        static_cast<int>(std::clamp(b, 0.0f, 1.0f) * 255.0f),
        static_cast<int>(std::clamp(a, 0.0f, 1.0f) * 255.0f));
}

void Tessellator::color(const int r, const int g, const int b)
{
    color(r, g, b, 255);
}

void Tessellator::color(const int r, const int g, const int b, const int a)
{
    if (colorDisabled_) {
        return;
    }

    const int clampedR = std::clamp(r, 0, 255);
    const int clampedG = std::clamp(g, 0, 255);
    const int clampedB = std::clamp(b, 0, 255);
    const int clampedA = std::clamp(a, 0, 255);

    hasColor_ = true;
    currentColor_ = (static_cast<std::uint32_t>(clampedA) << 24U)
        | (static_cast<std::uint32_t>(clampedB) << 16U)
        | (static_cast<std::uint32_t>(clampedG) << 8U)
        | static_cast<std::uint32_t>(clampedR);
}

void Tessellator::color(const int rgb)
{
    color((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
}

void Tessellator::color(const int rgb, const int a)
{
    color((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF, a);
}

void Tessellator::disableColor()
{
    colorDisabled_ = true;
}

void Tessellator::normal(const float x, const float y, const float z)
{
    hasNormals_ = true;
    // Beta scales X by 128 and Y/Z by 127 (asymmetry preserved for byte-exact parity).
    const auto nx = static_cast<std::int8_t>(x * 128.0f);
    const auto ny = static_cast<std::int8_t>(y * 127.0f);
    const auto nz = static_cast<std::int8_t>(z * 127.0f);
    currentNormal_ = static_cast<std::int32_t>(static_cast<std::uint8_t>(nx))
        | (static_cast<std::int32_t>(static_cast<std::uint8_t>(ny)) << 8U)
        | (static_cast<std::int32_t>(static_cast<std::uint8_t>(nz)) << 16U);
}

void Tessellator::translate(const double x, const double y, const double z)
{
    xOffset_ = x;
    yOffset_ = y;
    zOffset_ = z;
}

void Tessellator::translate(const float x, const float y, const float z)
{
    xOffset_ += static_cast<double>(x);
    yOffset_ += static_cast<double>(y);
    zOffset_ += static_cast<double>(z);
}

void Tessellator::vertex(const double x, const double y, const double z, const double u, const double v)
{
    texture(u, v);
    vertex(x, y, z);
}

void Tessellator::vertex(const double x, const double y, const double z)
{
    if (!drawing_) {
        return;
    }

    // Faithful to Beta: vertices carry a color only after an explicit tessellator.color().
    // When hasColor_ is false, draw() emits no glColor, so the externally-set glColor4f
    // (inventory icons, font glyphs) shows through — exactly like Java's "no color array".
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

void Tessellator::draw()
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

void Tessellator::finishWithoutDraw()
{
    if (!drawing_) {
        return;
    }
    drawing_ = false;
    lastDrawnVertices_ = vertices_;
    reset();
}

bool Tessellator::drawing() const noexcept
{
    return drawing_;
}

bool Tessellator::hasTexture() const noexcept
{
    return hasTexture_;
}

bool Tessellator::hasColor() const noexcept
{
    return hasColor_;
}

bool Tessellator::hasNormals() const noexcept
{
    return hasNormals_;
}

int Tessellator::mode() const noexcept
{
    return mode_;
}

const std::vector<TessellatorVertex>& Tessellator::vertices() const noexcept
{
    return drawing_ ? vertices_ : lastDrawnVertices_;
}

void Tessellator::pushVertex(const TessellatorVertex& vertex)
{
    vertices_.push_back(vertex);
    ++vertexCount_;
    bufferPosition_ += kVertexStride;
}

void Tessellator::flush()
{
    const bool wasDrawing = drawing_;
    draw();
    drawing_ = wasDrawing;
}

void Tessellator::reset()
{
    vertexCount_ = 0;
    bufferPosition_ = 0;
    addedVertexCount_ = 0;
    vertices_.clear();
}

} // namespace net::minecraft::client::render
