#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/ClientLog.hpp"
#include "net/minecraft/client/gl/GlConstants.hpp"
namespace net::minecraft::client::gl {
#ifndef NDEBUG
namespace {
void APIENTRY glDebugCallback(unsigned /*source*/,
                              unsigned type,
                              unsigned /*id*/,
                              unsigned severity,
                              int /*length*/,
                              const char* message,
                              const void* /*userParam*/) {
 // 0x9146 = GL_DEBUG_SEVERITY_HIGH, 0x824C = GL_DEBUG_TYPE_ERROR.
 if(severity == 0x9146 || type == 0x824C) {
  ClientLog::LOGGER.log(LogLevel::Warning, std::string("[GL] ") + (message ? message : "(null)"));
 }
}
} // namespace
#endif
#define GLFN(type, name) type GLCore::name = nullptr
GLFN(PFN_GenBuffers, genBuffers);
GLFN(PFN_BindBuffer, bindBuffer);
GLFN(PFN_BufferData, bufferData);
GLFN(PFN_BufferSubData, bufferSubData);
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
GLFN(PFN_MultiDrawArrays, multiDrawArrays);
GLFN(PFN_CreateShader, createShader);
GLFN(PFN_ShaderSource, shaderSource);
GLFN(PFN_CompileShader, compileShader);
GLFN(PFN_GetShaderiv, getShaderiv);
GLFN(PFN_GetShaderInfoLog, getShaderInfoLog);
GLFN(PFN_CreateProgram, createProgram);
GLFN(PFN_AttachShader, attachShader);
GLFN(PFN_LinkProgram, linkProgram);
GLFN(PFN_GetProgramiv, getProgramiv);
GLFN(PFN_GetProgramInfoLog, getProgramInfoLog);
GLFN(PFN_UseProgram, useProgram);
GLFN(PFN_DeleteShader, deleteShader);
GLFN(PFN_DeleteProgram, deleteProgram);
GLFN(PFN_GetUniformLocation, getUniformLocation);
GLFN(PFN_GetActiveUniform, getActiveUniform);
GLFN(PFN_Uniform1f, uniform1f);
GLFN(PFN_Uniform2f, uniform2f);
GLFN(PFN_Uniform3f, uniform3f);
GLFN(PFN_Uniform4f, uniform4f);
GLFN(PFN_Uniform1i, uniform1i);
GLFN(PFN_Uniform2i, uniform2i);
GLFN(PFN_Uniform3i, uniform3i);
GLFN(PFN_Uniform4i, uniform4i);
GLFN(PFN_UniformMatrix4fv, uniformMatrix4fv);
GLFN(PFN_GetAttribLocation, getAttribLocation);
GLFN(PFN_BindAttribLocation, bindAttribLocation);
GLFN(PFN_GenVertexArrays, genVertexArrays);
GLFN(PFN_BindVertexArray, bindVertexArray);
GLFN(PFN_DeleteVertexArrays, deleteVertexArrays);
GLFN(PFN_VertexAttribPointer, vertexAttribPointer);
GLFN(PFN_EnableVertexAttribArray, enableVertexAttribArray);
GLFN(PFN_DisableVertexAttribArray, disableVertexAttribArray);
GLFN(PFN_VertexAttrib4f, vertexAttrib4f);
GLFN(PFN_BindBufferRange, bindBufferRange);
GLFN(PFN_GenerateMipmap, generateMipmap);
GLFN(PFN_GetStringi, getStringi);
GLFN(PFN_DebugMessageCallback, debugMessageCallback);
GLFN(PFN_BlitFramebuffer, blitFramebuffer);
GLFN(PFN_MapBufferRange, mapBufferRange);
GLFN(PFN_UnmapBuffer, unmapBuffer);
GLFN(PFN_FlushMappedBufferRange, flushMappedBufferRange);
GLFN(PFN_TexImage3D, texImage3D);
GLFN(PFN_TexSubImage3D, texSubImage3D);
GLFN(PFN_GenQueries, genQueries);
GLFN(PFN_DeleteQueries, deleteQueries);
GLFN(PFN_BeginQuery, beginQuery);
GLFN(PFN_EndQuery, endQuery);
GLFN(PFN_GetQueryObjectiv, getQueryObjectiv);
GLFN(PFN_GetQueryObjectui64v, getQueryObjectui64v);
void* GLCore::activeTexture = nullptr;
#undef GLFN
bool GLCore::vboSupported = false;
bool GLCore::framebufferSupported = false;
bool GLCore::vaoSupported = false;
bool GLCore::shaderSupported = false;
bool GLCore::timerQuerySupported = false;
static bool g_loaded = false;
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
 if(g_loaded || wglGetCurrentContext() == nullptr) {
  return;
 }
 g_loaded = true;
 LOAD_TRY(texImage3D, "glTexImage3D", "glTexImage3DEXT");
 LOAD_TRY(texSubImage3D, "glTexSubImage3D", "glTexSubImage3DEXT");
 LOAD_TRY(genBuffers, "glGenBuffersARB", "glGenBuffers");
 LOAD_TRY(bindBuffer, "glBindBufferARB", "glBindBuffer");
 LOAD_TRY(bufferData, "glBufferDataARB", "glBufferData");
 LOAD_TRY(bufferSubData, "glBufferSubDataARB", "glBufferSubData");
 LOAD_TRY(deleteBuffers, "glDeleteBuffersARB", "glDeleteBuffers");
 LOAD_TRY(swapInterval, "wglSwapIntervalEXT");
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
 LOAD_TRY(multiDrawArrays, "glMultiDrawArrays", "glMultiDrawArraysEXT");
 LOAD_TRY(createShader, "glCreateShader", "glCreateShaderObjectARB");
 LOAD_TRY(shaderSource, "glShaderSource", "glShaderSourceARB");
 LOAD_TRY(compileShader, "glCompileShader", "glCompileShaderARB");
 LOAD_TRY(getShaderiv, "glGetShaderiv", "glGetObjectParameterivARB");
 LOAD_TRY(getShaderInfoLog, "glGetShaderInfoLog", "glGetInfoLogARB");
 LOAD_TRY(createProgram, "glCreateProgram", "glCreateProgramObjectARB");
 LOAD_TRY(attachShader, "glAttachShader", "glAttachObjectARB");
 LOAD_TRY(linkProgram, "glLinkProgram", "glLinkProgramARB");
 LOAD_TRY(getProgramiv, "glGetProgramiv", "glGetObjectParameterivARB");
 LOAD_TRY(getProgramInfoLog, "glGetProgramInfoLog", "glGetInfoLogARB");
 LOAD_TRY(useProgram, "glUseProgram", "glUseProgramObjectARB");
 LOAD_TRY(deleteShader, "glDeleteShader", "glDeleteObjectARB");
 LOAD_TRY(deleteProgram, "glDeleteProgram", "glDeleteObjectARB");
 LOAD_TRY(getUniformLocation, "glGetUniformLocation", "glGetUniformLocationARB");
 LOAD_TRY(getActiveUniform, "glGetActiveUniform", "glGetActiveUniformARB");
 LOAD_TRY(uniform1f, "glUniform1f", "glUniform1fARB");
 LOAD_TRY(uniform2f, "glUniform2f", "glUniform2fARB");
 LOAD_TRY(uniform3f, "glUniform3f", "glUniform3fARB");
 LOAD_TRY(uniform4f, "glUniform4f", "glUniform4fARB");
 LOAD_TRY(uniform1i, "glUniform1i", "glUniform1iARB");
 LOAD_TRY(uniform2i, "glUniform2i", "glUniform2iARB");
 LOAD_TRY(uniform3i, "glUniform3i", "glUniform3iARB");
 LOAD_TRY(uniform4i, "glUniform4i", "glUniform4iARB");
 LOAD_TRY(uniformMatrix4fv, "glUniformMatrix4fv", "glUniformMatrix4fvARB");
 LOAD_TRY(getAttribLocation, "glGetAttribLocation", "glGetAttribLocationARB");
 LOAD_TRY(bindAttribLocation, "glBindAttribLocation", "glBindAttribLocationARB");
 LOAD_TRY(genVertexArrays, "glGenVertexArrays", "glGenVertexArraysAPPLE");
 LOAD_TRY(bindVertexArray, "glBindVertexArray", "glBindVertexArrayAPPLE");
 LOAD_TRY(deleteVertexArrays, "glDeleteVertexArrays", "glDeleteVertexArraysAPPLE");
 LOAD_TRY(vertexAttribPointer, "glVertexAttribPointer", "glVertexAttribPointerARB");
 LOAD_TRY(enableVertexAttribArray, "glEnableVertexAttribArray", "glEnableVertexAttribArrayARB");
 LOAD_TRY(disableVertexAttribArray, "glDisableVertexAttribArray", "glDisableVertexAttribArrayARB");
 LOAD_TRY(vertexAttrib4f, "glVertexAttrib4f", "glVertexAttrib4fARB");
 LOAD_TRY(bindBufferRange, "glBindBufferRange", "glBindBufferRangeEXT");
 LOAD_TRY(generateMipmap, "glGenerateMipmap", "glGenerateMipmapEXT");
 LOAD_TRY(getStringi, "glGetStringi");
 LOAD_TRY(debugMessageCallback, "glDebugMessageCallback", "glDebugMessageCallbackARB", "glDebugMessageCallbackKHR");
 LOAD_TRY(blitFramebuffer, "glBlitFramebuffer", "glBlitFramebufferEXT");
 LOAD_TRY(mapBufferRange, "glMapBufferRange");
 LOAD_TRY(unmapBuffer, "glUnmapBuffer", "glUnmapBufferARB");
 LOAD_TRY(flushMappedBufferRange, "glFlushMappedBufferRange");
 LOAD_TRY(genQueries, "glGenQueries", "glGenQueriesARB");
 LOAD_TRY(deleteQueries, "glDeleteQueries", "glDeleteQueriesARB");
 LOAD_TRY(beginQuery, "glBeginQuery", "glBeginQueryARB");
 LOAD_TRY(endQuery, "glEndQuery", "glEndQueryARB");
 LOAD_TRY(getQueryObjectiv, "glGetQueryObjectiv", "glGetQueryObjectivARB");
 LOAD_TRY(getQueryObjectui64v, "glGetQueryObjectui64v", "glGetQueryObjectui64vEXT");
 GLCore::activeTexture =
     reinterpret_cast<void*>(reinterpret_cast<std::size_t>(wglGetProcAddress("glActiveTexture")));
 vboSupported = genBuffers && bindBuffer && bufferData;
 framebufferSupported = genFramebuffers && bindFramebuffer && deleteFramebuffers && checkFramebufferStatus &&
                        framebufferTexture2D && genRenderbuffers && bindRenderbuffer && deleteRenderbuffers &&
                        renderbufferStorage && framebufferRenderbuffer;
 vaoSupported = genVertexArrays && bindVertexArray && deleteVertexArrays;
 timerQuerySupported = genQueries && deleteQueries && beginQuery && endQuery && getQueryObjectiv &&
                       getQueryObjectui64v;
 if(multiDrawArrays == nullptr) {
  ClientLog::LOGGER.log(LogLevel::Warning,
                        "[GL] glMultiDrawArrays unavailable; chunk draws fall back to per-slot glDrawArrays");
 }
 shaderSupported = createShader && shaderSource && compileShader && createProgram && linkProgram &&
                   useProgram && getUniformLocation && vertexAttribPointer && enableVertexAttribArray;
#ifndef NDEBUG
 if(debugMessageCallback != nullptr) {
  ::glEnable(0x92E0); // GL_DEBUG_OUTPUT
  debugMessageCallback(glDebugCallback, nullptr);
 }
#endif
}
#undef LOAD_TRY
void GLCore::ensureLoaded() {
 init();
}
} // namespace net::minecraft::client::gl
