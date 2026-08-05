#include "net/minecraft/client/render/Tessellator.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
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
// An identity pose means the producer already emits pass-base-space positions
// (terrain via chunkOffset, particles, the block outline). Skipping the transform
// is worth a branch: it is the hot path for the largest vertex counts.
bool isIdentity(const math::Matrix4f& pose) noexcept {
 return !poseRotates(pose) && pose.data()[12] == 0.0f && pose.data()[13] == 0.0f && pose.data()[14] == 0.0f;
}
// Defaults in for vertices that never got their tangent/mid-tex filled: the
// tangent is rotated by the draw pose when one is active so the fallback
// direction matches the posed frame (the old end()-time bake did the same).
void fillUnsetAttribs(TessellatorVertex* vertices, std::size_t count, const math::Matrix4f* pose, bool poseRotates) {
 for(std::size_t i = 0; i < count; ++i) {
  TessellatorVertex& v = vertices[i];
  if(v.tangent[0] | v.tangent[1] | v.tangent[2] | v.tangent[3]) continue;
  v.midU = v.u;
  v.midV = v.v;
  v.tangent[0] = 32767;
  v.tangent[3] = 32767;
  if(poseRotates) {
   const float* m = pose->data();
   const float tx = m[0] * 1.0f + m[4] * 0.0f + m[8] * 0.0f;
   const float ty = m[1] * 1.0f + m[5] * 0.0f + m[9] * 0.0f;
   const float tz = m[2] * 1.0f + m[6] * 0.0f + m[10] * 0.0f;
   const float length = std::sqrt(tx * tx + ty * ty + tz * tz);
   if(length > 1.0e-6f) {
    v.tangent[0] = static_cast<std::int16_t>(tx / length * 32767.0f);
    v.tangent[1] = static_cast<std::int16_t>(ty / length * 32767.0f);
    v.tangent[2] = static_cast<std::int16_t>(tz / length * 32767.0f);
   }
  }
 }
}
} // namespace
TessellatorMesh::TessellatorMesh(const TessellatorMesh& other)
    : vertices(other.vertices),
      mode(other.mode),
      hasTexture(other.hasTexture),
      hasColor(other.hasColor),
      hasNormals(other.hasNormals),
      vbo_(0) {
}
TessellatorMesh& TessellatorMesh::operator=(const TessellatorMesh& other) {
 if(this != &other) {
  freeGpuBuffer();
  vertices = other.vertices;
  mode = other.mode;
  hasTexture = other.hasTexture;
  hasColor = other.hasColor;
  hasNormals = other.hasNormals;
  vbo_ = 0;
 }
 return *this;
}
TessellatorMesh::TessellatorMesh(TessellatorMesh&& other) noexcept
    : vertices(std::move(other.vertices)),
      mode(other.mode),
      hasTexture(other.hasTexture),
      hasColor(other.hasColor),
      hasNormals(other.hasNormals),
      vbo_(other.vbo_) {
 other.vbo_ = 0;
}
TessellatorMesh& TessellatorMesh::operator=(TessellatorMesh&& other) noexcept {
 if(this != &other) {
  freeGpuBuffer();
  vertices = std::move(other.vertices);
  mode = other.mode;
  hasTexture = other.hasTexture;
  hasColor = other.hasColor;
  hasNormals = other.hasNormals;
  vbo_ = other.vbo_;
  other.vbo_ = 0;
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
  return true;
 }
 return false;
}
void TessellatorMesh::freeGpuBuffer() {
 if(vbo_ != 0) {
  if(gl::GLCore::deleteBuffers != nullptr) {
   gl::GLCore::deleteBuffers(1, &vbo_);
  }
  vbo_ = 0;
 }
}
Tessellator Tessellator::INSTANCE{};
Tessellator& INSTANCE = Tessellator::INSTANCE;
static int clamp255(float v) {
 return static_cast<int>(std::clamp(v, 0.0f, 1.0f) * 255.0f);
}
Tessellator::Tessellator(std::size_t bufferSize) : builder_(bufferSize) {
}
void Tessellator::startQuads() {
 start(kGlQuads);
}
void Tessellator::start(int mode) {
 if(drawing_) {
  draw();
 }
 drawing_ = true;
 mode_ = mode;
 hasTexture_ = false;
 hasColor_ = false;
 hasNormals_ = false;
 colorDisabled_ = false;
 addedVertexCount_ = 0;
 reset();
 // Snapshot the pose for this batch AFTER reset(), which clears the previous
 // batch's snapshot. Mesh captures run with captureOnly and never draw; their
 // vertices stay model-local and the consumer supplies the transform, so the
 // flag keeps them out of this branch.
 if(!captureOnly_) {
  pose_ = core::drawPose();
  poseValid_ = !isIdentity(pose_);
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
 TessellatorVertex v0 = ptr[count - 3];
 TessellatorVertex v2 = ptr[count - 1];
 const auto append = [&](const TessellatorVertex& vertex) {
  auto& bytes = builder_.buffer();
  const std::size_t offset = bytes.size();
  bytes.resize(offset + sizeof(vertex));
  std::memcpy(bytes.data() + offset, &vertex, sizeof(vertex));
  builder_.nextVertex();
 };
 append(v0);
 append(v2);
}
namespace {
void fillMidTexAndTangent(TessellatorVertex* const* corners,
                          std::size_t cornerCount,
                          TessellatorVertex* write,
                          std::size_t writeCount) {
 float midU = 0.0f, midV = 0.0f;
 for(std::size_t i = 0; i < cornerCount; ++i) {
  midU += corners[i]->u;
  midV += corners[i]->v;
 }
 midU /= static_cast<float>(cornerCount);
 midV /= static_cast<float>(cornerCount);
 const float e1[3] = {corners[1]->x - corners[0]->x, corners[1]->y - corners[0]->y, corners[1]->z - corners[0]->z};
 const float e2[3] = {corners[2]->x - corners[0]->x, corners[2]->y - corners[0]->y, corners[2]->z - corners[0]->z};
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
  const float nx = static_cast<std::int8_t>(corners[0]->normal & 0xFF) / 127.0f;
  const float ny = static_cast<std::int8_t>((corners[0]->normal >> 8) & 0xFF) / 127.0f;
  const float nz = static_cast<std::int8_t>((corners[0]->normal >> 16) & 0xFF) / 127.0f;
  if(nx * nx + ny * ny + nz * nz > 1.0e-8f) {
   const float cross[3] = {ny * tangent[2] - nz * tangent[1], nz * tangent[0] - nx * tangent[2],
                           nx * tangent[1] - ny * tangent[0]};
   if(cross[0] * bitangent[0] + cross[1] * bitangent[1] + cross[2] * bitangent[2] < 0.0f) handedness = -1.0f;
  }
 }
 const std::int16_t packed[4] = {
     static_cast<std::int16_t>(std::lround(std::clamp(tangent[0], -1.0f, 1.0f) * 32767.0f)),
     static_cast<std::int16_t>(std::lround(std::clamp(tangent[1], -1.0f, 1.0f) * 32767.0f)),
     static_cast<std::int16_t>(std::lround(std::clamp(tangent[2], -1.0f, 1.0f) * 32767.0f)),
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
 fillMidTexAndTangent(corners, 4, vertices, quadSize);
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
 if(colorDisabled_)
  return;
 hasColor_ = true;
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
}
void Tessellator::disableColor() {
 colorDisabled_ = true;
}
void Tessellator::normal(float x, float y, float z) {
 hasNormals_ = true;
 currentNormal_ =
     static_cast<std::int32_t>(static_cast<std::uint8_t>(static_cast<std::int8_t>(x * 127.0f))) |
     (static_cast<std::int32_t>(static_cast<std::uint8_t>(static_cast<std::int8_t>(y * 127.0f))) << 8U) |
     (static_cast<std::int32_t>(static_cast<std::uint8_t>(static_cast<std::int8_t>(z * 127.0f))) << 16U);
}
void Tessellator::blockData(
    double x, double y, double z, int emission, int blockLight, int skyLight, int blockId, bool fluid, int metadata) {
 blockCenterX_ = x + 0.5;
 blockCenterY_ = y + 0.5;
 blockCenterZ_ = z + 0.5;
 blockEmission_ = std::clamp(emission, 0, 15);
 blockLight_ = static_cast<float>(std::clamp(blockLight, 0, 15));
 skyLight_ = static_cast<float>(std::clamp(skyLight, 0, 15));
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
 ++addedVertexCount_;
 if(mode_ == kGlQuads && kTriangleMode && !captureOnly_ && addedVertexCount_ % 4 == 0) {
  expandQuadToTriangles();
 }
 // Add in double, then cast once — avoids float(world) + float(offset) collapse at far coords.
 float vx = static_cast<float>(x + xOffset_);
 float vy = static_cast<float>(y + yOffset_);
 float vz = static_cast<float>(z + zOffset_);
 // Iris parity: the pose lives in the vertices, not in modelViewMatrix (see
 // setDrawPose). Positions take the full transform; the face normal is rotated
 // here so the packed byte normal is already camera-relative. Tangents need no
 // extra work — finishQuad derives them from the (posed) corners.
 float rotatedNormal[3] = {0.0f, 0.0f, 0.0f};
 bool poseRotates = false;
 if(poseValid_) {
  const float* m = pose_.data();
  const float px = m[0] * vx + m[4] * vy + m[8] * vz + m[12];
  const float py = m[1] * vx + m[5] * vy + m[9] * vz + m[13];
  const float pz = m[2] * vx + m[6] * vy + m[10] * vz + m[14];
  vx = px;
  vy = py;
  vz = pz;
  poseRotates = m[0] != 1.0f || m[5] != 1.0f || m[10] != 1.0f || m[1] != 0.0f || m[2] != 0.0f ||
                m[4] != 0.0f || m[6] != 0.0f || m[8] != 0.0f || m[9] != 0.0f;
  if(poseRotates) {
   const float nx = static_cast<float>(static_cast<std::int8_t>(currentNormal_ & 0xFF)) / 127.0f;
   const float ny = static_cast<float>(static_cast<std::int8_t>((currentNormal_ >> 8) & 0xFF)) / 127.0f;
   const float nz = static_cast<float>(static_cast<std::int8_t>((currentNormal_ >> 16) & 0xFF)) / 127.0f;
   rotatedNormal[0] = m[0] * nx + m[4] * ny + m[8] * nz;
   rotatedNormal[1] = m[1] * nx + m[5] * ny + m[9] * nz;
   rotatedNormal[2] = m[2] * nx + m[6] * ny + m[10] * nz;
   const float length = std::sqrt(rotatedNormal[0] * rotatedNormal[0] + rotatedNormal[1] * rotatedNormal[1] +
                                  rotatedNormal[2] * rotatedNormal[2]);
   if(length > 1.0e-6f) {
    rotatedNormal[0] /= length;
    rotatedNormal[1] /= length;
    rotatedNormal[2] /= length;
   }
  }
 }
 auto vProxy = builder_.vertex(vx, vy, vz);
 TessellatorVertex* vertex = reinterpret_cast<TessellatorVertex*>(
     builder_.buffer().data() + builder_.buffer().size() - sizeof(TessellatorVertex));
 // vaUV2 is an unnormalized ushort2 of level*16 (0..240). Keeping the fraction
 // here is what lets a smooth-lit corner land between two lightmap texels, so
 // the GL_LINEAR filter reproduces the vanilla averaged-luminance value instead
 // of snapping to a whole level.
 const auto packLevel = [](float level) {
  return static_cast<std::uint32_t>(std::clamp(std::lround(level * 16.0f), 0L, 240L));
 };
 vertex->light = static_cast<std::int32_t>(packLevel(blockLight_) | (packLevel(skyLight_) << 16U));
 if(hasBlockData_) {
  const auto component = [](double value) {
   return static_cast<std::uint32_t>(static_cast<std::uint8_t>(
       static_cast<std::int8_t>(std::clamp(std::lround(value * 64.0), -128L, 127L))));
  };
  vertex->midBlock = static_cast<std::int32_t>(
      component(blockCenterX_ - x) | (component(blockCenterY_ - y) << 8U) |
      (component(blockCenterZ_ - z) << 16U) | (static_cast<std::uint32_t>(blockEmission_) << 24U));
  // https://shaders.properties/current/reference/attributes/mc_entity/
  // x = block id; y = 1.0 fluids, -1.0 other blocks.
  vertex->entity[0] = static_cast<std::int16_t>(blockId_);
  vertex->entity[1] = static_cast<std::int16_t>(blockFluid_ ? 1 : -1);
  vertex->entity[2] = static_cast<std::int16_t>(blockMetadata_);
  vertex->entity[3] = 0;
 } else {
  // Non-terrain geometry must not look like fluid (RenderPearl SM_ENTITY wave).
  // https://shaders.properties/current/reference/attributes/mc_entity/
  vertex->entity[0] = -1;
  vertex->entity[1] = -1;
  vertex->entity[2] = 0;
  vertex->entity[3] = 0;
 }
 if(util::concurrent::tl_is_main_thread) {
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
 if(hasColor_)
  vProxy.color(currentColor_);
 if(hasNormals_) {
  const float nx = poseRotates ? rotatedNormal[0] : static_cast<float>(currentNormal_ & 0xFF) / 127.0f;
  const float ny = poseRotates ? rotatedNormal[1] : static_cast<float>((currentNormal_ >> 8) & 0xFF) / 127.0f;
  const float nz = poseRotates ? rotatedNormal[2] : static_cast<float>((currentNormal_ >> 16) & 0xFF) / 127.0f;
  vProxy.normal(nx, ny, nz);
 }
 vProxy.next();
 if(mode_ == kGlQuads && addedVertexCount_ % 4 == 0) {
  finishQuad();
 } else if((mode_ == 4 && addedVertexCount_ % 3 == 0) || (mode_ == 5 && addedVertexCount_ >= 3)) {
  // GL_TRIANGLES / GL_TRIANGLE_STRIP — fill last triangle.
  auto& bytes = builder_.buffer();
  const std::size_t count = bytes.size() / sizeof(TessellatorVertex);
  if(count >= 3) {
   TessellatorVertex* v = reinterpret_cast<TessellatorVertex*>(bytes.data()) + count - 3;
   TessellatorVertex* corners[3] = {v, v + 1, v + 2};
   fillMidTexAndTangent(corners, 3, v, 3);
  }
 }
}
void Tessellator::draw() {
 if(!drawing_)
  return;
 drawing_ = false;
 if(builder_.vertexCount() > 0 && !captureOnly_) {
  auto& bytes = builder_.buffer();
  fillUnsetAttribs(reinterpret_cast<TessellatorVertex*>(bytes.data()),
                   bytes.size() / sizeof(TessellatorVertex),
                   &pose_,
                   poseValid_ && poseRotates(pose_));
  render::core::RenderPass pass;
  pass.modelView = render::core::drawModelView();
  pass.projection = render::core::drawProjection();
  pass.fog = render::core::fog();
  // Apply pending section-local chunkOffset from WorldRenderer.
  render::core::applyPendingTerrain(pass);
  pass.vertexData = builder_.buffer().data();
  pass.vertexCount = builder_.vertexCount();
  pass.stride = static_cast<int>(sizeof(TessellatorVertex));
  pass.glMode = builder_.drawMode();
  pass.hasTexture = hasTexture_;
  pass.hasColor = hasColor_;
  pass.hasNormals = hasNormals_;
  render::core::submit(pass);
 }
 reset();
}
TessellatorMesh Tessellator::takeMesh() {
  std::size_t count = builder_.vertexCount();
  std::vector<TessellatorVertex> verts(count);
  if(count > 0) {
   std::memcpy(verts.data(), builder_.buffer().data(), count * sizeof(TessellatorVertex));
   fillUnsetAttribs(verts.data(), count, &pose_, poseValid_ && poseRotates(pose_));
  }
 TessellatorMesh mesh(std::move(verts), mode_, hasTexture_, hasColor_, hasNormals_);
 reset();
 return mesh;
}
namespace {
// Meshes captured in quad mode keep 4 vertices per quad; draw them through the
// shared quad index buffer instead of expanding to 6 vertices at capture time.
bool drawQuadMeshIndexed(const TessellatorMesh& mesh, unsigned vbo, int stride) {
 const std::size_t vertexCount = (mesh.vertices.size() / 4) * 4;
 if(vertexCount == 0 || vbo == 0 || !gl::GLCore::vboSupported) {
  return false;
 }
 if(!quad_index::ensure(vertexCount) || !render::core::ensureReady()) {
  return false;
 }
 {
  render::core::RenderPass pass;
  pass.modelView = render::core::drawModelView();
  pass.projection = render::core::drawProjection();
  pass.fog = render::core::fog();
  pass.hasTexture = mesh.hasTexture;
  pass.hasColor = mesh.hasColor;
  pass.hasNormals = mesh.hasNormals;
  if(!mesh.hasTexture) {
   render::core::bindWhiteDiffuse();
  }
  // Apply pending section-local chunkOffset from WorldRenderer.
  render::core::applyPendingTerrain(pass);
  // Meshes carry their pose in the vertices (applied at capture time), so the
  // VBO copy is always safe to draw directly.
  render::core::bindAndUploadUniforms(pass);
 }
 if(render::core::program() == nullptr) {
  return false;
 }
 gl::GLCore::bindBuffer(0x8892, vbo);
 render::core::configureAttribs(vbo, 0, stride, mesh.hasTexture, mesh.hasColor, mesh.hasNormals);
 gl::GLCore::bindBuffer(0x8893, quad_index::handle());
 const int mode = render::core::program()->tessellation() ? 0x000E : 0x0004;
 if(mode == 0x000E && gl::GLCore::patchParameteri != nullptr) gl::GLCore::patchParameteri(0x8E72, 3);
 ::glDrawElements(mode, static_cast<int>((vertexCount / 4) * 6), 0x1405, nullptr);
 gl::GLCore::bindBuffer(0x8892, 0);
 return true;
}
void drawQuadMeshExpanded(const TessellatorMesh& mesh, int stride) {
 static thread_local std::vector<TessellatorVertex> scratch;
 const std::size_t quadCount = mesh.vertices.size() / 4;
 scratch.clear();
 scratch.reserve(quadCount * 6);
 for(std::size_t quad = 0; quad < quadCount; ++quad) {
  const TessellatorVertex* v = mesh.vertices.data() + quad * 4;
  scratch.push_back(v[0]);
  scratch.push_back(v[1]);
  scratch.push_back(v[2]);
  scratch.push_back(v[0]);
  scratch.push_back(v[2]);
  scratch.push_back(v[3]);
 }
  render::core::RenderPass pass;
  pass.modelView = render::core::drawModelView();
  pass.projection = render::core::drawProjection();
  pass.fog = render::core::fog();
  // Apply pending section-local chunkOffset from WorldRenderer.
  render::core::applyPendingTerrain(pass);
  pass.vertexData = scratch.data();
  pass.vertexCount = scratch.size();
  pass.stride = stride;
  pass.glMode = 0x0004;
  pass.hasTexture = mesh.hasTexture;
  pass.hasColor = mesh.hasColor;
  pass.hasNormals = mesh.hasNormals;
  render::core::submit(pass);
 }
} // namespace
void Tessellator::drawMesh(const TessellatorMesh& mesh) {
 if(mesh.vertices.empty())
  return;
 int stride = static_cast<int>(sizeof(TessellatorVertex));
 if(mesh.mode == kGlQuads) {
  if(!drawQuadMeshIndexed(mesh, mesh.vbo_, stride)) {
   drawQuadMeshExpanded(mesh, stride);
  }
  return;
 }
 int mode = effectiveDrawMode(mesh.mode);
 render::core::RenderPass pass;
 pass.modelView = render::core::drawModelView();
 pass.projection = render::core::drawProjection();
 pass.fog = render::core::fog();
 // Apply pending section-local chunkOffset from WorldRenderer.
 render::core::applyPendingTerrain(pass);
 pass.hasTexture = mesh.hasTexture;
 pass.hasColor = mesh.hasColor;
 pass.hasNormals = mesh.hasNormals;
 pass.stride = stride;
 pass.glMode = mode;
 if(mesh.vbo_ != 0 && gl::GLCore::vboSupported) {
  gl::GLCore::bindBuffer(0x8892, mesh.vbo_);
  pass.buffer = mesh.vbo_;
  pass.byteOffset = 0;
  pass.vertexCount = mesh.vertices.size();
 } else {
  pass.vertexData = mesh.vertices.data();
  pass.vertexCount = mesh.vertices.size();
 }
 render::core::submit(pass);
 gl::GLCore::bindBuffer(0x8892, 0);
}
void Tessellator::reset() {
 builder_.reset();
 xOffset_ = 0.0;
 yOffset_ = 0.0;
 zOffset_ = 0.0;
 hasBlockData_ = false;
 // Only the local snapshot is dropped; the published pose is the producer's
 // state and outlives this batch, like a PoseStack top feeding repeated
 // VertexConsumer draws.
 // see src/net/minecraft/client/render/RenderCore.cpp setDrawPose
 pose_.identity();
 poseValid_ = false;
}
} // namespace net::minecraft::client::render
