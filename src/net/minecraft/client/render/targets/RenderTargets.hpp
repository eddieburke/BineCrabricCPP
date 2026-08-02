#pragma once
#include <array>
#include <string>
#include <unordered_map>
#include <vector>
#include "net/minecraft/client/gl/GlFramebuffer.hpp"
#include "net/minecraft/client/gl/GlResource.hpp"
namespace net::minecraft::client::render {
inline constexpr int kMaxColorAttachments = 8;
// Mirrors InternalTextureFormat.java (58 names incl. the RGBA alias, 57 distinct
// formats); "RGBA" parses as Rgba8 (GL_RGBA8) in Loader.cpp.
// https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/gl/texture/InternalTextureFormat.java
enum class ColorFormat {
 Rgba8, R8, R16, R16F, R32F, Rg8, Rg16, Rg16F, Rg32F, Rgb8, Rgb16, Rgb16F, Rgb32F,
 R11G11B10F, Rgb10A2, Rgb565, Rgb5A1, Rgba16, Rgba16F, Rgba32F,
 R8Snorm, Rg8Snorm, Rgb8Snorm, Rgba8Snorm, R16Snorm, Rg16Snorm, Rgb16Snorm, Rgba16Snorm,
 Rgba2, Rgba4, R3G3B2, Rgb10A2Ui, Rgb9E5,
 R8Ui, R16Ui, R32Ui, Rg8Ui, Rg16Ui, Rg32Ui, Rgb8Ui, Rgb16Ui, Rgb32Ui, Rgba8Ui, Rgba16Ui,
 Rgba32Ui,
 R8I, R16I, R32I, Rg8I, Rg16I, Rg32I, Rgb8I, Rgb16I, Rgb32I, Rgba8I, Rgba16I, Rgba32I
};
[[nodiscard]] inline bool isIntegerColorFormat(ColorFormat format) {
 return format == ColorFormat::R8Ui || format == ColorFormat::R16Ui || format == ColorFormat::R32Ui ||
        format == ColorFormat::Rg8Ui || format == ColorFormat::Rg16Ui || format == ColorFormat::Rg32Ui ||
        format == ColorFormat::Rgb8Ui || format == ColorFormat::Rgb16Ui || format == ColorFormat::Rgb32Ui ||
        format == ColorFormat::Rgba8Ui || format == ColorFormat::Rgba16Ui || format == ColorFormat::Rgba32Ui ||
        format == ColorFormat::Rgb10A2Ui ||
        format == ColorFormat::R8I || format == ColorFormat::R16I || format == ColorFormat::R32I ||
        format == ColorFormat::Rg8I || format == ColorFormat::Rg16I || format == ColorFormat::Rg32I ||
        format == ColorFormat::Rgb8I || format == ColorFormat::Rgb16I || format == ColorFormat::Rgb32I ||
        format == ColorFormat::Rgba8I || format == ColorFormat::Rgba16I || format == ColorFormat::Rgba32I;
}
[[nodiscard]] inline bool isSignedIntegerColorFormat(ColorFormat format) {
 return format == ColorFormat::R8I || format == ColorFormat::R16I || format == ColorFormat::R32I ||
        format == ColorFormat::Rg8I || format == ColorFormat::Rg16I || format == ColorFormat::Rg32I ||
        format == ColorFormat::Rgb8I || format == ColorFormat::Rgb16I || format == ColorFormat::Rgb32I ||
        format == ColorFormat::Rgba8I || format == ColorFormat::Rgba16I || format == ColorFormat::Rgba32I;
}
struct PackDefinition;
class ColorTargets {
 public:
  static constexpr int kMaxColortex = 32;
  void destroy();
  [[nodiscard]] bool valid() const noexcept { return valid_; }
  [[nodiscard]] int width() const noexcept { return width_; }
  [[nodiscard]] int height() const noexcept { return height_; }
  [[nodiscard]] int colorCount() const noexcept { return static_cast<int>(slots_.size()); }
  [[nodiscard]] unsigned int depthTexture() const noexcept { return depth_.handle(); }
  bool ensure(int width, int height, const std::vector<ColorFormat>& formats);
  bool ensureNamed(const std::string& name, int width, int height, ColorFormat format);
  void bindGbuffers();
  void endGbuffers();
  bool bindWrite(const std::vector<std::string>& outputs);
  void clearColors(const std::vector<bool>& enabled, const std::vector<std::array<float, 4>>& colors);
  // Java clears both main and alt sides of every buffer every frame (ClearPassCreator);
  // buffers reallocated this frame are additionally cleared even when their clear flag
  // is false, mirroring RenderTargets.fullClearRequired / onFullClear.
  void clearNamedColors(const std::string& name, const std::array<float, 4>& color);
  [[nodiscard]] bool fullClearRequired() const noexcept { return fullClearPending_; }
  // Restores the non-mipped min/mag filter on every target texture (both sides), mirroring
  // FinalPassRenderer.resetRenderTarget's turnOffMips at end of frame.
  void resetMipmaps();
  [[nodiscard]] unsigned int readTexture(int index) const noexcept;
  [[nodiscard]] unsigned int writeTexture(int index) const noexcept;
 [[nodiscard]] unsigned int readTexture(const std::string& name) const;
 [[nodiscard]] unsigned int writeTexture(const std::string& name) const;
 [[nodiscard]] ColorFormat formatOf(const std::string& name) const;
  // Java exposes colortex0-3 only to fullscreen passes; world/GBUFFERS and shadow
  // programs start at colortex4 (IrisSamplers.startIndex). Named buffers are a C++
  // extension and stay visible to every program kind.
  // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/samplers/IrisSamplers.java
  void fillReadSamplers(std::unordered_map<std::string, int>& textures, bool fullscreenPass) const;
   void fillImageBindings(std::unordered_map<std::string, int>& images) const;
    void prepareWrite(const std::string& name);
    void flip(const std::string& name);
   void flipIfEnabled(const PackDefinition& definition, const std::string& passName,
                      const std::string& bufferName);
  // Per-pass flip plan mirroring CompositeRenderer.java:165-187: buffers the pass
  // writes flip unless the pass's explicit directive says false, and every explicit
  // true directive flips its buffer even when the pass does not write it.
  void applyPassFlips(const PackDefinition& definition, const std::string& passName,
                      const std::vector<std::string>& outputs);
  // True when the pass declares an explicit false for this buffer (blocks the default
  // flip of a written buffer); an explicit true or no directive returns false.
  [[nodiscard]] static bool flipExplicitlyBlocked(const PackDefinition& definition,
                                                  const std::string& passName, const std::string& bufferName);
  // Every buffer the pass declares an explicit true for, written or not.
  [[nodiscard]] static std::vector<std::string> explicitTrueFlips(const PackDefinition& definition,
                                                                  const std::string& passName);
  // IrisSamplers.java:55: startIndex = isFullscreenPass ? 0 : 4.
  [[nodiscard]] static int renderTargetSamplerStartIndex(bool fullscreenPass);
   void applyPreFlips(const PackDefinition& definition, const std::string& stage);
 private:
   struct Slot {
    std::array<gl::GlTexture, 2> tex;
    int main = 0;
    ColorFormat format = ColorFormat::Rgba8;
    int width = 0;
    int height = 0;
    bool scaled = false;
   };
  bool allocateSlot(Slot& slot, int width, int height, ColorFormat format);
  void freeSlot(Slot& slot);
  void rebuildGbufferFbo();
  void clearAttachmentSet(const std::vector<unsigned int>& textures,
                          const std::vector<ColorFormat>& formats,
                          const std::vector<bool>& enabled,
                          const std::vector<std::array<float, 4>>& colors);
  void resetSlotFilters(Slot& slot);
  [[nodiscard]] Slot* findSlot(const std::string& name);
  [[nodiscard]] const Slot* findSlot(const std::string& name) const;
  [[nodiscard]] static int colortexIndex(const std::string& name);
  // FBO chain in parity with Java's GlFramebuffer usage (RenderTargets.java): the
  // gbuffer framebuffer holds every colortex slot + the depth texture; the write FBO
  // is rebound per pass/clear; the read/draw pair drives prepareWrite blits.
  gl::GlFramebuffer gbufferFbo_;
  gl::GlFramebuffer writeFbo_;
  gl::GlFramebuffer copyReadFbo_;
  gl::GlFramebuffer copyDrawFbo_;
  gl::GlTexture depth_;
  int width_ = 0;
  int height_ = 0;
  bool valid_ = false;
  bool gbufferActive_ = false;
  bool gbufferFboDirty_ = true;
  bool fullClearPending_ = true;
  int savedViewport_[4]{};
  int previousBoundFbo_ = 0;
  std::vector<Slot> slots_;
  std::unordered_map<std::string, Slot> named_;
};
} // namespace net::minecraft::client::render
