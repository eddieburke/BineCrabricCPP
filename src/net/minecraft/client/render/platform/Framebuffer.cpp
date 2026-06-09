#include "net/minecraft/client/render/platform/Framebuffer.hpp"

#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/gl/GlExtensions.hpp"

namespace net::minecraft::client::render::platform {

Framebuffer::~Framebuffer()
{
    destroy();
}

void Framebuffer::destroy() noexcept
{
    releaseAttachments();
    width_ = 0;
    height_ = 0;
}

void Framebuffer::releaseAttachments() noexcept
{
    if (depthRenderbuffer_ != 0) {
        gl::GlExtensions::deleteRenderbuffers(1, &depthRenderbuffer_);
        depthRenderbuffer_ = 0;
    }
    if (colorTexture_ != 0) {
        gl::GL11::glDeleteTextures(1, &colorTexture_);
        colorTexture_ = 0;
    }
    if (fbo_ != 0) {
        gl::GlExtensions::deleteFramebuffers(1, &fbo_);
        fbo_ = 0;
    }
}

void Framebuffer::resize(int width, int height)
{
    if (width <= 0) {
        width = 1;
    }
    if (height <= 0) {
        height = 1;
    }
    if (width_ == width && height_ == height && valid()) {
        return;
    }

    releaseAttachments();
    width_ = width;
    height_ = height;

    gl::GlExtensions::ensureLoaded();
    if (!gl::GlExtensions::isFboAvailable()) {
        return;
    }

    createAttachments(width, height);
}

void Framebuffer::createAttachments(int width, int height)
{
    constexpr int kTexture2D = 0x0DE1;
    constexpr int kRgba = 0x1908;
    constexpr int kUnsignedByte = 0x1401;
    constexpr int kNearest = 0x2600;
    constexpr int kClamp = 0x2900;
    constexpr int kTextureMinFilter = 0x2801;
    constexpr int kTextureMagFilter = 0x2800;
    constexpr int kTextureWrapS = 0x2802;
    constexpr int kTextureWrapT = 0x2803;

    gl::GlExtensions::genFramebuffers(1, &fbo_);
    gl::GlExtensions::bindFramebuffer(gl::GlExtensions::GL_FRAMEBUFFER, fbo_);

    gl::GL11::glGenTextures(1, &colorTexture_);
    gl::GL11::glBindTexture(kTexture2D, static_cast<int>(colorTexture_));
    gl::GL11::glTexParameteri(kTexture2D, kTextureMinFilter, kNearest);
    gl::GL11::glTexParameteri(kTexture2D, kTextureMagFilter, kNearest);
    gl::GL11::glTexParameteri(kTexture2D, kTextureWrapS, kClamp);
    gl::GL11::glTexParameteri(kTexture2D, kTextureWrapT, kClamp);
    // GL 1.x: use GL_RGBA as internal format, not GL_RGBA8.
    gl::GL11::glTexImage2D(kTexture2D, 0, kRgba, width, height, 0, kRgba, kUnsignedByte, nullptr);
    gl::GlExtensions::framebufferTexture2D(gl::GlExtensions::GL_FRAMEBUFFER, gl::GlExtensions::GL_COLOR_ATTACHMENT0, kTexture2D, colorTexture_, 0);

    gl::GlExtensions::genRenderbuffers(1, &depthRenderbuffer_);
    gl::GlExtensions::bindRenderbuffer(gl::GlExtensions::GL_RENDERBUFFER, depthRenderbuffer_);
    gl::GlExtensions::renderbufferStorage(gl::GlExtensions::GL_RENDERBUFFER, gl::GlExtensions::GL_DEPTH_COMPONENT24, width, height);
    gl::GlExtensions::framebufferRenderbuffer(gl::GlExtensions::GL_FRAMEBUFFER, gl::GlExtensions::GL_DEPTH_ATTACHMENT, gl::GlExtensions::GL_RENDERBUFFER,
        depthRenderbuffer_);

    const unsigned int status = gl::GlExtensions::checkFramebufferStatus(gl::GlExtensions::GL_FRAMEBUFFER);
    gl::GlExtensions::bindFramebuffer(gl::GlExtensions::GL_FRAMEBUFFER, 0);
    gl::GL11::glBindTexture(kTexture2D, 0);

    if (status != gl::GlExtensions::GL_FRAMEBUFFER_COMPLETE) {
        releaseAttachments();
        width_ = 0;
        height_ = 0;
    }
}

void Framebuffer::bind() const noexcept
{
    if (!valid()) {
        return;
    }
    gl::GlExtensions::bindFramebuffer(gl::GlExtensions::GL_FRAMEBUFFER, fbo_);
    gl::GL11::glDrawBuffer(static_cast<int>(gl::GlExtensions::GL_COLOR_ATTACHMENT0));
    gl::GL11::glReadBuffer(static_cast<int>(gl::GlExtensions::GL_COLOR_ATTACHMENT0));
}

void Framebuffer::unbind() noexcept
{
    gl::GlExtensions::bindFramebuffer(gl::GlExtensions::GL_FRAMEBUFFER, 0);
#ifdef MINECRAFT_GL_REAL
    gl::GL11::glDrawBuffer(gl::GL11::GL_BACK);
    gl::GL11::glReadBuffer(gl::GL11::GL_BACK);
#else
    gl::GL11::glDrawBuffer(0x0405);
    gl::GL11::glReadBuffer(0x0405);
#endif
}

} // namespace net::minecraft::client::render::platform
