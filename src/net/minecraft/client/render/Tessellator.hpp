#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "net/minecraft/client/gl/GL11.hpp"

namespace net::minecraft::client::render {

// Tight, array-uploadable vertex. Positions stay double for pixel-exact parity
// with the Java port; color/normal are packed so the whole struct streams to GL
// as interleaved client arrays (one glDrawArrays per layer, no per-vertex calls).
struct TessellatorVertex {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    double u = 0.0;
    double v = 0.0;
    std::uint32_t color = 0xFFFFFFFFU; // bytes in memory: r,g,b,a
    std::int32_t normal = 0;           // bytes in memory: nx,ny,nz,pad
};

// Finished tessellation output detached from any GL context. Built on any
// thread, replayed into GL on the main thread via Tessellator::drawMesh.
// Quads are stored verbatim (4 verts each) — no triangle expansion.
struct TessellatorMesh {
    std::vector<TessellatorVertex> vertices;
    int mode = 7;
    bool hasTexture = false;
    bool hasColor = false;
    bool hasNormals = false;

    [[nodiscard]] bool empty() const noexcept { return vertices.empty(); }
};

class Tessellator {
public:
    static Tessellator INSTANCE;

    explicit Tessellator(std::size_t bufferSize = 2'097'152);

    void startQuads();
    void start(int mode);

    void texture(double u, double v);

    void color(float r, float g, float b);
    void color(float r, float g, float b, float a);
    void color(int r, int g, int b);
    void color(int r, int g, int b, int a);
    void color(int rgb);
    void color(int rgb, int a);

    void disableColor();

    void normal(float x, float y, float z);

    void translate(double x, double y, double z);
    void translate(float x, float y, float z);

    void vertex(double x, double y, double z, double u, double v);
    void vertex(double x, double y, double z);

    void draw();

    // Finalize and move the buffered geometry out without touching GL.
    // Safe off the GL thread when the tessellator is in capture mode.
    [[nodiscard]] TessellatorMesh takeMesh();

    // Replay a captured mesh through GL client arrays (GL thread only).
    static void drawMesh(const TessellatorMesh& mesh);

    // Capture mode: never flush through GL mid-build; the vertex buffer just
    // grows. Required for tessellators used on worker threads.
    void setCaptureOnly(bool captureOnly) noexcept { captureOnly_ = captureOnly; }

    [[nodiscard]] bool drawing() const noexcept { return drawing_; }

private:
    static constexpr int kGlQuads = 7;

    void pushVertex(const TessellatorVertex& vertex);
    void flush();
    void reset();

    std::size_t flushThreshold_ = 0;
    bool drawing_ = false;
    bool hasTexture_ = false;
    bool hasColor_ = false;
    bool hasNormals_ = false;
    bool colorDisabled_ = false;
    bool captureOnly_ = false;
    int mode_ = 7;
    double u_ = 0.0;
    double v_ = 0.0;
    double xOffset_ = 0.0;
    double yOffset_ = 0.0;
    double zOffset_ = 0.0;
    std::uint32_t currentColor_ = 0xFFFFFFFFU;
    std::int32_t currentNormal_ = 0;
    std::vector<TessellatorVertex> vertices_;
};

// Java Tessellator.instance — namespace alias used by older port call sites.
extern Tessellator& INSTANCE;

} // namespace net::minecraft::client::render
