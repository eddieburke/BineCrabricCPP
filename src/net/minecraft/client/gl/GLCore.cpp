#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/GlConstants.hpp"
#include <algorithm>
#include <cstddef>
#include <cstring>
#include <limits>
#include <mutex>
#include <thread>
#include "net/minecraft/util/concurrent/ThreadCoordinator.hpp"
#include "net/minecraft/util/concurrent/ThreadNames.hpp"
namespace net::minecraft::client::gl {
#define GLFN(type, name) type GLCore::name = nullptr
GLFN(PFN_GenBuffers, genBuffers);
GLFN(PFN_BindBuffer, bindBuffer);
GLFN(PFN_BufferData, bufferData);
GLFN(PFN_BufferSubData, bufferSubData);
GLFN(PFN_BufferStorage, bufferStorage);
GLFN(PFN_ClearBufferSubData, clearBufferSubData);
GLFN(PFN_DeleteBuffers, deleteBuffers);
GLFN(PFN_SwapInterval, swapInterval);
GLFN(PFN_GenFramebuffers, genFramebuffers);
GLFN(PFN_BindFramebuffer, bindFramebuffer);
GLFN(PFN_DeleteFramebuffers, deleteFramebuffers);
GLFN(PFN_CheckFramebufferStatus, checkFramebufferStatus);
GLFN(PFN_FramebufferTexture2D, framebufferTexture2D);
GLFN(PFN_GenRenderbuffers, genRenderbuffers);
GLFN(PFN_BindRenderbuffer, bindRenderbuffer);
GLFN(PFN_DeleteRenderbuffers, deleteRenderbuffers);
GLFN(PFN_RenderbufferStorage, renderbufferStorage);
GLFN(PFN_FramebufferRenderbuffer, framebufferRenderbuffer);
GLFN(PFN_DrawBuffers, drawBuffers);
GLFN(PFN_ClearBufferfv, clearBufferfv);
GLFN(PFN_ClearBufferuiv, clearBufferuiv);
GLFN(PFN_ClearBufferiv, clearBufferiv);
GLFN(PFN_PatchParameteri, patchParameteri);
GLFN(PFN_MultiDrawArrays, multiDrawArrays);
GLFN(PFN_DrawArraysInstanced, drawArraysInstanced);
GLFN(PFN_VertexAttribDivisor, vertexAttribDivisor);
GLFN(PFN_CreateShader, createShader);
GLFN(PFN_ShaderSource, shaderSource);
GLFN(PFN_CompileShader, compileShader);
GLFN(PFN_MaxShaderCompilerThreadsKHR, maxShaderCompilerThreadsKHR);
GLFN(PFN_GetShaderiv, getShaderiv);
GLFN(PFN_GetShaderInfoLog, getShaderInfoLog);
GLFN(PFN_CreateProgram, createProgram);
GLFN(PFN_AttachShader, attachShader);
GLFN(PFN_LinkProgram, linkProgram);
GLFN(PFN_GetProgramiv, getProgramiv);
GLFN(PFN_GetProgramInfoLog, getProgramInfoLog);
GLFN(PFN_GetActiveUniform, getActiveUniform);
GLFN(PFN_UseProgram, useProgram);
GLFN(PFN_DeleteShader, deleteShader);
GLFN(PFN_DeleteProgram, deleteProgram);
GLFN(PFN_GetUniformLocation, getUniformLocation);
GLFN(PFN_Uniform1f, uniform1f);
GLFN(PFN_Uniform2f, uniform2f);
GLFN(PFN_Uniform3f, uniform3f);
GLFN(PFN_Uniform4f, uniform4f);
GLFN(PFN_Uniform1i, uniform1i);
GLFN(PFN_Uniform2i, uniform2i);
GLFN(PFN_Uniform3i, uniform3i);
GLFN(PFN_Uniform4i, uniform4i);
GLFN(PFN_UniformMatrix3fv, uniformMatrix3fv);
GLFN(PFN_UniformMatrix4fv, uniformMatrix4fv);
GLFN(PFN_GetAttribLocation, getAttribLocation);
GLFN(PFN_BindAttribLocation, bindAttribLocation);
GLFN(PFN_GenVertexArrays, genVertexArrays);
GLFN(PFN_BindVertexArray, bindVertexArray);
GLFN(PFN_DeleteVertexArrays, deleteVertexArrays);
GLFN(PFN_VertexAttribPointer, vertexAttribPointer);
GLFN(PFN_VertexAttribIPointer, vertexAttribIPointer);
GLFN(PFN_EnableVertexAttribArray, enableVertexAttribArray);
GLFN(PFN_DisableVertexAttribArray, disableVertexAttribArray);
GLFN(PFN_VertexAttrib4f, vertexAttrib4f);
GLFN(PFN_GenerateMipmap, generateMipmap);
GLFN(PFN_GetStringi, getStringi);
GLFN(PFN_BlitFramebuffer, blitFramebuffer);
GLFN(PFN_TexImage3D, texImage3D);
GLFN(PFN_GenQueries, genQueries);
GLFN(PFN_DeleteQueries, deleteQueries);
GLFN(PFN_BeginQuery, beginQuery);
GLFN(PFN_EndQuery, endQuery);
GLFN(PFN_GetQueryObjectiv, getQueryObjectiv);
GLFN(PFN_GetQueryObjectui64v, getQueryObjectui64v);
GLFN(PFN_DispatchCompute, dispatchCompute);
GLFN(PFN_DispatchComputeIndirect, dispatchComputeIndirect);
GLFN(PFN_MemoryBarrier, memoryBarrier);
GLFN(PFN_BindImageTexture, bindImageTexture);
GLFN(PFN_ClearTexImage, clearTexImage);
GLFN(PFN_BindBufferBase, bindBufferBase);
GLFN(PFN_GenSamplers, genSamplers);
GLFN(PFN_DeleteSamplers, deleteSamplers);
GLFN(PFN_BindSampler, bindSampler);
GLFN(PFN_SamplerParameteri, samplerParameteri);
GLFN(PFN_BlendFunci, blendFunci);
GLFN(PFN_BlendFuncSeparate, blendFuncSeparate);
GLFN(PFN_BlendFuncSeparatei, blendFuncSeparatei);
void* GLCore::activeTexture = nullptr;
#undef GLFN
bool GLCore::vboSupported = false;
bool GLCore::framebufferSupported = false;
bool GLCore::vaoSupported = false;
bool GLCore::shaderSupported = false;
bool GLCore::timerQuerySupported = false;
bool GLCore::computeSupported = false;
bool GLCore::instancedDrawSupported = false;
bool GLCore::ssboSupported = false;
int GLCore::maxShaderStorageUnits = 0;
bool GLCore::samplerObjectsSupported = false;
bool GLCore::perBufferBlendingSupported = false;
bool GLCore::swapControlTearSupported = false;
static std::once_flag g_initOnce;
static int g_appliedSwapInterval = std::numeric_limits<int>::min();
static void* loadProc(const char* name) {
 PROC proc = wglGetProcAddress(name);
 const auto value = reinterpret_cast<std::uintptr_t>(proc);
 if(proc == nullptr || value <= 3 || value == static_cast<std::uintptr_t>(-1)) {
  static HMODULE opengl = GetModuleHandleW(L"opengl32.dll");
  proc = opengl != nullptr ? GetProcAddress(opengl, name) : nullptr;
 }
 return reinterpret_cast<void*>(proc);
}
#define LOAD_TRY(dst, ...)                                               \
 do {                                                                    \
  constexpr const char* _try_names_[] = {__VA_ARGS__};                   \
  for(const char* _n_ : _try_names_) {                                   \
   GLCore::dst = reinterpret_cast<decltype(GLCore::dst)>(loadProc(_n_)); \
   if(GLCore::dst)                                                       \
    break;                                                               \
  }                                                                      \
 } while(0)
void GLCore::init() {
 if(wglGetCurrentContext() == nullptr) {
  return;
 }
 std::call_once(g_initOnce, [] {
  // init() first runs on the main GL thread at display setup; latch the
  // ThreadNames main-thread marker so the Debug confinement asserts (WI-5)
  // are effective before PASS-2 wires setMainThread() into the frame loop.
  net::minecraft::util::concurrent::setMainThread();
  LOAD_TRY(texImage3D, "glTexImage3D", "glTexImage3DEXT");
 LOAD_TRY(genBuffers, "glGenBuffersARB", "glGenBuffers");
 LOAD_TRY(bindBuffer, "glBindBufferARB", "glBindBuffer");
 LOAD_TRY(bufferData, "glBufferDataARB", "glBufferData");
 LOAD_TRY(bufferSubData, "glBufferSubDataARB", "glBufferSubData");
 LOAD_TRY(bufferStorage, "glBufferStorage", "glBufferStorageARB", "glBufferStorageEXT");
 LOAD_TRY(clearBufferSubData, "glClearBufferSubData");
 LOAD_TRY(deleteBuffers, "glDeleteBuffersARB", "glDeleteBuffers");
 LOAD_TRY(swapInterval, "wglSwapIntervalEXT");
 {
  using PFN_GetExtensionsStringEXT = const char*(APIENTRY*)();
  using PFN_GetExtensionsStringARB = const char*(APIENTRY*)(HDC);
  const char* extensions = nullptr;
  auto getExt = reinterpret_cast<PFN_GetExtensionsStringEXT>(loadProc("wglGetExtensionsStringEXT"));
  if(getExt != nullptr) {
   extensions = getExt();
  }
  if(extensions == nullptr) {
   auto getArb = reinterpret_cast<PFN_GetExtensionsStringARB>(loadProc("wglGetExtensionsStringARB"));
   if(getArb != nullptr) {
    const HDC dc = wglGetCurrentDC();
    extensions = dc != nullptr ? getArb(dc) : nullptr;
   }
  }
  swapControlTearSupported =
      extensions != nullptr && std::strstr(extensions, "WGL_EXT_swap_control_tear") != nullptr;
 }
 LOAD_TRY(genFramebuffers, "glGenFramebuffers", "glGenFramebuffersEXT");
 LOAD_TRY(bindFramebuffer, "glBindFramebuffer", "glBindFramebufferEXT");
 LOAD_TRY(deleteFramebuffers, "glDeleteFramebuffers", "glDeleteFramebuffersEXT");
 LOAD_TRY(checkFramebufferStatus, "glCheckFramebufferStatus", "glCheckFramebufferStatusEXT");
 LOAD_TRY(framebufferTexture2D, "glFramebufferTexture2D", "glFramebufferTexture2DEXT");
 LOAD_TRY(genRenderbuffers, "glGenRenderbuffers", "glGenRenderbuffersEXT");
 LOAD_TRY(bindRenderbuffer, "glBindRenderbuffer", "glBindRenderbufferEXT");
 LOAD_TRY(deleteRenderbuffers, "glDeleteRenderbuffers", "glDeleteRenderbuffersEXT");
 LOAD_TRY(renderbufferStorage, "glRenderbufferStorage", "glRenderbufferStorageEXT");
 LOAD_TRY(framebufferRenderbuffer, "glFramebufferRenderbuffer", "glFramebufferRenderbufferEXT");
 LOAD_TRY(drawBuffers, "glDrawBuffers", "glDrawBuffersARB", "glDrawBuffersEXT");
 LOAD_TRY(clearBufferfv, "glClearBufferfv");
 LOAD_TRY(clearBufferuiv, "glClearBufferuiv");
 LOAD_TRY(clearBufferiv, "glClearBufferiv");
 LOAD_TRY(patchParameteri, "glPatchParameteri");
 LOAD_TRY(multiDrawArrays, "glMultiDrawArrays", "glMultiDrawArraysEXT");
 LOAD_TRY(drawArraysInstanced, "glDrawArraysInstanced", "glDrawArraysInstancedARB");
 LOAD_TRY(vertexAttribDivisor, "glVertexAttribDivisor", "glVertexAttribDivisorARB");
 LOAD_TRY(createShader, "glCreateShader", "glCreateShaderObjectARB");
 LOAD_TRY(shaderSource, "glShaderSource", "glShaderSourceARB");
 LOAD_TRY(compileShader, "glCompileShader", "glCompileShaderARB");
 LOAD_TRY(maxShaderCompilerThreadsKHR, "glMaxShaderCompilerThreadsKHR", "glMaxShaderCompilerThreadsARB");
 LOAD_TRY(getShaderiv, "glGetShaderiv", "glGetObjectParameterivARB");
 LOAD_TRY(getShaderInfoLog, "glGetShaderInfoLog", "glGetInfoLogARB");
 LOAD_TRY(createProgram, "glCreateProgram", "glCreateProgramObjectARB");
 LOAD_TRY(attachShader, "glAttachShader", "glAttachObjectARB");
 LOAD_TRY(linkProgram, "glLinkProgram", "glLinkProgramARB");
 LOAD_TRY(getProgramiv, "glGetProgramiv", "glGetObjectParameterivARB");
 LOAD_TRY(getProgramInfoLog, "glGetProgramInfoLog", "glGetInfoLogARB");
 LOAD_TRY(getActiveUniform, "glGetActiveUniform", "glGetActiveUniformARB");
 LOAD_TRY(useProgram, "glUseProgram", "glUseProgramObjectARB");
 LOAD_TRY(deleteShader, "glDeleteShader", "glDeleteObjectARB");
 LOAD_TRY(deleteProgram, "glDeleteProgram", "glDeleteObjectARB");
 LOAD_TRY(getUniformLocation, "glGetUniformLocation", "glGetUniformLocationARB");
 LOAD_TRY(uniform1f, "glUniform1f", "glUniform1fARB");
 LOAD_TRY(uniform2f, "glUniform2f", "glUniform2fARB");
 LOAD_TRY(uniform3f, "glUniform3f", "glUniform3fARB");
 LOAD_TRY(uniform4f, "glUniform4f", "glUniform4fARB");
 LOAD_TRY(uniform1i, "glUniform1i", "glUniform1iARB");
 LOAD_TRY(uniform2i, "glUniform2i", "glUniform2iARB");
 LOAD_TRY(uniform3i, "glUniform3i", "glUniform3iARB");
 LOAD_TRY(uniform4i, "glUniform4i", "glUniform4iARB");
 LOAD_TRY(uniformMatrix3fv, "glUniformMatrix3fv", "glUniformMatrix3fvARB");
 LOAD_TRY(uniformMatrix4fv, "glUniformMatrix4fv", "glUniformMatrix4fvARB");
 LOAD_TRY(getAttribLocation, "glGetAttribLocation", "glGetAttribLocationARB");
 LOAD_TRY(bindAttribLocation, "glBindAttribLocation", "glBindAttribLocationARB");
 LOAD_TRY(genVertexArrays, "glGenVertexArrays", "glGenVertexArraysAPPLE");
 LOAD_TRY(bindVertexArray, "glBindVertexArray", "glBindVertexArrayAPPLE");
 LOAD_TRY(deleteVertexArrays, "glDeleteVertexArrays", "glDeleteVertexArraysAPPLE");
 LOAD_TRY(vertexAttribPointer, "glVertexAttribPointer", "glVertexAttribPointerARB");
 LOAD_TRY(vertexAttribIPointer, "glVertexAttribIPointer");
 LOAD_TRY(enableVertexAttribArray, "glEnableVertexAttribArray", "glEnableVertexAttribArrayARB");
 LOAD_TRY(disableVertexAttribArray, "glDisableVertexAttribArray", "glDisableVertexAttribArrayARB");
 LOAD_TRY(vertexAttrib4f, "glVertexAttrib4f", "glVertexAttrib4fARB");
 LOAD_TRY(generateMipmap, "glGenerateMipmap", "glGenerateMipmapEXT");
 LOAD_TRY(getStringi, "glGetStringi");
 LOAD_TRY(blitFramebuffer, "glBlitFramebuffer", "glBlitFramebufferEXT");
 LOAD_TRY(genQueries, "glGenQueries", "glGenQueriesARB");
 LOAD_TRY(deleteQueries, "glDeleteQueries", "glDeleteQueriesARB");
 LOAD_TRY(beginQuery, "glBeginQuery", "glBeginQueryARB");
 LOAD_TRY(endQuery, "glEndQuery", "glEndQueryARB");
 LOAD_TRY(getQueryObjectiv, "glGetQueryObjectiv", "glGetQueryObjectivARB");
 LOAD_TRY(getQueryObjectui64v, "glGetQueryObjectui64v", "glGetQueryObjectui64vEXT");
 LOAD_TRY(dispatchCompute, "glDispatchCompute");
 LOAD_TRY(dispatchComputeIndirect, "glDispatchComputeIndirect");
 LOAD_TRY(memoryBarrier, "glMemoryBarrier");
 LOAD_TRY(bindImageTexture, "glBindImageTexture");
 LOAD_TRY(clearTexImage, "glClearTexImage");
 LOAD_TRY(bindBufferBase, "glBindBufferBase");
 LOAD_TRY(genSamplers, "glGenSamplers");
 LOAD_TRY(deleteSamplers, "glDeleteSamplers");
 LOAD_TRY(bindSampler, "glBindSampler");
 LOAD_TRY(samplerParameteri, "glSamplerParameteri");
 LOAD_TRY(blendFunci, "glBlendFunci", "glBlendFunciARB");
 LOAD_TRY(blendFuncSeparate, "glBlendFuncSeparate", "glBlendFuncSeparateEXT");
 LOAD_TRY(blendFuncSeparatei, "glBlendFuncSeparatei", "glBlendFuncSeparateiARB");
 GLCore::activeTexture =
     reinterpret_cast<void*>(reinterpret_cast<std::size_t>(wglGetProcAddress("glActiveTexture")));
 vboSupported = genBuffers && bindBuffer && bufferData;
 framebufferSupported = genFramebuffers && bindFramebuffer && deleteFramebuffers && checkFramebufferStatus &&
                        framebufferTexture2D && genRenderbuffers && bindRenderbuffer && deleteRenderbuffers &&
                        renderbufferStorage && framebufferRenderbuffer;
 vaoSupported = genVertexArrays && bindVertexArray && deleteVertexArrays;
 timerQuerySupported =
     genQueries && deleteQueries && beginQuery && endQuery && getQueryObjectiv && getQueryObjectui64v;
  shaderSupported = createShader && shaderSource && compileShader && createProgram && linkProgram && useProgram &&
                    getUniformLocation && vertexAttribPointer && vertexAttribIPointer && enableVertexAttribArray;
  computeSupported = shaderSupported && dispatchCompute && memoryBarrier;
  instancedDrawSupported = shaderSupported && drawArraysInstanced && vertexAttribDivisor;
  // Java: IrisRenderSystem.supportsSSBO() = OpenGL44 || (ARB_SSBO && ARB_buffer_storage).
  // clearBufferSubData is required because the SSBO holder always zero-fills with it.
  ssboSupported = shaderSupported && bindBufferBase && bufferStorage && clearBufferSubData;
  // Java: SamplerLimits queries GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS when SSBO is supported.
  int maxStorageUnits = 0;
  if(ssboSupported) {
   ::glGetIntegerv(0x90DD, &maxStorageUnits);
  }
  maxShaderStorageUnits = maxStorageUnits;
 samplerObjectsSupported = genSamplers && deleteSamplers && bindSampler && samplerParameteri;
 perBufferBlendingSupported = blendFunci != nullptr;
  if(maxShaderCompilerThreadsKHR != nullptr) {
   const unsigned threads = net::minecraft::util::concurrent::ThreadCoordinator::instance().budget().glDriverThreads();
   maxShaderCompilerThreadsKHR(threads);
  }
 });
}
#undef LOAD_TRY
void GLCore::ensureLoaded() {
 init();
}
void GLCore::setSwapPacing(SwapPacing pacing) {
 ensureLoaded();
 int interval = 0;
 switch(pacing) {
 case SwapPacing::Unlimited:
  interval = 0;
  break;
 case SwapPacing::Adaptive:
  interval = swapControlTearSupported ? -1 : 1;
  break;
 case SwapPacing::VSync:
 default:
  interval = 1;
  break;
 }
 if(interval == g_appliedSwapInterval) {
  return;
 }
 if(swapInterval != nullptr) {
  swapInterval(interval);
  g_appliedSwapInterval = interval;
 }
}
void GLCore::resetSwapPacingCache() {
 g_appliedSwapInterval = std::numeric_limits<int>::min();
}
bool GLCore::present() {
 const HDC dc = wglGetCurrentDC();
 if(dc == nullptr) {
  return false;
 }
 return SwapBuffers(dc) != FALSE;
}
} // namespace net::minecraft::client::gl
