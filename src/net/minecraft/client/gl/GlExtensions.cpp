#include "net/minecraft/client/gl/GlExtensions.hpp"

#if defined(_WIN32) && defined(MINECRAFT_GL_REAL)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/gl.h>
#endif

#include <cstring>

namespace net::minecraft::client::gl::GlExtensions {

namespace {

#if defined(_WIN32) && defined(MINECRAFT_GL_REAL)

using GenBuffersFn = void(APIENTRY*)(GLsizei, GLuint*);
using DeleteBuffersFn = void(APIENTRY*)(GLsizei, const GLuint*);
using BindBufferFn = void(APIENTRY*)(GLenum, GLuint);
using BufferDataFn = void(APIENTRY*)(GLenum, GLsizeiptr, const void*, GLenum);

using GenFramebuffersFn = void(APIENTRY*)(GLsizei, GLuint*);
using DeleteFramebuffersFn = void(APIENTRY*)(GLsizei, const GLuint*);
using BindFramebufferFn = void(APIENTRY*)(GLenum, GLuint);
using FramebufferTexture2DFn = void(APIENTRY*)(GLenum, GLenum, GLenum, GLuint, GLint);
using GenRenderbuffersFn = void(APIENTRY*)(GLsizei, GLuint*);
using DeleteRenderbuffersFn = void(APIENTRY*)(GLsizei, const GLuint*);
using BindRenderbufferFn = void(APIENTRY*)(GLenum, GLuint);
using RenderbufferStorageFn = void(APIENTRY*)(GLenum, GLenum, GLsizei, GLsizei);
using FramebufferRenderbufferFn = void(APIENTRY*)(GLenum, GLenum, GLenum, GLuint);
using CheckFramebufferStatusFn = GLenum(APIENTRY*)(GLenum);
using SwapIntervalFn = BOOL(APIENTRY*)(int);

GenBuffersFn pfnGenBuffers = nullptr;
DeleteBuffersFn pfnDeleteBuffers = nullptr;
BindBufferFn pfnBindBuffer = nullptr;
BufferDataFn pfnBufferData = nullptr;

GenFramebuffersFn pfnGenFramebuffers = nullptr;
DeleteFramebuffersFn pfnDeleteFramebuffers = nullptr;
BindFramebufferFn pfnBindFramebuffer = nullptr;
FramebufferTexture2DFn pfnFramebufferTexture2D = nullptr;
GenRenderbuffersFn pfnGenRenderbuffers = nullptr;
DeleteRenderbuffersFn pfnDeleteRenderbuffers = nullptr;
BindRenderbufferFn pfnBindRenderbuffer = nullptr;
RenderbufferStorageFn pfnRenderbufferStorage = nullptr;
FramebufferRenderbufferFn pfnFramebufferRenderbuffer = nullptr;
CheckFramebufferStatusFn pfnCheckFramebufferStatus = nullptr;

bool loaded = false;
bool vboAvailable = false;
bool fboAvailable = false;
SwapIntervalFn pfnSwapInterval = nullptr;

template<typename Fn>
Fn loadProc(const char* name)
{
    return reinterpret_cast<Fn>(reinterpret_cast<void*>(::wglGetProcAddress(name)));
}

template<typename Fn>
void tryLoadProc(const char* name, Fn& out)
{
    out = loadProc<Fn>(name);
}

#endif

} // namespace

void ensureLoaded() noexcept
{
#if defined(_WIN32) && defined(MINECRAFT_GL_REAL)
    if (loaded) {
        return;
    }
    loaded = true;

    const char* extensions = reinterpret_cast<const char*>(::glGetString(GL_EXTENSIONS));

    const bool hasVboExtInString = extensions != nullptr
        && (std::strstr(extensions, "GL_ARB_vertex_buffer_object") != nullptr
            || std::strstr(extensions, "GL_EXT_vertex_buffer_object") != nullptr);

    tryLoadProc("glGenBuffersARB", pfnGenBuffers);
    if (pfnGenBuffers == nullptr) {
        tryLoadProc("glGenBuffers", pfnGenBuffers);
    }
    tryLoadProc("glDeleteBuffersARB", pfnDeleteBuffers);
    if (pfnDeleteBuffers == nullptr) {
        tryLoadProc("glDeleteBuffers", pfnDeleteBuffers);
    }
    tryLoadProc("glBindBufferARB", pfnBindBuffer);
    if (pfnBindBuffer == nullptr) {
        tryLoadProc("glBindBuffer", pfnBindBuffer);
    }
    tryLoadProc("glBufferDataARB", pfnBufferData);
    if (pfnBufferData == nullptr) {
        tryLoadProc("glBufferData", pfnBufferData);
    }

    vboAvailable = pfnGenBuffers != nullptr && pfnDeleteBuffers != nullptr && pfnBindBuffer != nullptr
        && pfnBufferData != nullptr && hasVboExtInString;

    const bool hasFboExtInString = extensions != nullptr
        && (std::strstr(extensions, "GL_EXT_framebuffer_object") != nullptr
            || std::strstr(extensions, "GL_ARB_framebuffer_object") != nullptr);

    tryLoadProc("glGenFramebuffersEXT", pfnGenFramebuffers);
    if (pfnGenFramebuffers == nullptr) {
        tryLoadProc("glGenFramebuffers", pfnGenFramebuffers);
    }
    tryLoadProc("glDeleteFramebuffersEXT", pfnDeleteFramebuffers);
    if (pfnDeleteFramebuffers == nullptr) {
        tryLoadProc("glDeleteFramebuffers", pfnDeleteFramebuffers);
    }
    tryLoadProc("glBindFramebufferEXT", pfnBindFramebuffer);
    if (pfnBindFramebuffer == nullptr) {
        tryLoadProc("glBindFramebuffer", pfnBindFramebuffer);
    }
    tryLoadProc("glFramebufferTexture2DEXT", pfnFramebufferTexture2D);
    if (pfnFramebufferTexture2D == nullptr) {
        tryLoadProc("glFramebufferTexture2D", pfnFramebufferTexture2D);
    }
    tryLoadProc("glGenRenderbuffersEXT", pfnGenRenderbuffers);
    if (pfnGenRenderbuffers == nullptr) {
        tryLoadProc("glGenRenderbuffers", pfnGenRenderbuffers);
    }
    tryLoadProc("glDeleteRenderbuffersEXT", pfnDeleteRenderbuffers);
    if (pfnDeleteRenderbuffers == nullptr) {
        tryLoadProc("glDeleteRenderbuffers", pfnDeleteRenderbuffers);
    }
    tryLoadProc("glBindRenderbufferEXT", pfnBindRenderbuffer);
    if (pfnBindRenderbuffer == nullptr) {
        tryLoadProc("glBindRenderbuffer", pfnBindRenderbuffer);
    }
    tryLoadProc("glRenderbufferStorageEXT", pfnRenderbufferStorage);
    if (pfnRenderbufferStorage == nullptr) {
        tryLoadProc("glRenderbufferStorage", pfnRenderbufferStorage);
    }
    tryLoadProc("glFramebufferRenderbufferEXT", pfnFramebufferRenderbuffer);
    if (pfnFramebufferRenderbuffer == nullptr) {
        tryLoadProc("glFramebufferRenderbuffer", pfnFramebufferRenderbuffer);
    }
    tryLoadProc("glCheckFramebufferStatusEXT", pfnCheckFramebufferStatus);
    if (pfnCheckFramebufferStatus == nullptr) {
        tryLoadProc("glCheckFramebufferStatus", pfnCheckFramebufferStatus);
    }

    tryLoadProc("wglSwapIntervalEXT", pfnSwapInterval);

    fboAvailable = pfnGenFramebuffers != nullptr && pfnDeleteFramebuffers != nullptr && pfnBindFramebuffer != nullptr
        && pfnFramebufferTexture2D != nullptr && pfnGenRenderbuffers != nullptr && pfnDeleteRenderbuffers != nullptr
        && pfnBindRenderbuffer != nullptr && pfnRenderbufferStorage != nullptr && pfnFramebufferRenderbuffer != nullptr
        && pfnCheckFramebufferStatus != nullptr;
    (void)hasFboExtInString;
#endif
}

bool isVboAvailable() noexcept
{
    ensureLoaded();
#if defined(_WIN32) && defined(MINECRAFT_GL_REAL)
    return vboAvailable;
#else
    return false;
#endif
}

bool isFboAvailable() noexcept
{
    ensureLoaded();
#if defined(_WIN32) && defined(MINECRAFT_GL_REAL)
    return fboAvailable;
#else
    return false;
#endif
}

void setSwapInterval(int interval) noexcept
{
#if defined(_WIN32) && defined(MINECRAFT_GL_REAL)
    ensureLoaded();
    if (pfnSwapInterval != nullptr) {
        pfnSwapInterval(interval);
    }
#else
    (void)interval;
#endif
}

void genBuffers(int count, unsigned int* names) noexcept
{
#if defined(_WIN32) && defined(MINECRAFT_GL_REAL)
    if (pfnGenBuffers != nullptr && names != nullptr && count > 0) {
        pfnGenBuffers(count, names);
    }
#else
    (void)count;
    (void)names;
#endif
}

void deleteBuffers(int count, const unsigned int* names) noexcept
{
#if defined(_WIN32) && defined(MINECRAFT_GL_REAL)
    if (pfnDeleteBuffers != nullptr && names != nullptr && count > 0) {
        pfnDeleteBuffers(count, names);
    }
#else
    (void)count;
    (void)names;
#endif
}

void bindBuffer(unsigned int target, unsigned int buffer) noexcept
{
#if defined(_WIN32) && defined(MINECRAFT_GL_REAL)
    if (pfnBindBuffer != nullptr) {
        pfnBindBuffer(static_cast<GLenum>(target), static_cast<GLuint>(buffer));
    }
#else
    (void)target;
    (void)buffer;
#endif
}

void bufferData(unsigned int target, std::ptrdiff_t size, const void* data, unsigned int usage) noexcept
{
#if defined(_WIN32) && defined(MINECRAFT_GL_REAL)
    if (pfnBufferData != nullptr) {
        pfnBufferData(static_cast<GLenum>(target), static_cast<GLsizeiptr>(size), data, static_cast<GLenum>(usage));
    }
#else
    (void)target;
    (void)size;
    (void)data;
    (void)usage;
#endif
}

void genFramebuffers(int n, unsigned int* ids) noexcept
{
#if defined(_WIN32) && defined(MINECRAFT_GL_REAL)
    if (pfnGenFramebuffers != nullptr) {
        pfnGenFramebuffers(n, ids);
    }
#else
    (void)n;
    (void)ids;
#endif
}

void deleteFramebuffers(int n, const unsigned int* ids) noexcept
{
#if defined(_WIN32) && defined(MINECRAFT_GL_REAL)
    if (pfnDeleteFramebuffers != nullptr) {
        pfnDeleteFramebuffers(n, ids);
    }
#else
    (void)n;
    (void)ids;
#endif
}

void bindFramebuffer(unsigned int target, unsigned int framebuffer) noexcept
{
#if defined(_WIN32) && defined(MINECRAFT_GL_REAL)
    if (pfnBindFramebuffer != nullptr) {
        pfnBindFramebuffer(static_cast<GLenum>(target), static_cast<GLuint>(framebuffer));
    }
#else
    (void)target;
    (void)framebuffer;
#endif
}

void framebufferTexture2D(unsigned int target, unsigned int attachment, int textarget, unsigned int texture,
    int level) noexcept
{
#if defined(_WIN32) && defined(MINECRAFT_GL_REAL)
    if (pfnFramebufferTexture2D != nullptr) {
        pfnFramebufferTexture2D(static_cast<GLenum>(target), static_cast<GLenum>(attachment),
            static_cast<GLenum>(textarget), static_cast<GLuint>(texture), level);
    }
#else
    (void)target;
    (void)attachment;
    (void)textarget;
    (void)texture;
    (void)level;
#endif
}

void genRenderbuffers(int n, unsigned int* ids) noexcept
{
#if defined(_WIN32) && defined(MINECRAFT_GL_REAL)
    if (pfnGenRenderbuffers != nullptr) {
        pfnGenRenderbuffers(n, ids);
    }
#else
    (void)n;
    (void)ids;
#endif
}

void deleteRenderbuffers(int n, const unsigned int* ids) noexcept
{
#if defined(_WIN32) && defined(MINECRAFT_GL_REAL)
    if (pfnDeleteRenderbuffers != nullptr) {
        pfnDeleteRenderbuffers(n, ids);
    }
#else
    (void)n;
    (void)ids;
#endif
}

void bindRenderbuffer(unsigned int target, unsigned int renderbuffer) noexcept
{
#if defined(_WIN32) && defined(MINECRAFT_GL_REAL)
    if (pfnBindRenderbuffer != nullptr) {
        pfnBindRenderbuffer(static_cast<GLenum>(target), static_cast<GLuint>(renderbuffer));
    }
#else
    (void)target;
    (void)renderbuffer;
#endif
}

void renderbufferStorage(unsigned int target, unsigned int internalformat, int width, int height) noexcept
{
#if defined(_WIN32) && defined(MINECRAFT_GL_REAL)
    if (pfnRenderbufferStorage != nullptr) {
        pfnRenderbufferStorage(static_cast<GLenum>(target), static_cast<GLenum>(internalformat), width, height);
    }
#else
    (void)target;
    (void)internalformat;
    (void)width;
    (void)height;
#endif
}

void framebufferRenderbuffer(unsigned int target, unsigned int attachment, unsigned int renderbuffertarget,
    unsigned int renderbuffer) noexcept
{
#if defined(_WIN32) && defined(MINECRAFT_GL_REAL)
    if (pfnFramebufferRenderbuffer != nullptr) {
        pfnFramebufferRenderbuffer(static_cast<GLenum>(target), static_cast<GLenum>(attachment),
            static_cast<GLenum>(renderbuffertarget), static_cast<GLuint>(renderbuffer));
    }
#else
    (void)target;
    (void)attachment;
    (void)renderbuffertarget;
    (void)renderbuffer;
#endif
}

unsigned int checkFramebufferStatus(unsigned int target) noexcept
{
#if defined(_WIN32) && defined(MINECRAFT_GL_REAL)
    if (pfnCheckFramebufferStatus != nullptr) {
        return static_cast<unsigned int>(pfnCheckFramebufferStatus(static_cast<GLenum>(target)));
    }
#endif
    (void)target;
    return 0;
}

} // namespace net::minecraft::client::gl::GlExtensions
