#include "net/minecraft/client/render/Tessellator.hpp"

#include <algorithm>

#include "net/minecraft/client/gl/GL11.hpp"

namespace net::minecraft::client::render {

Tessellator Tessellator::INSTANCE {};
Tessellator& INSTANCE = Tessellator::INSTANCE;

Tessellator::Tessellator(const std::size_t bufferSize)
{
    // bufferSize mirrors Java's int-buffer length (8 ints per vertex). It only
    // sets the mid-build flush threshold; reserve a modest slice up front and
    // let the vector grow on demand.
    flushThreshold_ = std::max<std::size_t>(bufferSize / 8, 256);
    vertices_.reserve(std::min<std::size_t>(flushThreshold_, 4096));
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
    u_ = static_cast<float>(u);
    v_ = static_cast<float>(v);
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
    xOffset_ = static_cast<float>(x);
    yOffset_ = static_cast<float>(y);
    zOffset_ = static_cast<float>(z);
}

void Tessellator::translate(const float x, const float y, const float z)
{
    xOffset_ += x;
    yOffset_ += y;
    zOffset_ += z;
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
    // When hasColor_ is false, drawMesh leaves GL_COLOR_ARRAY disabled, so the
    // externally-set glColor4f (inventory icons, font glyphs) shows through.
    // Quads are stored verbatim (no 4->6 triangle expansion); drawMesh issues
    // GL_QUADS directly.
    TessellatorVertex vertex;
    vertex.x = static_cast<float>(x) + xOffset_;
    vertex.y = static_cast<float>(y) + yOffset_;
    vertex.z = static_cast<float>(z) + zOffset_;
    vertex.u = hasTexture_ ? u_ : 0.0f;
    vertex.v = hasTexture_ ? v_ : 0.0f;
    vertex.color = hasColor_ ? currentColor_ : 0xFFFFFFFFU;
    vertex.normal = hasNormals_ ? currentNormal_ : 0;
    pushVertex(vertex);

    // Flush on a quad/primitive boundary so we never split geometry mid-quad.
    if (vertices_.size() >= flushThreshold_ && vertices_.size() % 4 == 0) {
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
        const TessellatorMesh mesh {std::move(vertices_), mode_, hasTexture_, hasColor_, hasNormals_};
        drawMesh(mesh);
    }
    reset();
}

void Tessellator::drawMesh(const TessellatorMesh& mesh)
{
    if (mesh.vertices.empty()) {
        return;
    }

    const auto* base = mesh.vertices.data();
    const int stride = static_cast<int>(sizeof(TessellatorVertex));

    gl::GL11::glEnableClientState(gl::GL11::GL_VERTEX_ARRAY);
    gl::GL11::glVertexPointer(3, gl::GL11::GL_FLOAT, stride, &base->x);

    if (mesh.hasTexture) {
        gl::GL11::glEnableClientState(gl::GL11::GL_TEXTURE_COORD_ARRAY);
        gl::GL11::glTexCoordPointer(2, gl::GL11::GL_FLOAT, stride, &base->u);
    }
    if (mesh.hasColor) {
        gl::GL11::glEnableClientState(gl::GL11::GL_COLOR_ARRAY);
        gl::GL11::glColorPointer(4, gl::GL11::GL_UNSIGNED_BYTE, stride, &base->color);
    }
    if (mesh.hasNormals) {
        gl::GL11::glEnableClientState(gl::GL11::GL_NORMAL_ARRAY);
        gl::GL11::glNormalPointer(gl::GL11::GL_BYTE, stride, &base->normal);
    }

    gl::GL11::glDrawArrays(mesh.mode, 0, static_cast<int>(mesh.vertices.size()));

    gl::GL11::glDisableClientState(gl::GL11::GL_VERTEX_ARRAY);
    if (mesh.hasTexture) {
        gl::GL11::glDisableClientState(gl::GL11::GL_TEXTURE_COORD_ARRAY);
    }
    if (mesh.hasColor) {
        gl::GL11::glDisableClientState(gl::GL11::GL_COLOR_ARRAY);
    }
    if (mesh.hasNormals) {
        gl::GL11::glDisableClientState(gl::GL11::GL_NORMAL_ARRAY);
    }
}

TessellatorMesh Tessellator::takeMesh()
{
    TessellatorMesh mesh;
    if (drawing_) {
        drawing_ = false;
        mesh.vertices = std::move(vertices_);
        mesh.mode = mode_;
        mesh.hasTexture = hasTexture_;
        mesh.hasColor = hasColor_;
        mesh.hasNormals = hasNormals_;
        vertices_ = {};
    }
    reset();
    return mesh;
}

void Tessellator::pushVertex(const TessellatorVertex& vertex)
{
    vertices_.push_back(vertex);
}

void Tessellator::flush()
{
    if (captureOnly_) {
        // Worker-thread capture: no GL available; let the buffer keep growing.
        return;
    }
    const bool wasDrawing = drawing_;
    draw();
    drawing_ = wasDrawing;
}

void Tessellator::reset()
{
    vertices_.clear();
}

} // namespace net::minecraft::client::render
