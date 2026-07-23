#include "net/minecraft/client/render/Tessellator.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include "net/minecraft/client/gl/EnginePipeline.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/render/QuadIndexBuffer.hpp"
namespace net::minecraft::client::render {
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
 builder_.vertex(v0.x, v0.y, v0.z)
     .tex(v0.u, v0.v)
     .color(v0.color)
     .normal(static_cast<float>(v0.normal & 0xFF) / 127.0f,
             static_cast<float>((v0.normal >> 8) & 0xFF) / 127.0f,
             static_cast<float>((v0.normal >> 16) & 0xFF) / 127.0f)
     .next();
 builder_.vertex(v2.x, v2.y, v2.z)
     .tex(v2.u, v2.v)
     .color(v2.color)
     .normal(static_cast<float>(v2.normal & 0xFF) / 127.0f,
             static_cast<float>((v2.normal >> 8) & 0xFF) / 127.0f,
             static_cast<float>((v2.normal >> 16) & 0xFF) / 127.0f)
     .next();
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
void Tessellator::translate(double x, double y, double z) {
 xOffset_ = static_cast<float>(x);
 yOffset_ = static_cast<float>(y);
 zOffset_ = static_cast<float>(z);
}
void Tessellator::translate(float x, float y, float z) {
 xOffset_ += x;
 yOffset_ += y;
 zOffset_ += z;
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
 float vx = static_cast<float>(x) + xOffset_;
 float vy = static_cast<float>(y) + yOffset_;
 float vz = static_cast<float>(z) + zOffset_;
 auto vProxy = builder_.vertex(vx, vy, vz);
 if(hasTexture_)
  vProxy.tex(u_, v_);
 if(hasColor_)
  vProxy.color(currentColor_);
 if(hasNormals_) {
  vProxy.normal(static_cast<float>(currentNormal_ & 0xFF) / 127.0f,
                static_cast<float>((currentNormal_ >> 8) & 0xFF) / 127.0f,
                static_cast<float>((currentNormal_ >> 16) & 0xFF) / 127.0f);
 }
 vProxy.next();
}
void Tessellator::draw() {
 if(!drawing_)
  return;
 drawing_ = false;
 if(builder_.vertexCount() > 0 && !captureOnly_) {
  builder_.draw();
 }
 reset();
}
TessellatorMesh Tessellator::takeMesh() {
 std::size_t count = builder_.vertexCount();
 std::vector<TessellatorVertex> verts(count);
 if(count > 0) {
  std::memcpy(verts.data(), builder_.buffer().data(), count * sizeof(TessellatorVertex));
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
 if(!quad_index::ensure(vertexCount) || !gl::engine_pipeline::ensureReady()) {
  return false;
 }
 gl::engine_pipeline::bindAndUploadUniforms();
 if(gl::engine_pipeline::program() == nullptr) {
  return false;
 }
 gl::GLCore::bindBuffer(0x8892, vbo);
 gl::engine_pipeline::configureAttribs(vbo, 0, stride, mesh.hasTexture, mesh.hasColor, mesh.hasNormals);
 gl::GLCore::bindBuffer(0x8893, quad_index::handle());
 ::glDrawElements(0x0004, static_cast<int>((vertexCount / 4) * 6), 0x1405, nullptr);
 gl::engine_pipeline::finishAttribs();
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
 gl::engine_pipeline::drawInterleaved(
     scratch.data(), scratch.size(), stride, 0x0004, mesh.hasTexture, mesh.hasColor, mesh.hasNormals);
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
 if(mesh.vbo_ != 0 && gl::GLCore::vboSupported) {
  gl::GLCore::bindBuffer(0x8892, mesh.vbo_);
  gl::engine_pipeline::drawFromBoundBuffer(
      mesh.vbo_, 0, mesh.vertices.size(), stride, mode, mesh.hasTexture, mesh.hasColor, mesh.hasNormals);
  gl::GLCore::bindBuffer(0x8892, 0);
 } else {
  gl::engine_pipeline::drawInterleaved(
      mesh.vertices.data(), mesh.vertices.size(), stride, mode, mesh.hasTexture, mesh.hasColor, mesh.hasNormals);
 }
}
void Tessellator::reset() {
 builder_.reset();
 xOffset_ = 0.0f;
 yOffset_ = 0.0f;
 zOffset_ = 0.0f;
}
} // namespace net::minecraft::client::render
