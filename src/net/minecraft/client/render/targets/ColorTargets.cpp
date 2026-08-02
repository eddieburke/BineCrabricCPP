#include "net/minecraft/client/render/targets/RenderTargets.hpp"
#include "net/minecraft/client/render/shaderpack/Pack.hpp"
#include "net/minecraft/client/render/GlState.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/GlConstants.hpp"
#include "net/minecraft/client/gl/GlResource.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <string_view>
namespace net::minecraft::client::render {
namespace {
struct GlFormat {
 int internal;
 unsigned format;
 unsigned type;
};
GlFormat glFormat(ColorFormat format) {
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
  // Signed normalized 8/16-bit; GL 3.1 (GL_ARB_texture_snorm).
  case ColorFormat::R8Snorm: return {0x8F94, 0x1903, 0x1400};
  case ColorFormat::Rg8Snorm: return {0x8F95, 0x8227, 0x1400};
  case ColorFormat::Rgb8Snorm: return {0x8F96, 0x1907, 0x1400};
  case ColorFormat::Rgba8Snorm: return {0x8F97, 0x1908, 0x1400};
  case ColorFormat::R16Snorm: return {0x8F98, 0x1903, 0x1402};
  case ColorFormat::Rg16Snorm: return {0x8F99, 0x8227, 0x1402};
  case ColorFormat::Rgb16Snorm: return {0x8F9A, 0x1907, 0x1402};
  case ColorFormat::Rgba16Snorm: return {0x8F9B, 0x1908, 0x1402};
  // 2/4-bit normalized and 3-3-2.
  case ColorFormat::Rgba2: return {0x8056, 0x1908, 0x1401};
  case ColorFormat::Rgba4: return {0x805A, 0x1908, 0x1401};
  case ColorFormat::R3G3B2: return {0x8050, 0x1907, 0x1401};
  // 10-10-10-2 unsigned integer; GL 3.3 (GL_ARB_texture_rgb10_a2ui).
  case ColorFormat::Rgb10A2Ui: return {0x906F, 0x8D99, 0x1405};
  // Shared-exponent float; GL 3.0 (GL_ARB_texture_rgb9_e5).
  case ColorFormat::Rgb9E5: return {0x8C3D, 0x1907, 0x1401};
  case ColorFormat::R8Ui: return {0x8232, 0x8D94, 0x1401};
  case ColorFormat::R16Ui: return {0x8234, 0x8D94, 0x1403};
  case ColorFormat::R32Ui: return {0x8236, 0x8D94, 0x1405};
  case ColorFormat::Rg8Ui: return {0x8238, 0x8228, 0x1401};
  case ColorFormat::Rg16Ui: return {0x823A, 0x8228, 0x1403};
  case ColorFormat::Rg32Ui: return {0x823C, 0x8228, 0x1405};
  case ColorFormat::Rgb8Ui: return {0x8D7D, 0x8D98, 0x1401};
  case ColorFormat::Rgb16Ui: return {0x8D77, 0x8D98, 0x1403};
  case ColorFormat::Rgb32Ui: return {0x8D71, 0x8D98, 0x1405};
  case ColorFormat::Rgba8Ui: return {0x8D7C, 0x8D99, 0x1401};
  case ColorFormat::Rgba16Ui: return {0x8D76, 0x8D99, 0x1403};
  case ColorFormat::Rgba32Ui: return {0x8D70, 0x8D99, 0x1405};
  case ColorFormat::R8I: return {0x8231, 0x8D94, 0x1400};
  case ColorFormat::R16I: return {0x8233, 0x8D94, 0x1402};
  case ColorFormat::R32I: return {0x8235, 0x8D94, 0x1404};
  case ColorFormat::Rg8I: return {0x8237, 0x8228, 0x1400};
  case ColorFormat::Rg16I: return {0x8239, 0x8228, 0x1402};
  case ColorFormat::Rg32I: return {0x823B, 0x8228, 0x1404};
  case ColorFormat::Rgb8I: return {0x8D8F, 0x8D98, 0x1400};
  case ColorFormat::Rgb16I: return {0x8D89, 0x8D98, 0x1402};
  case ColorFormat::Rgb32I: return {0x8D83, 0x8D98, 0x1404};
  case ColorFormat::Rgba8I: return {0x8D8E, 0x8D99, 0x1400};
  case ColorFormat::Rgba16I: return {0x8D88, 0x8D99, 0x1402};
  case ColorFormat::Rgba32I: return {0x8D82, 0x8D99, 0x1404};
  default: return {0x8058, 0x1908, 0x1401};
 }
}
void setTexParams(bool filterLinear) {
 ::glTexParameteri(gl::cap::Texture2D, gl::tex::MinFilter,
                   filterLinear ? gl::filter::Linear : gl::filter::Nearest);
 ::glTexParameteri(gl::cap::Texture2D, gl::tex::MagFilter,
                   filterLinear ? gl::filter::Linear : gl::filter::Nearest);
 ::glTexParameteri(gl::cap::Texture2D, gl::tex::WrapS, gl::wrap::ClampToEdge);
 ::glTexParameteri(gl::cap::Texture2D, gl::tex::WrapT, gl::wrap::ClampToEdge);
}
} // namespace
void ColorTargets::destroy() {
 if(gbufferActive_) {
  endGbuffers();
 }
  gbufferFbo_.destroy();
  writeFbo_.destroy();
  copyReadFbo_.destroy();
  copyDrawFbo_.destroy();
  depth_.reset();
  for(Slot& slot : slots_) {
   freeSlot(slot);
  }
 slots_.clear();
 for(auto& [name, slot] : named_) {
  freeSlot(slot);
 }
 named_.clear();
 width_ = 0;
 height_ = 0;
 valid_ = false;
 gbufferFboDirty_ = true;
}
void ColorTargets::freeSlot(Slot& slot) {
 slot.tex[0].reset();
 slot.tex[1].reset();
 slot.main = 0;
 slot.width = 0;
 slot.height = 0;
}
bool ColorTargets::allocateSlot(Slot& slot, int width, int height, ColorFormat format) {
 if(slot.width == width && slot.height == height && slot.format == format && slot.tex[0] &&
    slot.tex[1]) {
  return true;
 }
 freeSlot(slot);
 const GlFormat spec = glFormat(format);
 // Integer colortex cannot use LINEAR filter (GL incomplete / undefined).
 const bool linear = !render::isIntegerColorFormat(format);
 for(int i = 0; i < 2; ++i) {
  const unsigned int h = core::genTexture();
  if(h == 0) {
   freeSlot(slot);
   return false;
  }
  slot.tex[i] = gl::GlTexture(h);
  core::bindTexture(gl::cap::Texture2D, static_cast<int>(h));
  ::glTexImage2D(gl::cap::Texture2D, 0, spec.internal, width, height, 0, spec.format, spec.type, nullptr);
  setTexParams(linear);
 }
 slot.format = format;
 slot.width = width;
 slot.height = height;
 slot.main = 0;
 return true;
}
int ColorTargets::colortexIndex(const std::string& name) {
 if(name.rfind("colortex", 0) != 0) {
  return -1;
 }
 char* end = nullptr;
 const long index = std::strtol(name.c_str() + 8, &end, 10);
 if(end == name.c_str() + 8 || *end != '\0' || index < 0 || index >= kMaxColortex) {
  return -1;
 }
 return static_cast<int>(index);
}
ColorTargets::Slot* ColorTargets::findSlot(const std::string& name) {
 const int index = colortexIndex(name);
 if(index >= 0 && index < colorCount()) {
  return &slots_[static_cast<std::size_t>(index)];
 }
 const auto found = named_.find(name);
 return found == named_.end() ? nullptr : &found->second;
}
const ColorTargets::Slot* ColorTargets::findSlot(const std::string& name) const {
 const int index = colortexIndex(name);
 if(index >= 0 && index < colorCount()) {
  return &slots_[static_cast<std::size_t>(index)];
 }
 const auto found = named_.find(name);
 return found == named_.end() ? nullptr : &found->second;
}
bool ColorTargets::ensure(int width, int height, const std::vector<ColorFormat>& formats) {
 if(!gl::GLCore::framebufferSupported || width <= 0 || height <= 0) {
  return false;
 }
 std::vector<ColorFormat> normalized = formats;
 if(normalized.empty()) {
  normalized.push_back(ColorFormat::Rgba8);
 }
 if(static_cast<int>(normalized.size()) > kMaxColortex) {
  normalized.resize(static_cast<std::size_t>(kMaxColortex));
 }
   const bool same = valid_ && width == width_ && height == height_ && slots_.size() == normalized.size() &&
                     [&] {
                      for(std::size_t i = 0; i < normalized.size(); ++i) {
                       if(slots_[i].format != normalized[i] || !slots_[i].tex[0] || !slots_[i].tex[1]) {
                        return false;
                       }
                       // Scaled slots are managed by ensureNamed(); only unscaled slots
                       // must match the scene size. Java per-target sizes live in the
                       // RenderTarget (PackRenderTargetDirectives scale override).
                       if(!slots_[i].scaled &&
                          (slots_[i].width != width || slots_[i].height != height)) {
                        return false;
                       }
                      }
                      return static_cast<bool>(depth_);
                     }();
  if(same) {
   return true;
  }
  destroy();
  slots_.resize(normalized.size());
  for(std::size_t i = 0; i < normalized.size(); ++i) {
   slots_[i].scaled = false;
   if(!allocateSlot(slots_[i], width, height, normalized[i])) {
    destroy();
    return false;
   }
  }
    depth_ = gl::GlTexture(core::genTexture());
    if(!depth_) {
     destroy();
     return false;
    }
    core::bindTexture(gl::cap::Texture2D, static_cast<int>(depth_.handle()));
   ::glTexImage2D(gl::cap::Texture2D, 0, gl::framebuffer::Depth24Stencil8, width, height, 0,
                  gl::pixel::DepthStencil, gl::pixel::UnsignedInt248, nullptr);
   setTexParams(false);
  width_ = width;
  height_ = height;
  valid_ = true;
  // Freshly allocated textures hold undefined data; mirror RenderTargets.fullClearRequired
  // so the next clear pass clears every buffer regardless of its clear flag.
  fullClearPending_ = true;
  gbufferFboDirty_ = true;
  rebuildGbufferFbo();
   return valid_ && gbufferFbo_.valid();
}
bool ColorTargets::ensureNamed(const std::string& name, int width, int height,
                                    ColorFormat format) {
  if(width <= 0 || height <= 0) {
   return false;
  }
  const int index = colortexIndex(name);
  if(index >= 0) {
   if(index >= colorCount()) {
    return false;
   }
   // Java keeps per-index RenderTarget sizes (textureScaleOverride); a scaled colortexN
   // stays in its slot so reads/writes/flips/clears keep working at the scaled size.
   Slot& slot = slots_[static_cast<std::size_t>(index)];
   slot.scaled = width != width_ || height != height_;
   if(!allocateSlot(slot, width, height, format)) {
    return false;
   }
   gbufferFboDirty_ = true;
   return true;
  }
  return allocateSlot(named_[name], width, height, format);
}
void ColorTargets::rebuildGbufferFbo() {
 if(!valid_ || slots_.empty()) {
  return;
 }
 // Java createGbufferFramebuffer: one FBO with every colortex on the main textures
 // plus the depth attachment (RenderTargets.java:309-321).
  gbufferFbo_.addDepthAttachment(depth_.handle());
  std::vector<int> drawBuffers;
  drawBuffers.reserve(slots_.size());
  for(std::size_t i = 0; i < slots_.size(); ++i) {
   gbufferFbo_.addColorAttachment(static_cast<int>(i), slots_[i].tex[slots_[i].main].handle());
  drawBuffers.push_back(static_cast<int>(i));
 }
  if(!gbufferFbo_.drawBuffers(drawBuffers)) {
  valid_ = false;
  return;
 }
   const unsigned status = gbufferFbo_.checkStatus();
   gl::GLCore::bindFramebuffer(static_cast<unsigned>(gl::framebuffer::Framebuffer), 0);
  if(status != static_cast<unsigned>(gl::framebuffer::Complete)) {
  valid_ = false;
  return;
 }
 gbufferFboDirty_ = false;
}
void ColorTargets::bindGbuffers() {
 if(!valid_) {
  return;
 }
 if(gbufferFboDirty_) {
  rebuildGbufferFbo();
 }
 if(!gbufferActive_) {
  ::glGetIntegerv(static_cast<unsigned>(gl::query::FramebufferBinding), &previousBoundFbo_);
  core::getCachedViewport(savedViewport_);
  gbufferActive_ = true;
 }
  gbufferFbo_.bind();
  core::viewport(0, 0, width_, height_);
}
void ColorTargets::endGbuffers() {
 if(!gbufferActive_) {
  return;
 }
 gl::GLCore::bindFramebuffer(static_cast<unsigned>(gl::framebuffer::Framebuffer),
                             static_cast<unsigned>(previousBoundFbo_));
 core::viewport(savedViewport_[0], savedViewport_[1], savedViewport_[2], savedViewport_[3]);
  gbufferActive_ = false;
}
bool ColorTargets::bindWrite(const std::vector<std::string>& outputs) {
 if(!valid_ || outputs.empty()) {
  return false;
 }
 std::vector<int> drawBuffers;
 drawBuffers.reserve(outputs.size());
 int attachW = 0;
 int attachH = 0;
 for(std::size_t i = 0; i < outputs.size(); ++i) {
   Slot* slot = findSlot(outputs[i]);
   if(slot == nullptr || !slot->tex[0]) {
    return false;
   }
   const unsigned int tex = slot->tex[1 - slot->main].handle();
  writeFbo_.addColorAttachment(static_cast<int>(i), tex);
  drawBuffers.push_back(static_cast<int>(i));
  attachW = slot->width;
  attachH = slot->height;
 }
  if(!writeFbo_.drawBuffers(drawBuffers)) {
   return false;
  }
 // Fullscreen passes have no depth attachment (Java createColorFramebuffer); the
 // shared write FBO must drop the depth state any prior binding left behind.
 writeFbo_.removeDepthAttachment();
  const unsigned status = writeFbo_.checkStatus();
  if(status != static_cast<unsigned>(gl::framebuffer::Complete)) {
   return false;
  }
 core::viewport(0, 0, attachW > 0 ? attachW : width_, attachH > 0 ? attachH : height_);
 return true;
}
void ColorTargets::clearAttachmentSet(const std::vector<unsigned int>& textures,
                                      const std::vector<ColorFormat>& formats,
                                      const std::vector<bool>& enabled,
                                      const std::vector<std::array<float, 4>>& colors) {
   if(textures.empty() || gl::GLCore::framebufferSupported == false) {
    return;
   }
   // GL_MAX_COLOR_ATTACHMENTS is at least 8; chunk like ClearPassCreator chunks clears
   // by GL_MAX_DRAW_BUFFERS.
   // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/targets/ClearPassCreator.java
   constexpr std::size_t kMaxAttachments = 8;
   for(std::size_t start = 0; start < textures.size(); start += kMaxAttachments) {
    const std::size_t count = std::min(kMaxAttachments, textures.size() - start);
    bool any = false;
    for(std::size_t i = 0; i < count; ++i) {
     if(start + i >= enabled.size() || enabled[start + i]) {
      any = true;
      break;
     }
    }
    if(!any) {
     continue;
    }
    std::vector<int> drawBuffers;
    drawBuffers.reserve(count);
    for(std::size_t i = 0; i < count; ++i) {
     writeFbo_.addColorAttachment(static_cast<int>(i), textures[start + i]);
     drawBuffers.push_back(static_cast<int>(i));
    }
    if(!writeFbo_.drawBuffers(drawBuffers)) {
     continue;
    }
    if(writeFbo_.checkStatus() != static_cast<unsigned>(gl::framebuffer::Complete)) {
     continue;
    }
    if(gl::GLCore::clearBufferfv == nullptr) {
     static constexpr std::array<float, 4> zero{};
     const auto& color = start < colors.size() ? colors[start] : zero;
     core::clearColor(color[0], color[1], color[2], color[3]);
     core::clear(gl::attrib::ColorBufferBit);
     continue;
    }
    for(std::size_t i = 0; i < count; ++i) {
     const std::size_t index = start + i;
     if(index >= enabled.size() || !enabled[index]) {
      continue;
     }
     static constexpr std::array<float, 4> zero{};
     const auto& color = index < colors.size() ? colors[index] : zero;
     const ColorFormat fmt = index < formats.size() ? formats[index] : ColorFormat::Rgba8;
     if(render::isIntegerColorFormat(fmt)) {
      if(render::isSignedIntegerColorFormat(fmt)) {
       if(gl::GLCore::clearBufferiv == nullptr) {
        continue;
       }
       const int values[4] = {static_cast<int>(color[0]), static_cast<int>(color[1]),
                              static_cast<int>(color[2]), static_cast<int>(color[3])};
       gl::GLCore::clearBufferiv(gl::framebuffer::Color, static_cast<int>(i), values);
      } else if(gl::GLCore::clearBufferuiv != nullptr) {
       const unsigned values[4] = {static_cast<unsigned>(color[0]), static_cast<unsigned>(color[1]),
                                   static_cast<unsigned>(color[2]), static_cast<unsigned>(color[3])};
       gl::GLCore::clearBufferuiv(gl::framebuffer::Color, static_cast<int>(i), values);
      }
      continue;
     }
     gl::GLCore::clearBufferfv(gl::framebuffer::Color, static_cast<int>(i), color.data());
    }
   }
  }
