#pragma once
#include <array>
#include <string>
#include <unordered_map>
#include <vector>
#include "net/minecraft/client/gl/GlFramebuffer.hpp"
#include "net/minecraft/client/gl/GlResource.hpp"
namespace net::minecraft::client::render {
inline constexpr int kMaxColorAttachments = 8;
// https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/gl/texture/InternalTextureFormat.java
enum class ColorFormat {
 Rgba8,
 R8,
 R16,
 R16F,
 R32F,
 Rg8,
 Rg16,
 Rg16F,
 Rg32F,
 Rgb8,
 Rgb16,
 Rgb16F,
 Rgb32F,
 R11G11B10F,
 Rgb10A2,
 Rgb565,
 Rgb5A1,
 Rgba16,
 Rgba16F,
 Rgba32F,
 R8Snorm,
 Rg8Snorm,
 Rgb8Snorm,
 Rgba8Snorm,
 R16Snorm,
 Rg16Snorm,
 Rgb16Snorm,
 Rgba16Snorm,
 Rgba2,
 Rgba4,
 R3G3B2,
 Rgb10A2Ui,
 Rgb9E5,
 R8Ui,
 R16Ui,
 R32Ui,
 Rg8Ui,
 Rg16Ui,
 Rg32Ui,
 Rgb8Ui,
 Rgb16Ui,
 Rgb32Ui,
 Rgba8Ui,
 Rgba16Ui,
 Rgba32Ui,
 R8I,
 R16I,
 R32I,
 Rg8I,
 Rg16I,
 Rg32I,
 Rgb8I,
 Rgb16I,
 Rgb32I,
 Rgba8I,
 Rgba16I,
 Rgba32I
};
// The GL triple a ColorFormat maps to. This is the ONE description of a format.
// isIntegerColorFormat/isSignedIntegerColorFormat used to be hand-written `||` chains
// over 25 enumerators living here, while the table below lived in ColorTargets.cpp —
// two independently maintained answers to the same question, and a format added to one
// and not the other produces a target that samples as garbage rather than failing.
struct GlFormat {
 int internal;
 unsigned format;
 unsigned type;
};
// A target is an integer target exactly when its pixel format is one of the four
// *_INTEGER enums. Sampling one needs usampler/isampler and a uvec4/ivec4 fragment
// output — there is no implicit conversion, so getting this wrong is silent garbage.
inline constexpr unsigned kGlRedInteger = 0x8D94;
inline constexpr unsigned kGlRgInteger = 0x8228;
inline constexpr unsigned kGlRgbInteger = 0x8D98;
inline constexpr unsigned kGlRgbaInteger = 0x8D99;
// Signed component types (GL_BYTE / GL_SHORT / GL_INT).
inline constexpr unsigned kGlByte = 0x1400;
inline constexpr unsigned kGlShort = 0x1402;
inline constexpr unsigned kGlInt = 0x1404;
[[nodiscard]] constexpr GlFormat glFormat(ColorFormat format) {
 switch(format) {
 case ColorFormat::R8: return {0x8229, 0x1903, 0x1401};
 case ColorFormat::R16: return {0x822A, 0x1903, 0x1403};
 case ColorFormat::R16F: return {0x822D, 0x1903, 0x1406};
 case ColorFormat::R32F: return {0x822E, 0x1903, 0x1406};
 case ColorFormat::Rg8: return {0x822B, 0x8227, 0x1401};
 case ColorFormat::Rg16: return {0x822C, 0x8227, 0x1403};
 case ColorFormat::Rg16F: return {0x822F, 0x8227, 0x1406};
 case ColorFormat::Rg32F: return {0x8230, 0x8227, 0x1406};
 case ColorFormat::Rgb8: return {0x8051, 0x1907, 0x1401};
 case ColorFormat::Rgb16: return {0x8054, 0x1907, 0x1403};
 case ColorFormat::Rgb16F: return {0x881B, 0x1907, 0x1406};
 case ColorFormat::Rgb32F: return {0x8815, 0x1907, 0x1406};
 case ColorFormat::R11G11B10F: return {0x8C3A, 0x1907, 0x1406};
 case ColorFormat::Rgb10A2: return {0x8059, 0x1908, 0x1405};
 case ColorFormat::Rgb565: return {0x8D62, 0x1907, 0x1401};
 case ColorFormat::Rgb5A1: return {0x8057, 0x1908, 0x1401};
 case ColorFormat::Rgba16: return {0x805B, 0x1908, 0x1403};
 case ColorFormat::Rgba16F: return {0x881A, 0x1908, 0x1406};
 case ColorFormat::Rgba32F: return {0x8814, 0x1908, 0x1406};
 case ColorFormat::R8Snorm: return {0x8F94, 0x1903, 0x1400};
 case ColorFormat::Rg8Snorm: return {0x8F95, 0x8227, 0x1400};
 case ColorFormat::Rgb8Snorm: return {0x8F96, 0x1907, 0x1400};
 case ColorFormat::Rgba8Snorm: return {0x8F97, 0x1908, 0x1400};
 case ColorFormat::R16Snorm: return {0x8F98, 0x1903, 0x1402};
 case ColorFormat::Rg16Snorm: return {0x8F99, 0x8227, 0x1402};
 case ColorFormat::Rgb16Snorm: return {0x8F9A, 0x1907, 0x1402};
 case ColorFormat::Rgba16Snorm: return {0x8F9B, 0x1908, 0x1402};
 case ColorFormat::Rgba2: return {0x8056, 0x1908, 0x1401};
 case ColorFormat::Rgba4: return {0x805A, 0x1908, 0x1401};
 case ColorFormat::R3G3B2: return {0x8050, 0x1907, 0x1401};
 case ColorFormat::Rgb10A2Ui: return {0x906F, kGlRgbaInteger, 0x1405};
 case ColorFormat::Rgb9E5: return {0x8C3D, 0x1907, 0x1401};
 case ColorFormat::R8Ui: return {0x8232, kGlRedInteger, 0x1401};
 case ColorFormat::R16Ui: return {0x8234, kGlRedInteger, 0x1403};
 case ColorFormat::R32Ui: return {0x8236, kGlRedInteger, 0x1405};
 case ColorFormat::Rg8Ui: return {0x8238, kGlRgInteger, 0x1401};
 case ColorFormat::Rg16Ui: return {0x823A, kGlRgInteger, 0x1403};
 case ColorFormat::Rg32Ui: return {0x823C, kGlRgInteger, 0x1405};
 case ColorFormat::Rgb8Ui: return {0x8D7D, kGlRgbInteger, 0x1401};
 case ColorFormat::Rgb16Ui: return {0x8D77, kGlRgbInteger, 0x1403};
 case ColorFormat::Rgb32Ui: return {0x8D71, kGlRgbInteger, 0x1405};
 case ColorFormat::Rgba8Ui: return {0x8D7C, kGlRgbaInteger, 0x1401};
 case ColorFormat::Rgba16Ui: return {0x8D76, kGlRgbaInteger, 0x1403};
 case ColorFormat::Rgba32Ui: return {0x8D70, kGlRgbaInteger, 0x1405};
 case ColorFormat::R8I: return {0x8231, kGlRedInteger, kGlByte};
 case ColorFormat::R16I: return {0x8233, kGlRedInteger, kGlShort};
 case ColorFormat::R32I: return {0x8235, kGlRedInteger, kGlInt};
 case ColorFormat::Rg8I: return {0x8237, kGlRgInteger, kGlByte};
 case ColorFormat::Rg16I: return {0x8239, kGlRgInteger, kGlShort};
 case ColorFormat::Rg32I: return {0x823B, kGlRgInteger, kGlInt};
 case ColorFormat::Rgb8I: return {0x8D8F, kGlRgbInteger, kGlByte};
 case ColorFormat::Rgb16I: return {0x8D89, kGlRgbInteger, kGlShort};
 case ColorFormat::Rgb32I: return {0x8D83, kGlRgbInteger, kGlInt};
 case ColorFormat::Rgba8I: return {0x8D8E, kGlRgbaInteger, kGlByte};
 case ColorFormat::Rgba16I: return {0x8D88, kGlRgbaInteger, kGlShort};
 case ColorFormat::Rgba32I: return {0x8D82, kGlRgbaInteger, kGlInt};
 default: return {0x8058, 0x1908, 0x1401};
 }
}
[[nodiscard]] constexpr bool isIntegerColorFormat(ColorFormat format) {
 const unsigned pixelFormat = glFormat(format).format;
 return pixelFormat == kGlRedInteger || pixelFormat == kGlRgInteger ||
        pixelFormat == kGlRgbInteger || pixelFormat == kGlRgbaInteger;
}
[[nodiscard]] constexpr bool isSignedIntegerColorFormat(ColorFormat format) {
 if(!isIntegerColorFormat(format)) {
  return false;
 }
 const unsigned componentType = glFormat(format).type;
 return componentType == kGlByte || componentType == kGlShort || componentType == kGlInt;
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
 void clearNamedColors(const std::string& name, const std::array<float, 4>& color);
 [[nodiscard]] bool fullClearRequired() const noexcept { return fullClearPending_; }
 void resetMipmaps();
 [[nodiscard]] unsigned int readTexture(int index) const noexcept;
 [[nodiscard]] unsigned int writeTexture(int index) const noexcept;
 [[nodiscard]] unsigned int readTexture(const std::string& name) const;
 [[nodiscard]] unsigned int writeTexture(const std::string& name) const;
 [[nodiscard]] ColorFormat formatOf(const std::string& name) const;
 // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/samplers/IrisSamplers.java
 void fillReadSamplers(std::unordered_map<std::string, int>& textures, bool fullscreenPass) const;
 void fillImageBindings(std::unordered_map<std::string, int>& images) const;
 void prepareWrite(const std::string& name);
 void flip(const std::string& name);
 void flipIfEnabled(const PackDefinition& definition, const std::string& passName,
                    const std::string& bufferName);
 // CompositeRenderer.java:165-187: buffers the pass writes flip unless the pack
 // blocks it; buffers declared with an explicit true flip anyway.
 void applyPassFlips(const PackDefinition& definition, const std::string& passName,
                     const std::vector<std::string>& outputs);
 [[nodiscard]] static bool flipExplicitlyBlocked(const PackDefinition& definition,
                                                 const std::string& passName, const std::string& bufferName);
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
  // The ping-pong convention lives here and nowhere else. It used to be spelled
  // out as `tex[main]` / `tex[1 - main]` at a dozen call sites; a single one of
  // them written the wrong way round silently samples the buffer being written,
  // which reads as one frame of lag rather than as a bug.
  [[nodiscard]] unsigned int readHandle() const noexcept { return tex[main].handle(); }
  [[nodiscard]] unsigned int writeHandle() const noexcept { return tex[1 - main].handle(); }
  [[nodiscard]] bool allocated() const noexcept { return tex[0] && tex[1]; }
  void flipBuffers() noexcept { main = 1 - main; }
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
