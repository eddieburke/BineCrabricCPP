#pragma once
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>
#include "net/minecraft/client/render/BufferBuilder.hpp"
namespace net::minecraft::client::render {
struct TessellatorVertex {
 float x = 0.0f;
 float y = 0.0f;
 float z = 0.0f;
 float u = 0.0f;
 float v = 0.0f;
 std::uint32_t color = 0xFFFFFFFFU; // bytes in memory: r,g,b,a
 std::int32_t normal = 0; // nx, ny, nz
};
static_assert(sizeof(TessellatorVertex) == 28);
struct TessellatorMesh {
 std::vector<TessellatorVertex> vertices;
 int mode = 7;
 bool hasTexture = false;
 bool hasColor = false;
 bool hasNormals = false;
 TessellatorMesh() = default;
 TessellatorMesh(std::vector<TessellatorVertex> v, int m, bool ht, bool hc, bool hn)
     : vertices(std::move(v)), mode(m), hasTexture(ht), hasColor(hc), hasNormals(hn) {
 }
 TessellatorMesh(const TessellatorMesh& other);
 TessellatorMesh& operator=(const TessellatorMesh& other);
 TessellatorMesh(TessellatorMesh&& other) noexcept;
 TessellatorMesh& operator=(TessellatorMesh&& other) noexcept;
 ~TessellatorMesh();
 [[nodiscard]] bool empty() const noexcept {
  return vertices.empty();
 }
 [[nodiscard]] bool uploadToGpu();
 void freeGpuBuffer();
 void setGpuBuffer(unsigned vbo) noexcept {
  vbo_ = vbo;
 }

 private:
 friend class Tessellator;
 unsigned vbo_ = 0;
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
 [[nodiscard]] TessellatorMesh takeMesh();
 static void drawMesh(const TessellatorMesh& mesh);
 [[nodiscard]] static int effectiveDrawMode(int mode) noexcept;
 void setCaptureOnly(bool captureOnly) noexcept {
  captureOnly_ = captureOnly;
 }
 [[nodiscard]] bool drawing() const noexcept {
  return drawing_;
 }

 private:
 static constexpr int kGlQuads = 7;
 static constexpr bool kTriangleMode = true;
 void expandQuadToTriangles();
 void flush();
 void reset();
 BufferBuilder<TessellatorVertex> builder_;
 bool drawing_ = false;
 bool hasTexture_ = false;
 bool hasColor_ = false;
 bool hasNormals_ = false;
 bool colorDisabled_ = false;
 bool captureOnly_ = false;
 int addedVertexCount_ = 0;
 int mode_ = 7;
 float u_ = 0.0f;
 float v_ = 0.0f;
 float xOffset_ = 0.0f;
 float yOffset_ = 0.0f;
 float zOffset_ = 0.0f;
 std::uint32_t currentColor_ = 0xFFFFFFFFU;
 std::int32_t currentNormal_ = 0;
};
extern Tessellator& INSTANCE;
} // namespace net::minecraft::client::render
