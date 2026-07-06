#pragma once
// GLCore — VBO and multi-texture entry points for fixed-function rendering.
#include <cstdint>
#include <windows.h>
namespace net::minecraft::client::gl {
using PFN_GenBuffers = void(APIENTRY*)(int, unsigned*);
using PFN_BindBuffer = void(APIENTRY*)(unsigned, unsigned);
using PFN_BufferData = void(APIENTRY*)(unsigned, intptr_t, const void*, unsigned);
using PFN_BufferSubData = void(APIENTRY*)(unsigned, intptr_t, intptr_t, const void*);
using PFN_DeleteBuffers = void(APIENTRY*)(int, const unsigned*);
using PFN_SwapInterval = void(APIENTRY*)(int);
struct GLCore {
  static PFN_GenBuffers genBuffers;
  static PFN_BindBuffer bindBuffer;
  static PFN_BufferData bufferData;
  static PFN_BufferSubData bufferSubData;
  static PFN_DeleteBuffers deleteBuffers;
  static PFN_SwapInterval swapInterval;
  static void* activeTexture;
  static bool vboSupported;
  static void init();
  static void ensureLoaded();
};
} // namespace net::minecraft::client::gl
