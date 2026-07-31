#include "net/minecraft/client/render/RenderTargets.hpp"
#include "net/minecraft/client/render/shaderpack/ShaderPack.hpp"
#include "net/minecraft/client/render/shaderpack/ShaderPackGl.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include <algorithm>
#include <cstdlib>
#include <cstring>
namespace net::minecraft::client::render {
namespace {
constexpr unsigned kFramebuffer = 0x8D40;
constexpr unsigned kColorAttachment0 = 0x8CE0;
constexpr unsigned kDepthStencilAttachment = 0x821A;
constexpr unsigned kDepth24Stencil8 = 0x88F0;
constexpr unsigned kDepthStencil = 0x84F9;
constexpr unsigned kUnsignedInt248 = 0x84FA;
constexpr unsigned kFramebufferComplete = 0x8CD5;
constexpr unsigned kTexture2D = 0x0DE1;
constexpr int kFramebufferBinding = 0x8CA6;
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
 case ColorFormat::R8Ui: return {0x8232, 0x8D94, 0x1401};
 case ColorFormat::R16Ui: return {0x8234, 0x8D94, 0x1403};
 case ColorFormat::R32Ui: return {0x8236, 0x8D94, 0x1405};
 case ColorFormat::Rg8Ui: return {0x8238, 0x8228, 0x1401};
 case ColorFormat::Rg16Ui: return {0x823A, 0x8228, 0x1403};
 case ColorFormat::Rg32Ui: return {0x823C, 0x8228, 0x1405};
 case ColorFormat::Rgba8Ui: return {0x8D7C, 0x8D99, 0x1401};
 case ColorFormat::Rgba16Ui: return {0x8D76, 0x8D99, 0x1403};
 case ColorFormat::Rgba32Ui: return {0x8D70, 0x8D99, 0x1405};
 case ColorFormat::R8I: return {0x8231, 0x8D94, 0x1400};
 case ColorFormat::R16I: return {0x8233, 0x8D94, 0x1402};
 case ColorFormat::R32I: return {0x8235, 0x8D94, 0x1404};
 case ColorFormat::Rg8I: return {0x8237, 0x8228, 0x1400};
 case ColorFormat::Rg16I: return {0x8239, 0x8228, 0x1402};
 case ColorFormat::Rg32I: return {0x823B, 0x8228, 0x1404};
 case ColorFormat::Rgba8I: return {0x8D8E, 0x8D99, 0x1400};
 case ColorFormat::Rgba16I: return {0x8D88, 0x8D99, 0x1402};
 case ColorFormat::Rgba32I: return {0x8D82, 0x8D99, 0x1404};
 default: return {0x8058, 0x1908, 0x1401};
 }
}
void setTexParams(bool filterLinear) {
 ::glTexParameteri(kTexture2D, 0x2801, filterLinear ? 0x2601 : 0x2600);
 ::glTexParameteri(kTexture2D, 0x2800, filterLinear ? 0x2601 : 0x2600);
 ::glTexParameteri(kTexture2D, 0x2802, 0x812F);
 ::glTexParameteri(kTexture2D, 0x2803, 0x812F);
}
} // namespace
void ColorTargets::destroy() {
 if(gbufferActive_) {
  endGbuffers();
 }
 if(gbufferFbo_ != 0) {
  gl::GLCore::deleteFramebuffers(1, &gbufferFbo_);
  gbufferFbo_ = 0;
 }
 if(writeFbo_ != 0) {
  gl::GLCore::deleteFramebuffers(1, &writeFbo_);
  writeFbo_ = 0;
 }
 if(copyReadFbo_ != 0) {
  gl::GLCore::deleteFramebuffers(1, &copyReadFbo_);
  copyReadFbo_ = 0;
 }
 if(copyDrawFbo_ != 0) {
  gl::GLCore::deleteFramebuffers(1, &copyDrawFbo_);
  copyDrawFbo_ = 0;
 }
 if(depth_ != 0) {
  core::deleteTexture(depth_);
  depth_ = 0;
 }
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
 for(unsigned int& tex : slot.tex) {
  if(tex != 0) {
   core::deleteTexture(tex);
   tex = 0;
  }
 }
 slot.main = 0;
 slot.width = 0;
 slot.height = 0;
}
bool ColorTargets::allocateSlot(Slot& slot, int width, int height, ColorFormat format) {
 if(slot.width == width && slot.height == height && slot.format == format && slot.tex[0] != 0 &&
    slot.tex[1] != 0) {
  return true;
 }
 freeSlot(slot);
 const GlFormat spec = glFormat(format);
 // Integer colortex cannot use LINEAR filter (GL incomplete / undefined).
 const bool linear = !render::isIntegerColorFormat(format);
 for(int i = 0; i < 2; ++i) {
  slot.tex[i] = core::genTexture();
  if(slot.tex[i] == 0) {
   freeSlot(slot);
   return false;
  }
  core::bindTexture(kTexture2D, static_cast<int>(slot.tex[i]));
  ::glTexImage2D(kTexture2D, 0, spec.internal, width, height, 0, spec.format, spec.type, nullptr);
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
                     if(slots_[i].format != normalized[i] || slots_[i].width != width ||
                        slots_[i].height != height || slots_[i].tex[0] == 0 || slots_[i].tex[1] == 0) {
                      return false;
                     }
                    }
                    return depth_ != 0;
                   }();
 if(same) {
  return true;
 }
 destroy();
 slots_.resize(normalized.size());
 for(std::size_t i = 0; i < normalized.size(); ++i) {
  if(!allocateSlot(slots_[i], width, height, normalized[i])) {
   destroy();
   return false;
  }
 }
 depth_ = core::genTexture();
 if(depth_ == 0) {
  destroy();
  return false;
 }
 core::bindTexture(kTexture2D, static_cast<int>(depth_));
 ::glTexImage2D(kTexture2D, 0, static_cast<int>(kDepth24Stencil8), width, height, 0, kDepthStencil,
                kUnsignedInt248, nullptr);
 setTexParams(false);
 width_ = width;
 height_ = height;
 valid_ = true;
 gbufferFboDirty_ = true;
 rebuildGbufferFbo();
 return valid_ && gbufferFbo_ != 0;
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
  // Fullscreen colortex already dual-buffered by ensure(); sized overrides use named_.
  if(width == width_ && height == height_) {
   return allocateSlot(slots_[static_cast<std::size_t>(index)], width, height, format);
  }
 }
 return allocateSlot(named_[name], width, height, format);
}
void ColorTargets::rebuildGbufferFbo() {
 if(!valid_ || slots_.empty()) {
  return;
 }
 if(gbufferFbo_ == 0) {
  gl::GLCore::genFramebuffers(1, &gbufferFbo_);
 }
 gl::GLCore::bindFramebuffer(kFramebuffer, gbufferFbo_);
 std::vector<unsigned int> drawBuffers;
 drawBuffers.reserve(slots_.size());
 for(std::size_t i = 0; i < slots_.size(); ++i) {
  const unsigned int tex = slots_[i].tex[slots_[i].main];
  gl::GLCore::framebufferTexture2D(kFramebuffer, kColorAttachment0 + static_cast<unsigned>(i), kTexture2D, tex,
                                   0);
  drawBuffers.push_back(kColorAttachment0 + static_cast<unsigned>(i));
 }
 if(gl::GLCore::drawBuffers != nullptr) {
  gl::GLCore::drawBuffers(static_cast<int>(drawBuffers.size()), drawBuffers.data());
 }
 gl::GLCore::framebufferTexture2D(kFramebuffer, kDepthStencilAttachment, kTexture2D, depth_, 0);
 const unsigned status = gl::GLCore::checkFramebufferStatus(kFramebuffer);
 gl::GLCore::bindFramebuffer(kFramebuffer, 0);
 if(status != kFramebufferComplete) {
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
  ::glGetIntegerv(static_cast<unsigned>(kFramebufferBinding), &previousBoundFbo_);
  core::getCachedViewport(savedViewport_);
  gbufferActive_ = true;
 }
 gl::GLCore::bindFramebuffer(kFramebuffer, gbufferFbo_);
 core::viewport(0, 0, width_, height_);
}
void ColorTargets::endGbuffers() {
 if(!gbufferActive_) {
  return;
 }
 gl::GLCore::bindFramebuffer(kFramebuffer, static_cast<unsigned>(previousBoundFbo_));
 core::viewport(savedViewport_[0], savedViewport_[1], savedViewport_[2], savedViewport_[3]);
 gbufferActive_ = false;
}
bool ColorTargets::bindWrite(const std::vector<std::string>& outputs) {
 if(!valid_ || outputs.empty()) {
  return false;
 }
 if(writeFbo_ == 0) {
  gl::GLCore::genFramebuffers(1, &writeFbo_);
 }
 gl::GLCore::bindFramebuffer(kFramebuffer, writeFbo_);
 std::vector<unsigned int> drawBuffers;
 drawBuffers.reserve(outputs.size());
 int attachW = 0;
 int attachH = 0;
 for(std::size_t i = 0; i < outputs.size(); ++i) {
  Slot* slot = findSlot(outputs[i]);
  if(slot == nullptr || slot->tex[0] == 0) {
   return false;
  }
  const unsigned int tex = slot->tex[1 - slot->main];
  gl::GLCore::framebufferTexture2D(kFramebuffer, kColorAttachment0 + static_cast<unsigned>(i), kTexture2D, tex,
                                   0);
  drawBuffers.push_back(kColorAttachment0 + static_cast<unsigned>(i));
  attachW = slot->width;
  attachH = slot->height;
 }
 if(gl::GLCore::drawBuffers != nullptr) {
  gl::GLCore::drawBuffers(static_cast<int>(drawBuffers.size()), drawBuffers.data());
 }
 gl::GLCore::framebufferTexture2D(kFramebuffer, kDepthStencilAttachment, kTexture2D, 0, 0);
 const unsigned status = gl::GLCore::checkFramebufferStatus(kFramebuffer);
 if(status != kFramebufferComplete) {
  return false;
 }
 core::viewport(0, 0, attachW > 0 ? attachW : width_, attachH > 0 ? attachH : height_);
 return true;
}
void ColorTargets::clearColors(const std::vector<bool>& enabled,
                                     const std::vector<std::array<float, 4>>& colors) {
 if(!valid_) {
  return;
 }
 bindGbuffers();
 // World programs remap DrawBuffers via RENDERTARGETS; restore full sequential
 // mapping so clearBuffer*(COLOR, i) hits colortex i.
 if(gl::GLCore::drawBuffers != nullptr) {
  std::vector<unsigned int> drawBuffers;
  drawBuffers.reserve(slots_.size());
  for(std::size_t i = 0; i < slots_.size(); ++i) {
   drawBuffers.push_back(kColorAttachment0 + static_cast<unsigned>(i));
  }
  gl::GLCore::drawBuffers(static_cast<int>(drawBuffers.size()), drawBuffers.data());
 }
 if(gl::GLCore::clearBufferfv == nullptr) {
  core::clear(0x00004000);
  return;
 }
 for(int i = 0; i < colorCount(); ++i) {
  if(static_cast<std::size_t>(i) < enabled.size() && !enabled[static_cast<std::size_t>(i)]) {
   continue;
  }
  static constexpr std::array<float, 4> zero{};
  const auto& color = static_cast<std::size_t>(i) < colors.size() ? colors[static_cast<std::size_t>(i)] : zero;
  const ColorFormat fmt = slots_[static_cast<std::size_t>(i)].format;
  if(render::isIntegerColorFormat(fmt)) {
   if(render::isSignedIntegerColorFormat(fmt)) {
    if(gl::GLCore::clearBufferiv == nullptr) {
     continue;
    }
    const int values[4] = {static_cast<int>(color[0]), static_cast<int>(color[1]), static_cast<int>(color[2]),
                           static_cast<int>(color[3])};
    gl::GLCore::clearBufferiv(0x1800, i, values);
   } else if(gl::GLCore::clearBufferuiv != nullptr) {
    const unsigned values[4] = {static_cast<unsigned>(color[0]), static_cast<unsigned>(color[1]),
                                static_cast<unsigned>(color[2]), static_cast<unsigned>(color[3])};
    gl::GLCore::clearBufferuiv(0x1800, i, values);
   }
   continue;
  }
  gl::GLCore::clearBufferfv(0x1800, i, color.data());
 }
}
unsigned int ColorTargets::readTexture(int index) const noexcept {
 if(index < 0 || index >= colorCount()) {
  return 0;
 }
 const Slot& slot = slots_[static_cast<std::size_t>(index)];
 return slot.tex[slot.main];
}
unsigned int ColorTargets::writeTexture(int index) const noexcept {
 if(index < 0 || index >= colorCount()) {
  return 0;
 }
 const Slot& slot = slots_[static_cast<std::size_t>(index)];
 return slot.tex[1 - slot.main];
}
unsigned int ColorTargets::readTexture(const std::string& name) const {
 const Slot* slot = findSlot(name);
 return slot == nullptr ? 0u : slot->tex[slot->main];
}
unsigned int ColorTargets::writeTexture(const std::string& name) const {
 const Slot* slot = findSlot(name);
 return slot == nullptr ? 0u : slot->tex[1 - slot->main];
}
ColorFormat ColorTargets::formatOf(const std::string& name) const {
 const Slot* slot = findSlot(name);
 return slot == nullptr ? ColorFormat::Rgba8 : slot->format;
}
void ColorTargets::fillReadSamplers(std::unordered_map<std::string, int>& textures) const {
 for(int i = 0; i < colorCount(); ++i) {
  textures["colortex" + std::to_string(i)] = static_cast<int>(readTexture(i));
 }
 for(const auto& [name, slot] : named_) {
  textures[name] = static_cast<int>(slot.tex[slot.main]);
 }
}
void ColorTargets::fillImageBindings(std::unordered_map<std::string, int>& images) const {
 for(int i = 0; i < colorCount(); ++i) {
  images["colortex" + std::to_string(i)] = static_cast<int>(readTexture(i));
 }
 for(const auto& [name, slot] : named_) {
  images[name] = static_cast<int>(slot.tex[slot.main]);
 }
}
void ColorTargets::prepareWrite(const std::string& name) {
 Slot* slot = findSlot(name);
 if(slot == nullptr || slot->tex[0] == 0 || slot->tex[1] == 0 || gl::GLCore::blitFramebuffer == nullptr) {
  return;
 }
 if(copyReadFbo_ == 0) {
  gl::GLCore::genFramebuffers(1, &copyReadFbo_);
 }
 if(copyDrawFbo_ == 0) {
  gl::GLCore::genFramebuffers(1, &copyDrawFbo_);
 }
 const unsigned int src = slot->tex[slot->main];
 const unsigned int dst = slot->tex[1 - slot->main];
 constexpr unsigned kReadFramebuffer = 0x8CA8;
 constexpr unsigned kDrawFramebuffer = 0x8CA9;
 gl::GLCore::bindFramebuffer(kReadFramebuffer, copyReadFbo_);
 gl::GLCore::framebufferTexture2D(kReadFramebuffer, kColorAttachment0, kTexture2D, src, 0);
 gl::GLCore::bindFramebuffer(kDrawFramebuffer, copyDrawFbo_);
 gl::GLCore::framebufferTexture2D(kDrawFramebuffer, kColorAttachment0, kTexture2D, dst, 0);
 gl::GLCore::blitFramebuffer(0, 0, slot->width, slot->height, 0, 0, slot->width, slot->height, 0x00004000,
                             0x2600);
 gl::GLCore::bindFramebuffer(kReadFramebuffer, 0);
 gl::GLCore::bindFramebuffer(kDrawFramebuffer, 0);
}
void ColorTargets::prepareWrites(const std::vector<std::string>& names) {
 for(const std::string& name : names) {
  prepareWrite(name);
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
void ColorTargets::flipIfEnabled(const shaderpack::ShaderPackDefinition& definition, const std::string& passName,
                                       const std::string& bufferName) {
 bool flip = true;
 if(const auto declared = definition.flips.find(passName + "." + bufferName);
    declared != definition.flips.end()) {
  flip = declared->second;
 }
 if(flip) {
  this->flip(bufferName);
 }
}
void ColorTargets::applyPreFlips(const shaderpack::ShaderPackDefinition& definition, const std::string& stage) {
 if(stage != "deferred" && stage != "composite") {
  return;
 }
 const std::string pre = stage == "deferred" ? "deferred_pre." : "composite_pre.";
 for(const auto& [binding, shouldFlip] : definition.flips) {
  if(!shouldFlip || binding.rfind(pre, 0) != 0) {
   continue;
  }
  flip(binding.substr(pre.size()));
 }
}
} // namespace net::minecraft::client::render
