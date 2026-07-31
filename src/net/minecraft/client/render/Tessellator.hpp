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
 std::int32_t midBlock = 0;
 std::int32_t light = 0x00F000F0;
 std::int16_t entity[4]{};
 float midU = 0.0f;
 float midV = 0.0f;
 std::int16_t tangent[4]{};
};
static_assert(sizeof(TessellatorVertex) == 60);
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
 void light(int blockLight, int skyLight);
 void disableColor();
 void normal(float x, float y, float z);
 void blockData(double x,
                double y,
                double z,
                int emission,
                int blockLight = 15,
                int skyLight = 15,
                int blockId = 0,
                int renderType = 0,
                int metadata = 0);
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
 void finishQuad();
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
 // Keep offsets in double so absolute world coords + large camera offsets do not
 // lose sub-block precision before the final float vertex write.
 double xOffset_ = 0.0;
 double yOffset_ = 0.0;
 double zOffset_ = 0.0;
 std::uint32_t currentColor_ = 0xFFFFFFFFU;
 std::int32_t currentNormal_ = 0;
 double blockCenterX_ = 0.0;
 double blockCenterY_ = 0.0;
 double blockCenterZ_ = 0.0;
 int blockEmission_ = 0;
 int blockLight_ = 15;
 int skyLight_ = 15;
 int blockId_ = 0;
 int blockRenderType_ = 0;
 int blockMetadata_ = 0;
 bool hasBlockData_ = false;
};
extern Tessellator& INSTANCE;
} // namespace net::minecraft::client::render
