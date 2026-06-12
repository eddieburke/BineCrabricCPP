#pragma once

// Real WGL OpenGL context bound to the existing Win32 client window.
//
// Beta 1.7.3 uses fixed-function immediate-mode GL, so a legacy context
// (opengl32.dll, no extensions loader needed) is exactly the right target.
// This wraps pixel-format selection + context creation + buffer swap.

#ifdef _WIN32

#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <stdexcept>

#include "net/minecraft/client/gl/GlExtensions.hpp"

namespace net::minecraft::client::gl3d {

class GlContext {
public:
    GlContext() = default;

    GlContext(const GlContext&) = delete;
    GlContext& operator=(const GlContext&) = delete;

    ~GlContext()
    {
        if (glrc_ != nullptr) {
            ::wglMakeCurrent(nullptr, nullptr);
            ::wglDeleteContext(glrc_);
        }
        if (hdc_ != nullptr && hwnd_ != nullptr) {
            ::ReleaseDC(hwnd_, hdc_);
        }
    }

    // Creates and makes-current a GL context on the given window. Idempotent.
    void create(HWND hwnd)
    {
        if (glrc_ != nullptr) {
            return;
        }
        hwnd_ = hwnd;
        hdc_ = ::GetDC(hwnd);
        if (hdc_ == nullptr) {
            throw std::runtime_error("gl3d: GetDC failed");
        }

        PIXELFORMATDESCRIPTOR pfd {};
        pfd.nSize = sizeof(pfd);
        pfd.nVersion = 1;
        pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
        pfd.iPixelType = PFD_TYPE_RGBA;
        pfd.cColorBits = 32;
        pfd.cDepthBits = 24;
        pfd.cStencilBits = 8;
        pfd.iLayerType = PFD_MAIN_PLANE;

        const int format = ::ChoosePixelFormat(hdc_, &pfd);
        if (format == 0 || !::SetPixelFormat(hdc_, format, &pfd)) {
            throw std::runtime_error("gl3d: pixel format selection failed");
        }

        glrc_ = ::wglCreateContext(hdc_);
        if (glrc_ == nullptr) {
            throw std::runtime_error("gl3d: wglCreateContext failed");
        }
        ::wglMakeCurrent(hdc_, glrc_);
        net::minecraft::client::gl::GlExtensions::setSwapInterval(0);
    }

    void makeCurrent() const
    {
        if (glrc_ != nullptr) {
            ::wglMakeCurrent(hdc_, glrc_);
        }
    }

    void swap() const
    {
        if (hdc_ != nullptr) {
            ::SwapBuffers(hdc_);
        }
    }

    [[nodiscard]] bool valid() const noexcept { return glrc_ != nullptr; }

private:
    HWND hwnd_ = nullptr;
    HDC hdc_ = nullptr;
    HGLRC glrc_ = nullptr;
};

} // namespace net::minecraft::client::gl3d

#endif // _WIN32
