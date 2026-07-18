#include "net/minecraft/client/gl/EnginePipeline.hpp"
#include <cstdint>
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/GlConstants.hpp"
#include "net/minecraft/client/gl/PipelineState.hpp"
#include "net/minecraft/client/gl/ProgramCache.hpp"
#include "net/minecraft/client/gl/ShaderProgram.hpp"
#include "net/minecraft/client/gl/UniformSync.hpp"
#include "net/minecraft/util/math/MatrixStacks.hpp"
namespace net::minecraft::client::gl {
namespace math = net::minecraft::util::math;
namespace engine_pipeline {
namespace {
constexpr unsigned int kArrayBuffer = 0x8892;  // GL_ARRAY_BUFFER
constexpr unsigned int kStreamDraw = 0x88E0;   // GL_STREAM_DRAW
constexpr unsigned int kFloat = 0x1406;        // GL_FLOAT
constexpr unsigned int kUnsignedByte = 0x1401; // GL_UNSIGNED_BYTE
constexpr unsigned int kByte = 0x1400;         // GL_BYTE
const char* kVersion = "#version 330 core\n";
// Byte offsets within TessellatorVertex (pos, uv, color, normal); stride is 28.
constexpr std::size_t kOffPos = 0;
constexpr std::size_t kOffUV = 12;
constexpr std::size_t kOffColor = 20;
constexpr std::size_t kOffNormal = 24;
ProgramCache g_cache;
ShaderProgram* g_program = nullptr;
bool g_triedProgram = false;
unsigned int g_vao = 0;
unsigned int g_streamVbo = 0;
std::size_t g_streamCapacity = 0;
// Fullscreen-triangle blit resources.
ShaderProgram g_blit;
bool g_triedBlit = false;
unsigned int g_blitVao = 0;
unsigned int g_blitVbo = 0;
const char* kBlitVsh =
    "in vec2 aPos;\n"
    "in vec2 aUV;\n"
    "out vec2 vUV;\n"
    "void main(){ vUV = aUV; gl_Position = vec4(aPos, 0.0, 1.0); }\n";
const char* kBlitFsh =
    "in vec2 vUV;\n"
    "uniform sampler2D uTexture;\n"
    "out vec4 fragColor;\n"
    "void main(){ fragColor = texture(uTexture, vUV); }\n";
const void* bufOffset(std::size_t o) {
  return reinterpret_cast<const void*>(static_cast<std::uintptr_t>(o));
}
void ensureVao() {
  if(g_vao == 0 && GLCore::vaoSupported) {
    GLCore::genVertexArrays(1, &g_vao);
  }
  if(g_streamVbo == 0 && GLCore::genBuffers != nullptr) {
    GLCore::genBuffers(1, &g_streamVbo);
  }
}
void uploadStreaming(const void* data, std::size_t bytes) {
  GLCore::bindBuffer(kArrayBuffer, g_streamVbo);
  if(g_streamCapacity < bytes) {
    GLCore::bufferData(kArrayBuffer, static_cast<intptr_t>(bytes), data, kStreamDraw);
    g_streamCapacity = bytes;
  } else {
    // Orphan then refill to avoid GPU/CPU stalls on the shared stream buffer.
    GLCore::bufferData(kArrayBuffer, static_cast<intptr_t>(g_streamCapacity), nullptr, kStreamDraw);
    if(GLCore::bufferSubData != nullptr) {
      GLCore::bufferSubData(kArrayBuffer, 0, static_cast<intptr_t>(bytes), data);
    } else {
      GLCore::bufferData(kArrayBuffer, static_cast<intptr_t>(bytes), data, kStreamDraw);
    }
  }
}
bool ensureBlitResources() {
  GLCore::ensureLoaded();
  if(!GLCore::shaderSupported || !GLCore::vaoSupported) {
    return false;
  }
  if(!g_triedBlit) {
    g_triedBlit = true;
    g_blit.compile(kBlitVsh, kBlitFsh, kVersion);
    GLCore::genVertexArrays(1, &g_blitVao);
    GLCore::genBuffers(1, &g_blitVbo);
    const float tri[] = {
        -1.0f, -1.0f, 0.0f, 0.0f,
        3.0f, -1.0f, 2.0f, 0.0f,
        -1.0f, 3.0f, 0.0f, 2.0f};
    GLCore::bindBuffer(kArrayBuffer, g_blitVbo);
    GLCore::bufferData(kArrayBuffer, static_cast<intptr_t>(sizeof(tri)), tri, 0x88E4);
    GLCore::bindBuffer(kArrayBuffer, 0);
  }
  return g_blit.valid() && g_blitVao != 0;
}
} // namespace
bool ensureReady() {
  GLCore::ensureLoaded();
  if(!GLCore::shaderSupported || !GLCore::vaoSupported) {
    return false;
  }
  if(!g_triedProgram) {
    g_triedProgram = true;
    g_program = g_cache.get("shaders/engine/legacy.vsh", "shaders/engine/legacy.fsh", kVersion);
  }
  ensureVao();
  return g_program != nullptr && g_vao != 0;
}
ShaderProgram* program() {
  return g_program;
}
void bindAndUploadUniforms() {
  if(g_program == nullptr) {
    return;
  }
  static UniformSnapshot snap;
  g_program->bind();
  const UniformDelta delta = diffAndUpdate(
      snap, g_pipeline, g_engineLighting, math::g_modelView.top().data(), math::g_projection.top().data());
  if(delta.modelView) {
    g_program->setMatrix4("uModelView", math::g_modelView.top());
  }
  if(delta.projection) {
    g_program->setMatrix4("uProjection", math::g_projection.top());
  }
  if(delta.pipeline) {
    const PipelineState& p = g_pipeline;
    g_program->set1i("uUseTexture", p.texture2D ? 1 : 0);
    g_program->set4f("uConstColor", p.constColor[0], p.constColor[1], p.constColor[2], p.constColor[3]);
    g_program->set1i("uAlphaTest", p.alphaTest ? 1 : 0);
    g_program->set1f("uAlphaRef", p.alphaRef);
    g_program->set1i("uLighting", p.lighting ? 1 : 0);
    g_program->set1i("uFogMode", p.fogEnabled ? p.fogMode : 0);
    g_program->set4f("uFogColor", p.fogColor[0], p.fogColor[1], p.fogColor[2], p.fogColor[3]);
    g_program->set1f("uFogDensity", p.fogDensity);
    g_program->set1f("uFogStart", p.fogStart);
    g_program->set1f("uFogEnd", p.fogEnd);
    g_program->set1i("uTexture", 0);
    g_pipeline.dirty = false;
  }
  if(delta.lighting) {
    const EngineLighting& l = g_engineLighting;
    g_program->set3f("uLightDir0", l.dir0[0], l.dir0[1], l.dir0[2]);
    g_program->set3f("uLightDir1", l.dir1[0], l.dir1[1], l.dir1[2]);
    g_program->set3f("uLightColor0", l.color0[0], l.color0[1], l.color0[2]);
    g_program->set3f("uLightColor1", l.color1[0], l.color1[1], l.color1[2]);
    g_program->set3f("uAmbient", l.ambient[0], l.ambient[1], l.ambient[2]);
  }
}
void configureAttribs(std::size_t baseOffset,
                      int stride,
                      bool hasTexture,
                      bool hasColor,
                      bool hasNormals) {
  if(g_vao != 0) {
    GLCore::bindVertexArray(g_vao);
  }
  // Position (always present).
  GLCore::enableVertexAttribArray(0);
  GLCore::vertexAttribPointer(0, 3, kFloat, 0, stride, bufOffset(baseOffset + kOffPos));
  // UV.
  if(hasTexture) {
    GLCore::enableVertexAttribArray(1);
    GLCore::vertexAttribPointer(1, 2, kFloat, 0, stride, bufOffset(baseOffset + kOffUV));
  } else {
    GLCore::disableVertexAttribArray(1);
    GLCore::vertexAttrib4f(1, 0.0f, 0.0f, 0.0f, 0.0f);
  }
  // Color (normalized ubyte).
  if(hasColor) {
    GLCore::enableVertexAttribArray(2);
    GLCore::vertexAttribPointer(2, 4, kUnsignedByte, 1, stride, bufOffset(baseOffset + kOffColor));
  } else {
    GLCore::disableVertexAttribArray(2);
    GLCore::vertexAttrib4f(2, 1.0f, 1.0f, 1.0f, 1.0f);
  }
  // Normal (normalized byte).
  if(hasNormals) {
    GLCore::enableVertexAttribArray(3);
    GLCore::vertexAttribPointer(3, 3, kByte, 1, stride, bufOffset(baseOffset + kOffNormal));
  } else {
    GLCore::disableVertexAttribArray(3);
    GLCore::vertexAttrib4f(
        3, g_pipeline.currentNormal[0], g_pipeline.currentNormal[1], g_pipeline.currentNormal[2], 0.0f);
  }
}
void finishAttribs() {
  if(g_vao != 0) {
    GLCore::bindVertexArray(0);
  }
}
void drawFromBoundBuffer(std::size_t byteOffset,
                         std::size_t vertexCount,
                         int stride,
                         int glMode,
                         bool hasTexture,
                         bool hasColor,
                         bool hasNormals) {
  if(!ensureReady() || vertexCount == 0) {
    return;
  }
  bindAndUploadUniforms();
  configureAttribs(byteOffset, stride, hasTexture, hasColor, hasNormals);
  ::glDrawArrays(static_cast<GLenum>(glMode), 0, static_cast<GLsizei>(vertexCount));
  finishAttribs();
}
void drawInterleaved(const void* data,
                     std::size_t vertexCount,
                     int stride,
                     int glMode,
                     bool hasTexture,
                     bool hasColor,
                     bool hasNormals) {
  if(!ensureReady() || vertexCount == 0) {
    return;
  }
  bindAndUploadUniforms();
  uploadStreaming(data, vertexCount * static_cast<std::size_t>(stride));
  configureAttribs(0, stride, hasTexture, hasColor, hasNormals);
  ::glDrawArrays(static_cast<GLenum>(glMode), 0, static_cast<GLsizei>(vertexCount));
  finishAttribs();
  GLCore::bindBuffer(kArrayBuffer, 0);
}
void blitFullscreen() {
  if(!ensureBlitResources()) {
    return;
  }
  g_blit.bind();
  g_blit.set1i("uTexture", 0);
  GLCore::bindVertexArray(g_blitVao);
  GLCore::bindBuffer(kArrayBuffer, g_blitVbo);
  const int stride = 4 * static_cast<int>(sizeof(float));
  GLCore::enableVertexAttribArray(0);
  GLCore::vertexAttribPointer(0, 2, kFloat, 0, stride, bufOffset(0));
  GLCore::enableVertexAttribArray(1);
  GLCore::vertexAttribPointer(1, 2, kFloat, 0, stride, bufOffset(2 * sizeof(float)));
  ::glDrawArrays(static_cast<GLenum>(0x0004 /*TRIANGLES*/), 0, 3);
  GLCore::bindVertexArray(0);
  GLCore::bindBuffer(kArrayBuffer, 0);
}
bool drawFullscreen(ShaderProgram& program, int textureId, int depthTextureId) {
  if(!ensureBlitResources()) {
    return false;
  }
  program.bind();
  program.set1i("uTexture", 0);
  program.set1i("colortex0", 0);
  if(depthTextureId > 0) {
    program.set1i("depthtex0", 1);
  }
  GLCore::bindVertexArray(g_blitVao);
  GLCore::bindBuffer(kArrayBuffer, g_blitVbo);
  const int stride = 4 * static_cast<int>(sizeof(float));
  GLCore::enableVertexAttribArray(0);
  GLCore::vertexAttribPointer(0, 2, kFloat, 0, stride, bufOffset(0));
  GLCore::enableVertexAttribArray(1);
  GLCore::vertexAttribPointer(1, 2, kFloat, 0, stride, bufOffset(2 * sizeof(float)));
  ::glDrawArrays(static_cast<GLenum>(0x0004), 0, 3);
  GLCore::bindVertexArray(0);
  GLCore::bindBuffer(kArrayBuffer, 0);
  (void)textureId;
  return true;
}
} // namespace engine_pipeline
} // namespace net::minecraft::client::gl