void ColorTargets::clearColors(const std::vector<bool>& enabled,
                                    const std::vector<std::array<float, 4>>& colors) {
  if(!valid_) {
   return;
  }
  const bool full = fullClearPending_;
  fullClearPending_ = false;
  // ClearPassCreator issues two passes per buffer group: one writing the alt textures,
  // one writing the main textures, so both sides hold the clear color regardless of the
  // current flip state.
  // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/targets/ClearPassCreator.java
  std::vector<unsigned int> mainTextures;
  std::vector<unsigned int> altTextures;
  std::vector<ColorFormat> formats;
  mainTextures.reserve(slots_.size());
  altTextures.reserve(slots_.size());
  formats.reserve(slots_.size());
  for(const Slot& slot : slots_) {
   mainTextures.push_back(slot.tex[slot.main].handle());
   altTextures.push_back(slot.tex[1 - slot.main].handle());
   formats.push_back(slot.format);
  }
  std::vector<bool> effective = enabled;
  if(full) {
   // Freshly allocated buffers hold undefined data; clear everything this frame even
   // when the pack cleared the flag (RenderTargets.fullClearRequired).
   effective.assign(slots_.size(), true);
  }
  int previousFbo = 0;
  ::glGetIntegerv(static_cast<unsigned>(gl::query::FramebufferBinding), &previousFbo);
  clearAttachmentSet(mainTextures, formats, effective, colors);
  clearAttachmentSet(altTextures, formats, effective, colors);
  gl::GLCore::bindFramebuffer(static_cast<unsigned>(gl::framebuffer::Framebuffer),
                              static_cast<unsigned>(previousFbo));
  // Leave the gbuffer framebuffer bound (rebuilding it if flips/resizes dirtied it) so
  // the frame's depth clear and world rendering attach to it.
  bindGbuffers();
 }
