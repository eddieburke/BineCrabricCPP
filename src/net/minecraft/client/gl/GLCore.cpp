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
void* GLCore::activeTexture = nullptr;
#undef GLFN
bool GLCore::vboSupported = false;
bool GLCore::framebufferSupported = false;
static bool g_loaded = false;
#define LOAD(dst, name) GLCore::dst = reinterpret_cast<decltype(GLCore::dst)>(reinterpret_cast<std::size_t>(wglGetProcAddress(name)))
#define LOAD_TRY(dst, ...)                                                                                          \
  do {                                                                                                              \
    constexpr const char* _try_names_[] = {__VA_ARGS__};                                                            \
    for(const char* _n_ : _try_names_) {                                                                            \
      GLCore::dst = reinterpret_cast<decltype(GLCore::dst)>(reinterpret_cast<std::size_t>(wglGetProcAddress(_n_))); \
      if(GLCore::dst) break;                                                                                        \
    }                                                                                                               \
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
  GLCore::activeTexture = reinterpret_cast<void*>(reinterpret_cast<std::size_t>(wglGetProcAddress("glActiveTexture")));
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
