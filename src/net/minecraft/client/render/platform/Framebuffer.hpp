#pragma once

namespace net::minecraft::client::render::platform {

// Native OpenGL FBO wrapper: RGBA8 color texture + 24-bit depth renderbuffer.
class Framebuffer {
public:
    Framebuffer() = default;
    ~Framebuffer();

    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;

    void resize(int width, int height);
    void bind() const noexcept;
    static void unbind() noexcept;
    void destroy() noexcept;

    [[nodiscard]] unsigned int colorTexture() const noexcept { return colorTexture_; }
    [[nodiscard]] int width() const noexcept { return width_; }
    [[nodiscard]] int height() const noexcept { return height_; }
    [[nodiscard]] bool valid() const noexcept { return fbo_ != 0 && colorTexture_ != 0; }

private:
    void createAttachments(int width, int height);
    void releaseAttachments() noexcept;

    unsigned int fbo_ = 0;
    unsigned int colorTexture_ = 0;
    unsigned int depthRenderbuffer_ = 0;
    int width_ = 0;
    int height_ = 0;
};

} // namespace net::minecraft::client::render::platform