void ColorTargets::clearNamedColors(const std::string& name, const std::array<float, 4>& color) {
   Slot* slot = findSlot(name);
   if(slot == nullptr || !slot->tex[0] || !slot->tex[1]) {
    return;
   }
   int previousFbo = 0;
   ::glGetIntegerv(static_cast<unsigned>(gl::query::FramebufferBinding), &previousFbo);
   for(const gl::GlTexture& tex : slot->tex) {
    writeFbo_.addColorAttachment(0, tex.handle());
   if(!writeFbo_.drawBuffers(std::vector<int>{0})) {
    continue;
   }
   if(writeFbo_.checkStatus() != static_cast<unsigned>(gl::framebuffer::Complete)) {
    continue;
   }
   if(render::isIntegerColorFormat(slot->format)) {
    if(render::isSignedIntegerColorFormat(slot->format)) {
     if(gl::GLCore::clearBufferiv == nullptr) {
      continue;
     }
     const int values[4] = {static_cast<int>(color[0]), static_cast<int>(color[1]),
                            static_cast<int>(color[2]), static_cast<int>(color[3])};
     gl::GLCore::clearBufferiv(gl::framebuffer::Color, 0, values);
    } else if(gl::GLCore::clearBufferuiv != nullptr) {
     const unsigned values[4] = {static_cast<unsigned>(color[0]), static_cast<unsigned>(color[1]),
                                 static_cast<unsigned>(color[2]), static_cast<unsigned>(color[3])};
     gl::GLCore::clearBufferuiv(gl::framebuffer::Color, 0, values);
    }
    continue;
   }
   if(gl::GLCore::clearBufferfv == nullptr) {
    core::clearColor(color[0], color[1], color[2], color[3]);
    core::clear(gl::attrib::ColorBufferBit);
    continue;
   }
   gl::GLCore::clearBufferfv(gl::framebuffer::Color, 0, color.data());
  }
  gl::GLCore::bindFramebuffer(static_cast<unsigned>(gl::framebuffer::Framebuffer),
                              static_cast<unsigned>(previousFbo));
 }
