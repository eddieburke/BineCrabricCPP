#include "net/minecraft/client/gl/GLCore.hpp"
namespace net::minecraft::client::gl {
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
GLFN(PFN_Uniform1f, uniform1f);
GLFN(PFN_Uniform2f, uniform2f);
GLFN(PFN_Uniform3f, uniform3f);
GLFN(PFN_Uniform4f, uniform4f);
GLFN(PFN_Uniform1i, uniform1i);
GLFN(PFN_Uniform2i, uniform2i);
GLFN(PFN_Uniform3i, uniform3i);
GLFN(PFN_Uniform4i, uniform4i);
void* GLCore::activeTexture = nullptr;
#undef GLFN
bool GLCore::vboSupported = false;
bool GLCore::framebufferSupported = false;
static bool g_loaded = false;
#define LOAD(dst, name) \
  GLCore::dst = reinterpret_cast<decltype(GLCore::dst)>(reinterpret_cast<std::size_t>(wglGetProcAddress(name)))
#define LOAD_TRY(dst, ...)                                                                                \
  do {                                                                                                    \
    constexpr const char* _try_names_[] = {__VA_ARGS__};                                                  \
    for(const char* _n_ : _try_names_) {                                                                  \
      GLCore::dst =                                                                                       \
          reinterpret_cast<decltype(GLCore::dst)>(reinterpret_cast<std::size_t>(wglGetProcAddress(_n_))); \
      if(GLCore::dst)                                                                                     \
        break;                                                                                            \
    }                                                                                                     \
  } while(0)
void GLCore::init() {
  if(g_loaded) {
    return;
  }
  g_loaded = true;
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
  LOAD_TRY(uniform1f, "glUniform1f", "glUniform1fARB");
  LOAD_TRY(uniform2f, "glUniform2f", "glUniform2fARB");
  LOAD_TRY(uniform3f, "glUniform3f", "glUniform3fARB");
  LOAD_TRY(uniform4f, "glUniform4f", "glUniform4fARB");
  LOAD_TRY(uniform1i, "glUniform1i", "glUniform1iARB");
  LOAD_TRY(uniform2i, "glUniform2i", "glUniform2iARB");
  LOAD_TRY(uniform3i, "glUniform3i", "glUniform3iARB");
  LOAD_TRY(uniform4i, "glUniform4i", "glUniform4iARB");
  GLCore::activeTexture =
      reinterpret_cast<void*>(reinterpret_cast<std::size_t>(wglGetProcAddress("glActiveTexture")));
  vboSupported = genBuffers && bindBuffer && bufferData;
  framebufferSupported = genFramebuffers && bindFramebuffer && deleteFramebuffers && checkFramebufferStatus &&
                         framebufferTexture2D && genRenderbuffers && bindRenderbuffer && deleteRenderbuffers &&
                         renderbufferStorage && framebufferRenderbuffer;
}
#undef LOAD_TRY
#undef LOAD
void GLCore::ensureLoaded() {
  init();
}
} // namespace net::minecraft::client::gl
