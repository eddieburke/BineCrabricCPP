#pragma once
// EnginePipeline — the core-profile replacement for fixed-function drawing. Owns the
// engine ubershader, a shared streaming VBO + VAO, and uploads the current matrices
// (from the CPU MatrixStacks) plus PipelineState/lighting/fog as uniforms. Every
// Tessellator draw and the fullscreen blit route through drawInterleaved().
#include <cstddef>
namespace net::minecraft::client::gl {
class ShaderProgram;
// Item-lighting state, written by gl::Lighting (replaces glLightfv).
struct EngineLighting {
  float dir0[3]{0.0f, 1.0f, 0.0f};
  float dir1[3]{0.0f, 1.0f, 0.0f};
  float color0[3]{0.0f, 0.0f, 0.0f};
  float color1[3]{0.0f, 0.0f, 0.0f};
  float ambient[3]{0.0f, 0.0f, 0.0f};
};
inline EngineLighting g_engineLighting;
namespace engine_pipeline {
// Compiles the ubershader on first use (idempotent). Safe to call before a GL context
// exists — it simply returns false until the context and shader support are ready.
bool ensureReady();
// Returns the engine ubershader, or nullptr if unavailable.
ShaderProgram* program();
// Binds the ubershader and uploads all current frame/pipeline uniforms. Called by
// drawInterleaved automatically, but exposed for the chunk path which manages its own
// vertex source (multiDrawArrays).
void bindAndUploadUniforms();
// Uploads and draws interleaved vertices in the TessellatorVertex layout.
// Offsets are fixed: pos@0 (3f), uv@12 (2f), color@20 (4 ubyte norm), normal@24 (3 byte norm).
void drawInterleaved(const void* data,
                     std::size_t vertexCount,
                     int stride,
                     int glMode,
                     bool hasTexture,
                     bool hasColor,
                     bool hasNormals);
// Draws directly from a currently-bound GL_ARRAY_BUFFER (VBO). `byteOffset` is the
// start offset into that buffer. Used by mesh/chunk paths that own their VBO.
void drawFromBoundBuffer(std::size_t byteOffset,
                         std::size_t vertexCount,
                         int stride,
                         int glMode,
                         bool hasTexture,
                         bool hasColor,
                         bool hasNormals);
// Binds the shared VAO and configures the 4 generic attributes against the currently
// bound GL_ARRAY_BUFFER at the given base offset. Enables/disables attribs to match
// the has* flags and supplies neutral constants for the disabled ones. Callers that
// issue their own draw (e.g. multiDrawArrays) use this + finishAttribs().
void configureAttribs(std::size_t baseOffset,
                      int stride,
                      bool hasTexture,
                      bool hasColor,
                      bool hasNormals);
void finishAttribs();
// Fullscreen-triangle blit of the currently bound 2D texture (replaces the last
// glBegin in drawFullscreenTexturedQuad). Assumes the caller set the desired viewport.
void blitFullscreen();
bool drawFullscreen(ShaderProgram& program, int textureId, int depthTextureId = -1);
} // namespace engine_pipeline
} // namespace net::minecraft::client::gl