unsigned int ColorTargets::readTexture(int index) const noexcept {
 if(index < 0 || index >= colorCount()) {
  return 0;
 }
  const Slot& slot = slots_[static_cast<std::size_t>(index)];
  return slot.tex[slot.main].handle();
}
unsigned int ColorTargets::writeTexture(int index) const noexcept {
 if(index < 0 || index >= colorCount()) {
  return 0;
 }
 const Slot& slot = slots_[static_cast<std::size_t>(index)];
 return slot.tex[1 - slot.main].handle();
}
unsigned int ColorTargets::readTexture(const std::string& name) const {
 const Slot* slot = findSlot(name);
 return slot == nullptr ? 0u : slot->tex[slot->main].handle();
}
unsigned int ColorTargets::writeTexture(const std::string& name) const {
 const Slot* slot = findSlot(name);
 return slot == nullptr ? 0u : slot->tex[1 - slot->main].handle();
}
ColorFormat ColorTargets::formatOf(const std::string& name) const {
 const Slot* slot = findSlot(name);
 return slot == nullptr ? ColorFormat::Rgba8 : slot->format;
}
void ColorTargets::fillReadSamplers(std::unordered_map<std::string, int>& textures, bool fullscreenPass) const {
  // colortex0-3 are only sampleable from fullscreen passes; world/GBUFFERS and shadow
  // programs start at colortex4. The legacy aliases (gcolor..gaux4) are derived from
  // these bindings by refreshTextureAliases and inherit the same restriction, while
  // depthtex0-2 are wired separately by the callers (addWorldDepthSamplers /
  // addCompositeSamplers) and stay available to every program kind.
  // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/samplers/IrisSamplers.java
  for(int i = renderTargetSamplerStartIndex(fullscreenPass); i < colorCount(); ++i) {
   textures["colortex" + std::to_string(i)] = static_cast<int>(readTexture(i));
  }
  for(const auto& [name, slot] : named_) {
   textures[name] = static_cast<int>(slot.tex[slot.main].handle());
  }
 }
 void ColorTargets::fillImageBindings(std::unordered_map<std::string, int>& images) const {
 // Image bindings (colorimgN) target the same textures a raster pass would attach
 // through bindWrite(): the write side (tex[1 - main]). prepareWrite() pre-copies the
 // read side into it so imageLoad sees the current content, and applyPassFlips()
 // toggles main afterward so the next pass reads the freshly written side.
 for(int i = 0; i < colorCount(); ++i) {
  images["colortex" + std::to_string(i)] = static_cast<int>(writeTexture(i));
 }
  for(const auto& [name, slot] : named_) {
   images[name] = static_cast<int>(slot.tex[slot.main].handle());
  }
 }
 void ColorTargets::prepareWrite(const std::string& name) {
  Slot* slot = findSlot(name);
  if(slot == nullptr || !slot->tex[0] || !slot->tex[1] || gl::GLCore::blitFramebuffer == nullptr) {
   return;
  }
  const unsigned int src = slot->tex[slot->main].handle();
  const unsigned int dst = slot->tex[1 - slot->main].handle();
  copyReadFbo_.addColorAttachment(0, src);
  copyReadFbo_.readBuffer(0);
  copyDrawFbo_.addColorAttachment(0, dst);
  copyDrawFbo_.drawBuffers(std::vector<int>{0});
  copyReadFbo_.bindAsReadBuffer();
  copyDrawFbo_.bindAsDrawBuffer();
 gl::GLCore::blitFramebuffer(0, 0, slot->width, slot->height, 0, 0, slot->width, slot->height,
                             gl::attrib::ColorBufferBit, gl::filter::Nearest);
  gl::GLCore::bindFramebuffer(static_cast<unsigned>(gl::framebuffer::ReadFramebuffer), 0);
  gl::GLCore::bindFramebuffer(static_cast<unsigned>(gl::framebuffer::DrawFramebuffer), 0);
}
void ColorTargets::resetSlotFilters(Slot& slot) {
 const int filter = render::isIntegerColorFormat(slot.format) ? gl::filter::Nearest : gl::filter::Linear;
 for(const gl::GlTexture& tex : slot.tex) {
  if(!tex) {
   continue;
  }
  core::bindTexture(gl::cap::Texture2D, static_cast<int>(tex.handle()));
  ::glTexParameteri(gl::cap::Texture2D, gl::tex::MinFilter, filter);
  ::glTexParameteri(gl::cap::Texture2D, gl::tex::MagFilter, filter);
 }
}
void ColorTargets::resetMipmaps() {
  // FinalPassRenderer resets mipmapping on every target at the end of the frame so the
  // next frame's sampling falls back to the non-mipped filter.
  // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/pipeline/FinalPassRenderer.java
  for(Slot& slot : slots_) {
   resetSlotFilters(slot);
  }
  for(auto& [name, slot] : named_) {
   (void)name;
   resetSlotFilters(slot);
  }
 }
