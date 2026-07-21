#pragma once
// EnginePipeline — the core-profile replacement for fixed-function drawing. A uniform +
// draw bridge: it binds whatever program the active RenderType/shaderpack resolved, a
// shared streaming VBO + VAO, and uploads the current matrices (from the CPU MatrixStacks)
// plus RenderSystem state, fog, and the single world-light block as uniforms. Every
// Tessellator draw and the fullscreen blit route through drawInterleaved().
#include <cstddef>
#include "net/minecraft/util/math/Matrix4f.hpp"
namespace net::minecraft::client::gl {
class ShaderProgram;
namespace engine_pipeline {
namespace math = net::minecraft::util::math;
// The one lighting block every world program reads. Sourced from the world sun
// (UnifiedLightRegistry) transformed into view space, plus a flat ambient term.
// `enabled` mirrors the RenderSystem lighting capability; `worldTime` drives animated
// pack shaders. GUI/text programs simply don't declare these uniforms (set* no-ops).
struct WorldLightUniforms {
 bool enabled = false;
 float sunDirView[3] = {0.0f, 0.0f, 0.0f};
 float sunColor[3] = {1.0f, 1.0f, 1.0f};
 float sunIntensity = 0.0f;
 float fillDirView[3] = {0.0f, 0.0f, 0.0f};
 float fillIntensity = 0.0f;
 float ambient[3] = {1.0f, 1.0f, 1.0f};
 float worldTime = 0.0f;
 float brightness = 0.0f;
};
struct FogUniforms {
 bool enabled = false;
 int mode = 0; // 0 off, 1 linear, 2 exp, 3 exp2 (matches legacy.fsh uFogMode)
 float color[4] = {0.0f, 0.0f, 0.0f, 0.0f};
 float density = 0.0f;
 float start = 0.0f;
 float end = 0.0f;
};
struct PipelineState {
 bool blend = false;
 bool depthTest = false;
 bool cullFace = false;
 bool polygonOffset = false;
 bool depthWrite = true;
 bool texture2D = false;
 int blendSrc = 1;
 int blendDst = 0;
 int depthFunc = 0x0201;
 int cullFaceMode = 0x0405;
 float alphaRef = 0.1f;
 int alphaFunc = 0x0207;
 float constColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
};
struct RenderPass {
 math::Matrix4f modelView = math::Matrix4f::identityMatrix();
 math::Matrix4f projection = math::Matrix4f::identityMatrix();
 const void* vertexData = nullptr;
 unsigned buffer = 0;
 std::size_t byteOffset = 0;
 std::size_t vertexCount = 0;
 int stride = 0;
 int glMode = 0x0004;
 bool hasTexture = false;
 bool hasColor = false;
 bool hasNormals = false;
 bool hasLightmap = false;
 FogUniforms fog{};
 PipelineState state{};
 ShaderProgram* programOverride = nullptr;
};
void submitFullscreenPass(const RenderPass& pass);
// Sets the fog state the ubershader reads (uFogMode/uFogColor/uFogDensity/uFogStart/
// uFogEnd). Reproduces the fixed-function GL_FOG cap + glFogi/glFogf/glFogfv state
// that used to live in the removed PipelineState. `enabled` mirrors the GL_FOG
// capability toggle; `mode` only takes effect while enabled is true.
// Sets the per-frame world-light block (sun-in-view, color, intensity, ambient, worldTime)
// that every world program reads. GameRenderer computes it once per frame from the world
// sun. Preserves the `enabled` flag, which is owned by setLightingEnabled below.
void setWorldLight(const WorldLightUniforms& light);
// Toggles the world-light `enabled` flag (uLighting). Driven by RenderSystem's
// enableLighting/disableLighting so lit vs fullbright draws behave as before.
void setLightingEnabled(bool enabled);
void setFog(const FogUniforms& fog);
const FogUniforms& fog();
// Ensures the shared VAO/streaming VBO exist on first use (idempotent). Safe to call
// before a GL context exists — it simply returns false until GL/shader support is ready.
bool ensureReady();
// Returns the currently active program, or nullptr if none is bound.
ShaderProgram* program();
// Binds the ubershader (or override) and uploads all current frame/pipeline uniforms.
// Matrices come from `pass`, not the global stacks. Called by submit() automatically,
// but exposed for the chunk path which manages its own vertex source (multiDrawArrays).
void bindAndUploadUniforms(const RenderPass& pass);
// Builds a pass from the current global matrices/uniforms and uploads it.
void bindAndUploadUniforms();
// Core draw entry point: uploads uniforms from an explicit pass (matrices from pass,
// not globals) and draws. Used by both the interleaved and bound-buffer paths.
void submit(const RenderPass& pass);
// The program every subsequent draw uses, set by the active RenderType from the resolved
// pack/engine program. nullptr means no program is bound and draws are skipped.
// RenderType::setupRenderState() sets this so a draw runs its resolved program.
void setActiveProgram(ShaderProgram* program);
ShaderProgram* activeProgram();
ShaderProgram* getActiveProgram();
// Back-compat alias kept for callers that still reference the old terrain override.
inline void setTerrainProgramOverride(ShaderProgram* program) { setActiveProgram(program); }
// Uploads and draws interleaved vertices in the TessellatorVertex layout.
// Offsets are fixed: pos@0 (3f), uv@12 (2f), color@20 (4 ubyte norm), normal@24 (3 byte norm),
// light@28 (2f) when hasLightmap is true.
void drawInterleaved(const void* data,
                     std::size_t vertexCount,
                     int stride,
                     int glMode,
                     bool hasTexture,
                     bool hasColor,
                     bool hasNormals,
                     bool hasLightmap = false);
// Draws directly from a currently-bound GL_ARRAY_BUFFER (VBO). `buffer` names that
// bound VBO so redundant attrib reconfiguration can be skipped; `byteOffset` is the
// start offset into it. Used by mesh/chunk paths that own their VBO.
void drawFromBoundBuffer(unsigned buffer,
                         std::size_t byteOffset,
                         std::size_t vertexCount,
                         int stride,
                         int glMode,
                         bool hasTexture,
                         bool hasColor,
                         bool hasNormals,
                         bool hasLightmap = false);
// Binds the shared VAO and configures the generic attributes against the currently
// bound GL_ARRAY_BUFFER (`buffer` must name it) at the given base offset. Skips
// reconfiguration when the buffer/offset/stride/layout match the previous call.
// Enables/disables attribs to match the has* flags and supplies neutral constants for
// the disabled ones. hasLightmap additionally wires the aLight attribute (loc 4).
// Callers that issue their own draw (e.g. multiDrawArrays) use this + finishAttribs().
void configureAttribs(unsigned buffer,
                      std::size_t baseOffset,
                      int stride,
                      bool hasTexture,
                      bool hasColor,
                      bool hasNormals,
                      bool hasLightmap = false);
void finishAttribs();
// Must be called whenever a VBO that may have been captured by the shared VAO's attrib
// pointers is deleted, so a recycled buffer id cannot false-hit the layout cache.
void invalidateAttribCache();
// Fullscreen-triangle blit of the currently bound 2D texture (replaces the last
// glBegin in drawFullscreenTexturedQuad). Assumes the caller set the desired viewport.
void blitFullscreen();
// Draws the shared fullscreen-triangle VAO without touching the shader program or
// uniforms, so a caller can bind its own program + samplers first (e.g. shaderpack
// post-process passes). Restores the previous VAO afterwards.
void drawFullscreenTriangle();
} // namespace engine_pipeline
} // namespace net::minecraft::client::gl
