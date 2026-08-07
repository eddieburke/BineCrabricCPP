#pragma once
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>
#include "net/minecraft/client/render/BufferBuilder.hpp"
#include "net/minecraft/client/render/VertexAbi.hpp"
#include "net/minecraft/util/math/Matrix4f.hpp"
namespace net::minecraft::client::render {
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

 private:
 friend class Tessellator;
 unsigned vbo_ = 0;
};
class Tessellator {
 public:
 static thread_local Tessellator INSTANCE;
 explicit Tessellator(std::size_t bufferSize = 4096);
 void startQuads();
 void start(int mode);
 void texture(double u, double v);
 void color(float r, float g, float b);
 void color(float r, float g, float b, float a);
 void color(int r, int g, int b);
 void color(int r, int g, int b, int a);
 void color(int rgb);
 void color(int rgb, int a);
 void light(float blockLight, float skyLight);
 void normal(float x, float y, float z);
 void blockData(double x,
                double y,
                double z,
                int emission,
                int blockLight = 15,
                int skyLight = 15,
                int blockId = 0,
                bool fluid = false,
                int metadata = 0);
 void translate(double x, double y, double z);
 void translate(float x, float y, float z);
 void vertex(double x, double y, double z, double u, double v);
 void vertex(double x, double y, double z);
 void draw();
 void beginBatch();
 void endBatch();
 class ScopedBatch {
  public:
  ScopedBatch() {
   Tessellator::INSTANCE.beginBatch();
  }
  ~ScopedBatch() {
   Tessellator::INSTANCE.endBatch();
  }
  ScopedBatch(const ScopedBatch&) = delete;
  ScopedBatch& operator=(const ScopedBatch&) = delete;
 };
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
 void beginPart(int mode);
 int batchDepth_ = 0;
 net::minecraft::util::math::Matrix4f pose_{};
 bool poseValid_ = false;
 BufferBuilder<TessellatorVertex> builder_;
 bool drawing_ = false;
 bool hasTexture_ = false;
 bool hasColor_ = false;
 bool hasNormals_ = false;
 bool captureOnly_ = false;
 int addedVertexCount_ = 0;
 int mode_ = 7;
 float u_ = 0.0f;
 float v_ = 0.0f;
 double xOffset_ = 0.0;
 double yOffset_ = 0.0;
 double zOffset_ = 0.0;
 std::uint32_t currentColor_ = 0xFFFFFFFFU;
 std::int32_t currentNormal_ = 0;
 double blockCenterX_ = 0.0;
 double blockCenterY_ = 0.0;
 double blockCenterZ_ = 0.0;
 int blockEmission_ = 0;
 float blockLight_ = 15.0f;
 float skyLight_ = 15.0f;
 int blockId_ = 0;
 bool blockFluid_ = false;
 int blockMetadata_ = 0;
 bool hasBlockData_ = false;
};
} // namespace net::minecraft::client::render