void ColorTargets::flip(const std::string& name) {
 Slot* slot = findSlot(name);
 if(slot == nullptr) {
  return;
 }
 slot->main = 1 - slot->main;
 if(colortexIndex(name) >= 0) {
  gbufferFboDirty_ = true;
 }
}
void ColorTargets::flipIfEnabled(const PackDefinition& definition, const std::string& passName,
                                        const std::string& bufferName) {
  // Written-buffer flip: only an explicit false blocks it (CompositeRenderer.java:174).
  bool flip = true;
  if(const auto declared = definition.flips.find(passName + "." + bufferName);
     declared != definition.flips.end()) {
   flip = declared->second;
  }
  if(flip) {
   this->flip(bufferName);
  }
}
bool ColorTargets::flipExplicitlyBlocked(const PackDefinition& definition,
                                         const std::string& passName, const std::string& bufferName) {
  // Java compares with Boolean.FALSE so an explicit true falls through to the default
  // flip; only an explicit false blocks it (CompositeRenderer.java:174).
  const auto declared = definition.flips.find(passName + "." + bufferName);
  return declared != definition.flips.end() && !declared->second;
}
std::vector<std::string> ColorTargets::explicitTrueFlips(const PackDefinition& definition,
                                                         const std::string& passName) {
  // The explicitFlips sweep: every explicit true flips its buffer even when the pass
  // does not write it (CompositeRenderer.java:182-187). Pre-flip keys like
  // "<pass>_pre.<buffer>" never match because the pass name is followed by '.'.
  std::vector<std::string> buffers;
  const std::string prefix = passName + ".";
  for(const auto& [binding, shouldFlip] : definition.flips) {
   if(!shouldFlip || binding.rfind(prefix, 0) != 0) {
    continue;
   }
   buffers.push_back(binding.substr(prefix.size()));
  }
  return buffers;
}
int ColorTargets::renderTargetSamplerStartIndex(bool fullscreenPass) {
  // IrisSamplers.java:55: colortex0-3 are only sampleable from fullscreen passes.
  return fullscreenPass ? 0 : 4;
}
void ColorTargets::applyPassFlips(const PackDefinition& definition,
                                  const std::string& passName, const std::vector<std::string>& outputs) {
  // Mirrors CompositeRenderer.java:165-187: buffers the pass writes flip unless the
  // pass's explicit directive says false, then every explicit true directive flips its
  // buffer even when the pass does not write it. A written buffer with an explicit
  // true is therefore toggled twice, exactly like Java's BufferFlipper in the
  // drawBuffers loop followed by the explicitFlips sweep (net no change).
  for(const std::string& name : outputs) {
   flipIfEnabled(definition, passName, name);
  }
  for(const std::string& name : explicitTrueFlips(definition, passName)) {
   flip(name);
  }
}
void ColorTargets::applyPreFlips(const PackDefinition& definition, const std::string& stage) {
  // Explicit flip.<stage>_pre.<buffer> directives are applied before that stage's passes
  // run; only true entries flip (false entries are ignored), and pre-flips never count
  // toward flippedAtLeastOnce.
  // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/pipeline/CompositeRenderer.java
  // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/pipeline/IrisRenderingPipeline.java
  static constexpr std::array<std::pair<std::string_view, std::string_view>, 5> stages = {
      std::pair{"begin", "begin_pre."},       std::pair{"shadowcomp", "shadowcomp_pre."},
      std::pair{"prepare", "prepare_pre."},   std::pair{"deferred", "deferred_pre."},
      std::pair{"composite", "composite_pre."}};
  const auto found = std::find_if(stages.begin(), stages.end(), [&](const auto& entry) {
   return entry.first == stage;
  });
  if(found == stages.end()) {
   return;
  }
  const std::string_view pre = found->second;
  for(const auto& [binding, shouldFlip] : definition.flips) {
   if(!shouldFlip || binding.rfind(pre, 0) != 0) {
    continue;
   }
   flip(binding.substr(pre.size()));
  }
 }
} // namespace net::minecraft::client::render
