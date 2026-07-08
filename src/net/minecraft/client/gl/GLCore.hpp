#pragma once
// GLCore — VBO and multi-texture entry points for fixed-function rendering.
#include <windows.h>

#include <cstdint>

namespace net::minecraft::client::gl {
using PFN_GenBuffers = void(APIENTRY*)(int, unsigned*);
using PFN_BindBuffer = void(APIENTRY*)(unsigned, unsigned);
using PFN_BufferData = void(APIENTRY*)(unsigned, intptr_t, const void*, unsigned);
using PFN_BufferSubData = void(APIENTRY*)(unsigned, intptr_t, intptr_t, const void*);
using PFN_DeleteBuffers = void(APIENTRY*)(int, const unsigned*);
using PFN_SwapInterval = void(APIENTRY*)(int);
using PFN_GenFramebuffers = void(APIENTRY*)(int, unsigned*);
using PFN_BindFramebuffer = void(APIENTRY*)(unsigned, unsigned);
using PFN_DeleteFramebuffers = void(APIENTRY*)(int, const unsigned*);
using PFN_CheckFramebufferStatus = unsigned(APIENTRY*)(unsigned);
using PFN_FramebufferTexture2D = void(APIENTRY*)(unsigned, unsigned, unsigned, unsigned, int);
using PFN_GenRenderbuffers = void(APIENTRY*)(int, unsigned*);
using PFN_BindRenderbuffer = void(APIENTRY*)(unsigned, unsigned);
using PFN_DeleteRenderbuffers = void(APIENTRY*)(int, const unsigned*);
using PFN_RenderbufferStorage = void(APIENTRY*)(unsigned, unsigned, int, int);
using PFN_FramebufferRenderbuffer = void(APIENTRY*)(unsigned, unsigned, unsigned, unsigned);

struct GLCore {
    static PFN_GenBuffers genBuffers;
    static PFN_BindBuffer bindBuffer;
    static PFN_BufferData bufferData;
    static PFN_BufferSubData bufferSubData;
    static PFN_DeleteBuffers deleteBuffers;
    static PFN_SwapInterval swapInterval;
    static PFN_GenFramebuffers genFramebuffers;
    static PFN_BindFramebuffer bindFramebuffer;
    static PFN_DeleteFramebuffers deleteFramebuffers;
    static PFN_CheckFramebufferStatus checkFramebufferStatus;
    static PFN_FramebufferTexture2D framebufferTexture2D;
    static PFN_GenRenderbuffers genRenderbuffers;
    static PFN_BindRenderbuffer bindRenderbuffer;
    static PFN_DeleteRenderbuffers deleteRenderbuffers;
    static PFN_RenderbufferStorage renderbufferStorage;
    static PFN_FramebufferRenderbuffer framebufferRenderbuffer;
    static void* activeTexture;
    static bool vboSupported;
    static bool framebufferSupported;
    static void init();
    static void ensureLoaded();
};
}  // namespace net::minecraft::client::gl
