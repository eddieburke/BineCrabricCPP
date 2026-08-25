#include "net/minecraft/client/render/Tessellator.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <mutex>
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/gl/ShaderProgram.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/render/QuadIndexBuffer.hpp"
#include "net/minecraft/util/concurrent/ThreadNames.hpp"
namespace net::minecraft::client::render {
namespace math = net::minecraft::util::math;
namespace {
bool poseRotates(const math::Matrix4f& pose) noexcept {
 const float* m = pose.data();
 return m[0] != 1.0f || m[5] != 1.0f || m[10] != 1.0f || m[1] != 0.0f || m[2] != 0.0f ||
        m[4] != 0.0f || m[6] != 0.0f || m[8] != 0.0f || m[9] != 0.0f;
}
bool isIdentity(const math::Matrix4f& pose) noexcept {
 return !poseRotates(pose) && pose.data()[12] == 0.0f && pose.data()[13] == 0.0f && pose.data()[14] == 0.0f;
}
void fillUnsetAttribs(TessellatorVertex* vertices, std::size_t count, const math::Matrix4f* pose, bool poseRotates) {
 std::int16_t tangent[3] = {32767, 0, 0};
 if(poseRotates) {
  const float* m = pose->data();
  const float length = std::sqrt(m[0] * m[0] + m[1] * m[1] + m[2] * m[2]);
  if(length > 1.0e-6f) {
   const float inverse = 32767.0f / length;
   tangent[0] = static_cast<std::int16_t>(m[0] * inverse);
   tangent[1] = static_cast<std::int16_t>(m[1] * inverse);
   tangent[2] = static_cast<std::int16_t>(m[2] * inverse);
  }
 }
 for(std::size_t i = 0; i < count; ++i) {
  TessellatorVertex& v = vertices[i];
  if(v.tangent[0] | v.tangent[1] | v.tangent[2] | v.tangent[3]) continue;
  v.midU = v.u;
  v.midV = v.v;
  v.tangent[0] = tangent[0];
  v.tangent[1] = tangent[1];
  v.tangent[2] = tangent[2];
  v.tangent[3] = 32767;
 }
}
} // namespace
namespace {
std::mutex g_vertexPoolMutex;
std::vector<std::vector<TessellatorVertex>> g_vertexPool;
// A section mesh is bounded, so a buffer is always worth keeping; the cap only stops the
// pool growing without bound when uploads outpace captures.
constexpr std::size_t kVertexPoolMax = 256;
} // namespace
std::vector<TessellatorVertex> VertexBufferPool::acquire() {
 const std::lock_guard<std::mutex> guard(g_vertexPoolMutex);
 if(g_vertexPool.empty()) {
  return {};
 }
 std::vector<TessellatorVertex> buffer = std::move(g_vertexPool.back());
 g_vertexPool.pop_back();
 buffer.clear();
 return buffer;
}
void VertexBufferPool::release(std::vector<TessellatorVertex>&& buffer) {
 if(buffer.capacity() == 0) {
  return;
 }
 const std::lock_guard<std::mutex> guard(g_vertexPoolMutex);
 if(g_vertexPool.size() >= kVertexPoolMax) {
  return;
 }
 buffer.clear();
 g_vertexPool.push_back(std::move(buffer));
}
TessellatorMesh::TessellatorMesh(const TessellatorMesh& other)
    : vertices(other.vertices),
      mode(other.mode),
      hasTexture(other.hasTexture),
      hasNormals(other.hasNormals),
      vbo_(0),
      vertexCount_(other.vertices.size()) {
}
TessellatorMesh& TessellatorMesh::operator=(const TessellatorMesh& other) {
 if(this != &other) {
  freeGpuBuffer();
  vertices = other.vertices;
  mode = other.mode;
  hasTexture = other.hasTexture;
  hasNormals = other.hasNormals;
  vbo_ = 0;
  vertexCount_ = vertices.size();
 }
 return *this;
}
TessellatorMesh::TessellatorMesh(TessellatorMesh&& other) noexcept
    : vertices(std::move(other.vertices)),
      mode(other.mode),
      hasTexture(other.hasTexture),
      hasNormals(other.hasNormals),
      vbo_(other.vbo_),
      vertexCount_(other.vertexCount_) {
 other.vbo_ = 0;
 other.vertexCount_ = 0;
}
TessellatorMesh& TessellatorMesh::operator=(TessellatorMesh&& other) noexcept {
 if(this != &other) {
  freeGpuBuffer();
  vertices = std::move(other.vertices);
  mode = other.mode;
  hasTexture = other.hasTexture;
  hasNormals = other.hasNormals;
  vbo_ = other.vbo_;
  vertexCount_ = other.vertexCount_;
  other.vbo_ = 0;
  other.vertexCount_ = 0;
 }
 return *this;
}
TessellatorMesh::~TessellatorMesh() {
 freeGpuBuffer();
}
bool TessellatorMesh::uploadToGpu() {
 if(vertices.empty())
  return false;
 freeGpuBuffer();
 if(gl::GLCore::genBuffers != nullptr) {
  gl::GLCore::genBuffers(1, &vbo_);
  gl::GLCore::bindBuffer(0x8892, vbo_);
  gl::GLCore::bufferData(0x8892, vertices.size() * sizeof(TessellatorVertex), vertices.data(), 0x88E8);
  gl::GLCore::bindBuffer(0x8892, 0);
  vertexCount_ = vertices.size();
  return true;
 }
 return false;
}
void TessellatorMesh::releaseCpuVertices() {
 VertexBufferPool::release(std::move(vertices));
 vertices.clear();
}
void TessellatorMesh::freeGpuBuffer() {
 if(vbo_ != 0) {
  if(gl::GLCore::deleteBuffers != nullptr) {
   gl::GLCore::deleteBuffers(1, &vbo_);
  }
  vbo_ = 0;
 }
}
thread_local Tessellator Tessellator::INSTANCE{};
static int clamp255(float v) {
 return static_cast<int>(std::clamp(v, 0.0f, 1.0f) * 255.0f);
}
Tessellator::Tessellator(std::size_t bufferSize)
    : builder_(bufferSize), isMainThread_(util::concurrent::tl_is_main_thread) {
}
void Tessellator::startQuads() {
 start(kGlQuads);
}
void Tessellator::start(int mode) {
 if(drawing_) {
  if(batchDepth_ > 0) {
   beginPart(mode);
   return;
  }
  draw();
 }
 if(batchDepth_ > 0) {
  beginPart(mode);
  return;
 }
 drawing_ = true;
 mode_ = mode;
 discardedVertexCount_ = 0;
 hasTexture_ = false;
 colorExplicit_ = false;
 constColorPacked_ = core::constColorPacked();
 hasNormals_ = false;
 recalculateNormals_ = captureOnly_ || core::renderStage() != core::RenderStage::None;
 discarding_ = !captureOnly_ && !core::drawEnabled();
 addedVertexCount_ = 0;
 reset();
 if(!captureOnly_) {
  pose_ = core::drawPose();
  poseValid_ = !isIdentity(pose_);
  poseRotates_ = poseValid_ && poseRotates(pose_);
  normalDirty_ = true;
 }
 builder_.begin(effectiveDrawMode(mode));
}
int Tessellator::effectiveDrawMode(int mode) noexcept {
 return (mode == kGlQuads && kTriangleMode) ? 0x0004 : mode; // GL_TRIANGLES
}
void Tessellator::expandQuadToTriangles() {
 auto& buf = builder_.buffer();
 if(buf.size() < 3 * sizeof(TessellatorVertex))
  return;
 std::size_t size = buf.size();
 TessellatorVertex* ptr = reinterpret_cast<TessellatorVertex*>(buf.data());
 std::size_t count = size / sizeof(TessellatorVertex);
 // Copies, not pointers: appending below can reallocate and invalidate ptr.
 const TessellatorVertex v0 = ptr[count - 3];
 const TessellatorVertex v2 = ptr[count - 1];
 // Appended rather than resize()-then-memcpy so the two vertices are not first
 // zero-filled by resize() and then overwritten in full.
 const auto* v0Bytes = reinterpret_cast<const std::uint8_t*>(&v0);
 buf.insert(buf.end(), v0Bytes, v0Bytes + sizeof(v0));
 const auto* v2Bytes = reinterpret_cast<const std::uint8_t*>(&v2);
 buf.insert(buf.end(), v2Bytes, v2Bytes + sizeof(v2));
 builder_.nextVertex();
 builder_.nextVertex();
}
namespace {
// https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/vertices/NormalHelper.java
bool faceNormal(const TessellatorVertex& v0,
                const TessellatorVertex& v1,
                const TessellatorVertex& v2,
                const TessellatorVertex& v3,
                float (&out)[3]) {
 const float d0[3] = {v2.x - v0.x, v2.y - v0.y, v2.z - v0.z};
 const float d1[3] = {v3.x - v1.x, v3.y - v1.y, v3.z - v1.z};
 out[0] = d0[1] * d1[2] - d0[2] * d1[1];
 out[1] = d0[2] * d1[0] - d0[0] * d1[2];
 out[2] = d0[0] * d1[1] - d0[1] * d1[0];
 const float length = std::sqrt(out[0] * out[0] + out[1] * out[1] + out[2] * out[2]);
 if(length <= 1.0e-8f) {
  return false;
 }
 for(float& component : out) component /= length;
 return true;
}
void fillMidTexAndTangent(TessellatorVertex* const* corners,
                          std::size_t cornerCount,
                          TessellatorVertex* write,
                          std::size_t writeCount,
                          bool deriveNormal) {
 float midU = 0.0f, midV = 0.0f;
 for(std::size_t i = 0; i < cornerCount; ++i) {
  midU += corners[i]->u;
  midV += corners[i]->v;
 }
 midU /= static_cast<float>(cornerCount);
 midV /= static_cast<float>(cornerCount);
 const float e1[3] = {corners[1]->x - corners[0]->x, corners[1]->y - corners[0]->y, corners[1]->z - corners[0]->z};
 const float e2[3] = {corners[2]->x - corners[0]->x, corners[2]->y - corners[0]->y, corners[2]->z - corners[0]->z};
 float derived[3]{};
 bool derivedValid = false;
 if(deriveNormal && cornerCount == 4) {
  if(faceNormal(*corners[0], *corners[1], *corners[2], *corners[3], derived)) {
   derivedValid = true;
   const std::int32_t packed =
       static_cast<std::int32_t>(static_cast<std::uint8_t>(static_cast<std::int8_t>(derived[0] * 127.0f))) |
       (static_cast<std::int32_t>(static_cast<std::uint8_t>(static_cast<std::int8_t>(derived[1] * 127.0f)))
        << 8U) |
       (static_cast<std::int32_t>(static_cast<std::uint8_t>(static_cast<std::int8_t>(derived[2] * 127.0f)))
        << 16U);
   for(std::size_t i = 0; i < writeCount; ++i) write[i].normal = packed;
  }
 }
 const float du1 = corners[1]->u - corners[0]->u, dv1 = corners[1]->v - corners[0]->v;
 const float du2 = corners[2]->u - corners[0]->u, dv2 = corners[2]->v - corners[0]->v;
 const float det = du1 * dv2 - du2 * dv1;
 float tangent[3] = {1.0f, 0.0f, 0.0f};
 float handedness = 1.0f;
 if(std::abs(det) > 1.0e-8f) {
  const float inv = 1.0f / det;
  for(int a = 0; a < 3; ++a) tangent[a] = (e1[a] * dv2 - e2[a] * dv1) * inv;
  const float len = std::sqrt(tangent[0] * tangent[0] + tangent[1] * tangent[1] + tangent[2] * tangent[2]);
  if(len > 1.0e-8f)
   for(float& t : tangent) t /= len;
  const float bitangent[3] = {(e2[0] * du1 - e1[0] * du2) * inv, (e2[1] * du1 - e1[1] * du2) * inv,
                              (e2[2] * du1 - e1[2] * du2) * inv};
  const float nx = derivedValid ? derived[0] : static_cast<std::int8_t>(corners[0]->normal & 0xFF) / 127.0f;
  const float ny = derivedValid ? derived[1] : static_cast<std::int8_t>((corners[0]->normal >> 8) & 0xFF) / 127.0f;
  const float nz = derivedValid ? derived[2] : static_cast<std::int8_t>((corners[0]->normal >> 16) & 0xFF) / 127.0f;
  if(nx * nx + ny * ny + nz * nz > 1.0e-8f) {
   // (RenderPearl, prog/lit_deferred.fsh), so a flipped w mirrors the
   const float predictedBitangent[3] = {tangent[1] * nz - tangent[2] * ny,
                                        tangent[2] * nx - tangent[0] * nz,
                                        tangent[0] * ny - tangent[1] * nx};
   if(predictedBitangent[0] * bitangent[0] + predictedBitangent[1] * bitangent[1] +
          predictedBitangent[2] * bitangent[2] <
      0.0f) {
    handedness = -1.0f;
   }
  }
 }
 const auto packTangent = [](float value) {
  const float scaled = std::clamp(value, -1.0f, 1.0f) * 32767.0f;
  return static_cast<std::int16_t>(scaled >= 0.0f ? scaled + 0.5f : scaled - 0.5f);
 };
 const std::int16_t packed[4] = {
     packTangent(tangent[0]),
     packTangent(tangent[1]),
     packTangent(tangent[2]),
     static_cast<std::int16_t>(handedness * 32767.0f)};
 for(std::size_t i = 0; i < writeCount; ++i) {
  write[i].midU = midU;
  write[i].midV = midV;
  std::copy(std::begin(packed), std::end(packed), write[i].tangent);
 }
}
} // namespace
void Tessellator::finishQuad() {
 auto& bytes = builder_.buffer();
 const std::size_t count = bytes.size() / sizeof(TessellatorVertex);
 const std::size_t quadSize = captureOnly_ ? 4 : 6;
 if(count < quadSize) return;
 TessellatorVertex* vertices = reinterpret_cast<TessellatorVertex*>(bytes.data()) + count - quadSize;
 TessellatorVertex* corners[4] = {vertices, vertices + 1, vertices + 2, vertices + (captureOnly_ ? 3 : 5)};
 fillMidTexAndTangent(corners, 4, vertices, quadSize, recalculateNormals_);
 if(recalculateNormals_) {
  hasNormals_ = true;
 }
}
void Tessellator::texture(double u, double v) {
 hasTexture_ = true;
 u_ = static_cast<float>(u);
 v_ = static_cast<float>(v);
}
void Tessellator::color(float r, float g, float b) {
 color(clamp255(r), clamp255(g), clamp255(b));
}
void Tessellator::color(float r, float g, float b, float a) {
 color(clamp255(r), clamp255(g), clamp255(b), clamp255(a));
}
void Tessellator::color(int r, int g, int b) {
 color(r, g, b, 255);
}
void Tessellator::color(int r, int g, int b, int a) {
 colorExplicit_ = true;
 currentColor_ = (static_cast<std::uint32_t>(a & 0xFF) << 24U) | (static_cast<std::uint32_t>(b & 0xFF) << 16U) |
                 (static_cast<std::uint32_t>(g & 0xFF) << 8U) | static_cast<std::uint32_t>(r & 0xFF);
}
void Tessellator::color(int rgb) {
 int a = (rgb >> 24) & 0xFF;
 if(a == 0)
  a = 255;
 color((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF, a);
}
void Tessellator::color(int rgb, int a) {
 color((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF, a);
}
void Tessellator::light(float blockLight, float skyLight) {
 blockLight_ = std::clamp(blockLight, 0.0f, 15.0f);
 skyLight_ = std::clamp(skyLight, 0.0f, 15.0f);
 const std::uint32_t block = static_cast<std::uint32_t>(std::clamp(std::lround(blockLight_ * 16.0f), 0L, 240L));
 const std::uint32_t sky = static_cast<std::uint32_t>(std::clamp(std::lround(skyLight_ * 16.0f), 0L, 240L));
 currentLight_ = static_cast<std::int32_t>(block | (sky << 16U));
}
void Tessellator::normal(float x, float y, float z) {
 hasNormals_ = true;
 currentNormal_ =
     static_cast<std::int32_t>(static_cast<std::uint8_t>(static_cast<std::int8_t>(x * 127.0f))) |
     (static_cast<std::int32_t>(static_cast<std::uint8_t>(static_cast<std::int8_t>(y * 127.0f))) << 8U) |
     (static_cast<std::int32_t>(static_cast<std::uint8_t>(static_cast<std::int8_t>(z * 127.0f))) << 16U);
 normalDirty_ = true;
}
void Tessellator::refreshNormal() {
 // normal() packs signed int8s, so every unpack must sign-extend. The
 // non-rotating path used to read the bytes unsigned, which turned any
 // negative component into ~2.0 instead of -1.0.
 const float nx = static_cast<float>(static_cast<std::int8_t>(currentNormal_ & 0xFF)) / 127.0f;
 const float ny = static_cast<float>(static_cast<std::int8_t>((currentNormal_ >> 8) & 0xFF)) / 127.0f;
 const float nz = static_cast<float>(static_cast<std::int8_t>((currentNormal_ >> 16) & 0xFF)) / 127.0f;
 normalDirty_ = false;
 if(!poseRotates_) {
  drawNormal_[0] = nx;
  drawNormal_[1] = ny;
  drawNormal_[2] = nz;
  return;
 }
 const float* m = pose_.data();
 float rx = m[0] * nx + m[4] * ny + m[8] * nz;
 float ry = m[1] * nx + m[5] * ny + m[9] * nz;
 float rz = m[2] * nx + m[6] * ny + m[10] * nz;
 const float length = std::sqrt(rx * rx + ry * ry + rz * rz);
 if(length > 1.0e-6f) {
  rx /= length;
  ry /= length;
  rz /= length;
 }
 drawNormal_[0] = rx;
 drawNormal_[1] = ry;
 drawNormal_[2] = rz;
}
void Tessellator::blockData(
    double x, double y, double z, int emission, int blockLight, int skyLight, int blockId, bool fluid, int metadata) {
 blockCenterX_ = x + 0.5;
 blockCenterY_ = y + 0.5;
 blockCenterZ_ = z + 0.5;
 blockEmission_ = std::clamp(emission, 0, 15);
 blockLight_ = static_cast<float>(std::clamp(blockLight, 0, 15));
 skyLight_ = static_cast<float>(std::clamp(skyLight, 0, 15));
 currentLight_ = static_cast<std::int32_t>(
     (static_cast<std::uint32_t>(std::clamp(blockLight, 0, 15) * 16) & 0xFFFFU) |
     (static_cast<std::uint32_t>(std::clamp(skyLight, 0, 15) * 16) << 16U));
 blockId_ = blockId;
 blockFluid_ = fluid;
 blockMetadata_ = metadata;
 hasBlockData_ = true;
}
void Tessellator::translate(double x, double y, double z) {
 xOffset_ = x;
 yOffset_ = y;
 zOffset_ = z;
}
void Tessellator::translate(float x, float y, float z) {
 xOffset_ += static_cast<double>(x);
 yOffset_ += static_cast<double>(y);
 zOffset_ += static_cast<double>(z);
}
void Tessellator::vertex(double x, double y, double z, double u, double v) {
 texture(u, v);
 vertex(x, y, z);
}
void Tessellator::vertex(double x, double y, double z) {
 if(!drawing_)
  return;
 if(discarding_) {
  ++discardedVertexCount_;
  return;
 }
 ++addedVertexCount_;
 if(mode_ == kGlQuads && kTriangleMode && !captureOnly_ && addedVertexCount_ % 4 == 0) {
  expandQuadToTriangles();
 }
 float vx = static_cast<float>(x + xOffset_);
 float vy = static_cast<float>(y + yOffset_);
 float vz = static_cast<float>(z + zOffset_);
 if(poseValid_) {
  const float* m = pose_.data();
  const float px = m[0] * vx + m[4] * vy + m[8] * vz + m[12];
  const float py = m[1] * vx + m[5] * vy + m[9] * vz + m[13];
  const float pz = m[2] * vx + m[6] * vy + m[10] * vz + m[14];
  vx = px;
  vy = py;
  vz = pz;
 }
 auto vProxy = builder_.vertex(vx, vy, vz);
 TessellatorVertex* vertex = vProxy.ptr;
 vertex->light = currentLight_;
 if(hasBlockData_) {
  // Three of these per vertex, and std::lround is a libm call GCC will not fold
  // away. Adding the half before truncating is the same round-half-away-from-zero
  // for every value that reaches here, which is a block-local offset in [-2, 2].
  const auto component = [](double value) {
   const double scaled = value * 64.0;
   const long rounded = static_cast<long>(scaled < 0.0 ? scaled - 0.5 : scaled + 0.5);
   return static_cast<std::uint32_t>(
       static_cast<std::uint8_t>(static_cast<std::int8_t>(std::clamp(rounded, -128L, 127L))));
  };
  vertex->midBlock = static_cast<std::int32_t>(
      component(blockCenterX_ - x) | (component(blockCenterY_ - y) << 8U) |
      (component(blockCenterZ_ - z) << 16U) | (static_cast<std::uint32_t>(blockEmission_) << 24U));
  // https://shaders.properties/current/reference/attributes/mc_entity/
  vertex->entity[0] = static_cast<std::int16_t>(blockId_);
  vertex->entity[1] = static_cast<std::int16_t>(blockFluid_ ? 1 : -1);
  vertex->entity[2] = static_cast<std::int16_t>(blockMetadata_);
  vertex->entity[3] = 0;
 } else {
  // https://shaders.properties/current/reference/attributes/mc_entity/
  vertex->entity[0] = -1;
  vertex->entity[1] = -1;
  vertex->entity[2] = 0;
  vertex->entity[3] = 0;
 }
 if(isMainThread_) {
  vertex->irisEntity[0] = core::entityId();
  vertex->irisEntity[1] = core::blockEntityId();
  vertex->irisEntity[2] = core::renderedItemId();
 } else {
  vertex->irisEntity[0] = -1;
  vertex->irisEntity[1] = -1;
  vertex->irisEntity[2] = -1;
 }
 if(hasTexture_)
  vProxy.tex(u_, v_);
 vProxy.color(colorExplicit_ ? currentColor_ : constColorPacked_);
 if(hasNormals_) {
  if(normalDirty_) {
   refreshNormal();
  }
  vProxy.normal(drawNormal_[0], drawNormal_[1], drawNormal_[2]);
 }
 vProxy.next();
 if(mode_ == kGlQuads && addedVertexCount_ % 4 == 0) {
  finishQuad();
 } else if((mode_ == 4 && addedVertexCount_ % 3 == 0) || (mode_ == 5 && addedVertexCount_ >= 3)) {
  auto& bytes = builder_.buffer();
  const std::size_t count = bytes.size() / sizeof(TessellatorVertex);
  if(count >= 3) {
   TessellatorVertex* v = reinterpret_cast<TessellatorVertex*>(bytes.data()) + count - 3;
   TessellatorVertex* corners[3] = {v, v + 1, v + 2};
   fillMidTexAndTangent(corners, 3, v, 3, false);
  }
 }
}
void Tessellator::draw() {
 if(!drawing_)
  return;
 drawing_ = false;
 if(batchDepth_ > 0) {
  return;
 }
 if(discarding_) {
  reset();
  return;
 }
 if(builder_.vertexCount() > 0 && !captureOnly_) {
  auto& bytes = builder_.buffer();
  fillUnsetAttribs(reinterpret_cast<TessellatorVertex*>(bytes.data()),
                   bytes.size() / sizeof(TessellatorVertex),
                   &pose_,
                   poseRotates_);
  render::core::RenderPass pass;
  pass.modelView = render::core::drawModelView();
  pass.projection = render::core::drawProjection();
  render::core::applyPendingTerrain(pass);
  pass.vertexData = builder_.buffer().data();
  pass.vertexCount = builder_.vertexCount();
  pass.stride = static_cast<int>(sizeof(TessellatorVertex));
  pass.glMode = builder_.drawMode();
  pass.hasTexture = hasTexture_;
  pass.hasNormals = hasNormals_;
  render::core::submit(pass);
 }
 reset();
}
TessellatorMesh Tessellator::takeMesh() {
 std::size_t count = builder_.vertexCount();
 // reserve+insert rather than vector(count): the latter value-initialises every
 // vertex and the memcpy then overwrites all of it, so each mesh was written twice.
 std::vector<TessellatorVertex> verts = VertexBufferPool::acquire();
 if(count > 0) {
  const auto* src = reinterpret_cast<const TessellatorVertex*>(builder_.buffer().data());
  verts.reserve(count);
  verts.insert(verts.end(), src, src + count);
  fillUnsetAttribs(verts.data(), count, &pose_, poseRotates_);
 }
 TessellatorMesh mesh(std::move(verts), mode_, hasTexture_, hasNormals_);
 reset();
 return mesh;
}
void Tessellator::drawMesh(const TessellatorMesh& mesh, std::optional<std::uint32_t> colorOverride) {
 if(mesh.empty() || mesh.vbo_ == 0 || !gl::GLCore::vboSupported)
  return;
 int stride = static_cast<int>(sizeof(TessellatorVertex));
 if(mesh.mode == kGlQuads) {
  const std::size_t vertexCount = (mesh.vertexCount() / 4) * 4;
  if(!quad_index::ensure(vertexCount)) return;
  render::core::RenderPass pass;
  pass.modelView = render::core::drawModelView();
  pass.projection = render::core::drawProjection();
  render::core::applyPendingTerrain(pass);
  pass.buffer = mesh.vbo_;
  pass.vertexCount = vertexCount;
  pass.stride = stride;
  pass.hasTexture = mesh.hasTexture;
  pass.hasNormals = mesh.hasNormals;
  pass.overrideColor = colorOverride.has_value();
  pass.colorOverride = colorOverride.value_or(0u);
  render::core::submitIndexedQuads(pass, quad_index::handle(), static_cast<int>((vertexCount / 4) * 6));
  return;
 }
 int mode = effectiveDrawMode(mesh.mode);
 render::core::RenderPass pass;
 pass.modelView = render::core::drawModelView();
 pass.projection = render::core::drawProjection();
 render::core::applyPendingTerrain(pass);
 pass.hasTexture = mesh.hasTexture;
 pass.hasNormals = mesh.hasNormals;
 pass.stride = stride;
 pass.glMode = mode;
 pass.buffer = mesh.vbo_;
 pass.overrideColor = colorOverride.has_value();
 pass.colorOverride = colorOverride.value_or(0u);
 pass.byteOffset = 0;
 pass.vertexCount = mesh.vertexCount();
 render::core::submit(pass);
 gl::GLCore::bindBuffer(0x8892, 0);
}
void Tessellator::reset() {
 builder_.reset();
 xOffset_ = 0.0;
 yOffset_ = 0.0;
 zOffset_ = 0.0;
 hasBlockData_ = false;
 poseValid_ = false;
 poseRotates_ = false;
 normalDirty_ = true;
}
void Tessellator::beginBatch() {
 if(batchDepth_ == 0) {
  if(drawing_) {
   draw();
  }
  discardedVertexCount_ = 0;
  builder_.reset();
 }
 ++batchDepth_;
}
void Tessellator::beginPart(int mode) {
 if(builder_.vertexCount() == 0) {
  builder_.begin(effectiveDrawMode(mode));
 }
 drawing_ = true;
 mode_ = mode;
 hasTexture_ = false;
 colorExplicit_ = false;
 constColorPacked_ = core::constColorPacked();
 hasNormals_ = false;
 recalculateNormals_ = captureOnly_ || core::renderStage() != core::RenderStage::None;
 discarding_ = !captureOnly_ && !core::drawEnabled();
 addedVertexCount_ = 0;
 xOffset_ = 0.0;
 yOffset_ = 0.0;
 zOffset_ = 0.0;
 hasBlockData_ = false;
 if(!captureOnly_) {
  pose_ = core::drawPose();
  poseValid_ = !isIdentity(pose_);
  poseRotates_ = poseValid_ && poseRotates(pose_);
  normalDirty_ = true;
 }
}
void Tessellator::endBatch() {
 if(batchDepth_ > 0) {
  --batchDepth_;
 }
 if(batchDepth_ == 0) {
  drawing_ = true;
  draw();
 }
}
} // namespace net::minecraft::client::render
