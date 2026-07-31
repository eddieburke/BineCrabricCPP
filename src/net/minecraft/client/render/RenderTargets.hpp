#pragma once
#include <array>
#include <string>
#include <unordered_map>
#include <vector>
namespace net::minecraft::client::render {
inline constexpr int kMaxColorAttachments = 8;
enum class ColorFormat {
 Rgba8, R8, R16, R16F, R32F, Rg8, Rg16, Rg16F, Rg32F, Rgb8, Rgb16, Rgb16F, Rgb32F,
 R11G11B10F, Rgb10A2, Rgb565, Rgb5A1, Rgba16, Rgba16F, Rgba32F,
 R8Ui, R16Ui, R32Ui, Rg8Ui, Rg16Ui, Rg32Ui, Rgba8Ui, Rgba16Ui, Rgba32Ui,
 R8I, R16I, R32I, Rg8I, Rg16I, Rg32I, Rgba8I, Rgba16I, Rgba32I
};
[[nodiscard]] inline bool isIntegerColorFormat(ColorFormat format) {
 return format == ColorFormat::R8Ui || format == ColorFormat::R16Ui || format == ColorFormat::R32Ui ||
        format == ColorFormat::Rg8Ui || format == ColorFormat::Rg16Ui || format == ColorFormat::Rg32Ui ||
        format == ColorFormat::Rgba8Ui || format == ColorFormat::Rgba16Ui || format == ColorFormat::Rgba32Ui ||
        format == ColorFormat::R8I || format == ColorFormat::R16I || format == ColorFormat::R32I ||
        format == ColorFormat::Rg8I || format == ColorFormat::Rg16I || format == ColorFormat::Rg32I ||
        format == ColorFormat::Rgba8I || format == ColorFormat::Rgba16I || format == ColorFormat::Rgba32I;
}
[[nodiscard]] inline bool isSignedIntegerColorFormat(ColorFormat format) {
 return format == ColorFormat::R8I || format == ColorFormat::R16I || format == ColorFormat::R32I ||
        format == ColorFormat::Rg8I || format == ColorFormat::Rg16I || format == ColorFormat::Rg32I ||
        format == ColorFormat::Rgba8I || format == ColorFormat::Rgba16I || format == ColorFormat::Rgba32I;
}
namespace shaderpack { struct ShaderPackDefinition; }
class ColorTargets {
 public:
  static constexpr int kMaxColortex = 32;
  void destroy();
  [[nodiscard]] bool valid() const noexcept { return valid_; }
  [[nodiscard]] int width() const noexcept { return width_; }
  [[nodiscard]] int height() const noexcept { return height_; }
  [[nodiscard]] int colorCount() const noexcept { return static_cast<int>(slots_.size()); }
  [[nodiscard]] unsigned int depthTexture() const noexcept { return depth_; }
  [[nodiscard]] unsigned int colorTexture(int index) const noexcept { return readTexture(index); }
  bool ensure(int width, int height, const std::vector<ColorFormat>& formats);
  bool ensureNamed(const std::string& name, int width, int height, ColorFormat format);
  void bindGbuffers();
  void endGbuffers();
  bool bindWrite(const std::vector<std::string>& outputs);
  void clearColors(const std::vector<bool>& enabled, const std::vector<std::array<float, 4>>& colors);
  [[nodiscard]] unsigned int readTexture(int index) const noexcept;
  [[nodiscard]] unsigned int writeTexture(int index) const noexcept;
 [[nodiscard]] unsigned int readTexture(const std::string& name) const;
 [[nodiscard]] unsigned int writeTexture(const std::string& name) const;
 [[nodiscard]] ColorFormat formatOf(const std::string& name) const;
 void fillReadSamplers(std::unordered_map<std::string, int>& textures) const;
  void fillImageBindings(std::unordered_map<std::string, int>& images) const;
  void prepareWrite(const std::string& name);
  void prepareWrites(const std::vector<std::string>& names);
  void flip(const std::string& name);
  void flipIfEnabled(const shaderpack::ShaderPackDefinition& definition, const std::string& passName,
                     const std::string& bufferName);
  void applyPreFlips(const shaderpack::ShaderPackDefinition& definition, const std::string& stage);
 private:
  struct Slot {
   unsigned int tex[2] = {0, 0};
   int main = 0;
   ColorFormat format = ColorFormat::Rgba8;
   int width = 0;
   int height = 0;
  };
  bool allocateSlot(Slot& slot, int width, int height, ColorFormat format);
  void freeSlot(Slot& slot);
  void rebuildGbufferFbo();
  [[nodiscard]] Slot* findSlot(const std::string& name);
  [[nodiscard]] const Slot* findSlot(const std::string& name) const;
  [[nodiscard]] static int colortexIndex(const std::string& name);
  unsigned int gbufferFbo_ = 0;
  unsigned int writeFbo_ = 0;
  unsigned int copyReadFbo_ = 0;
  unsigned int copyDrawFbo_ = 0;
  unsigned int depth_ = 0;
  int width_ = 0;
  int height_ = 0;
  bool valid_ = false;
  bool gbufferActive_ = false;
  bool gbufferFboDirty_ = true;
  int savedViewport_[4]{};
  int previousBoundFbo_ = 0;
  std::vector<Slot> slots_;
  std::unordered_map<std::string, Slot> named_;
};
} // namespace net::minecraft::client::render
