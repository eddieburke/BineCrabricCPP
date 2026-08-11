#include "net/minecraft/client/render/targets/RenderTargets.hpp"
#include "net/minecraft/client/render/shaderpack/Pack.hpp"
#include "net/minecraft/client/render/GlState.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/GlConstants.hpp"
#include "net/minecraft/client/gl/GlResource.hpp"
#include "net/minecraft/client/debug/RenderProfiler.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/util/logging/Logging.hpp"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <string_view>
namespace net::minecraft::client::render {
namespace {
const std::string& colortexName(int index) {
 static const std::array<std::string, ColorTargets::kMaxColortex> names = [] {
  std::array<std::string, ColorTargets::kMaxColortex> built;
  for(int i = 0; i < ColorTargets::kMaxColortex; ++i) {
   built[static_cast<std::size_t>(i)] = "colortex" + std::to_string(i);
  }
  return built;
 }();
 return names[static_cast<std::size_t>(index)];
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
 writeFboCache_.clear();
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
 if(slot.width == width && slot.height == height && slot.format == format && slot.allocated()) {
  return true;
 }
 freeSlot(slot);
 const GlFormat spec = glFormat(format);
 const bool linear = !render::isIntegerColorFormat(format);
 for(int i = 0; i < 2; ++i) {
  const unsigned int h = core::genTexture();
  if(h == 0) {
   freeSlot(slot);
   return false;
  }
  slot.tex[i] = gl::GlTexture(h);
  debug::RenderProfiler::instance().record(debug::RenderMetric::RenderTargetAllocations);
  core::bindTexture(gl::cap::Texture2D, static_cast<int>(h));
  ::glTexImage2D(gl::cap::Texture2D, 0, spec.internal, width, height, 0, spec.format, spec.type, nullptr);
  setTexParams(linear);
 }
 slot.format = format;
 slot.width = width;
 slot.height = height;
 slot.main = 0;
 ++writeFboGeneration_;
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
 // The const overload below is the only implementation; this one casts the constness
 // back off. Both bodies used to be written out, which is two chances to get the
 // colortexN-vs-named lookup order wrong.
 return const_cast<Slot*>(static_cast<const ColorTargets*>(this)->findSlot(name));
}
const ColorTargets::Slot* ColorTargets::findSlot(const std::string& name) const {
 const int index = colortexIndex(name);
 if(index >= 0 && index < colorCount()) {
  return &slots_[static_cast<std::size_t>(index)];
 }
 const auto found = named_.find(name);
 return found == named_.end() ? nullptr : &found->second;
}
bool ColorTargets::ensure(int width, int height, const std::vector<ColorFormat>& formats,
                          int gbufferColorCount) {
 if(!gl::GLCore::framebufferSupported || width <= 0 || height <= 0) {
  return false;
 }
 gbufferColorCount_ = std::clamp(gbufferColorCount, 1, kMaxColorAttachments);
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
                     if(slots_[i].format != normalized[i] || !slots_[i].allocated()) {
                      return false;
                     }
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
 debug::RenderProfiler::instance().record(debug::RenderMetric::RenderTargetAllocations);
 core::bindTexture(gl::cap::Texture2D, static_cast<int>(depth_.handle()));
 ::glTexImage2D(gl::cap::Texture2D, 0, gl::framebuffer::Depth24Stencil8, width, height, 0,
                gl::pixel::DepthStencil, gl::pixel::UnsignedInt248, nullptr);
 setTexParams(false);
 width_ = width;
 height_ = height;
 valid_ = true;
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
  Slot& slot = slots_[static_cast<std::size_t>(index)];
  const bool changed = !targetMatches(name, width, height, format);
  slot.scaled = width != width_ || height != height_;
  if(!allocateSlot(slot, width, height, format)) {
   return false;
  }
  gbufferFboDirty_ = gbufferFboDirty_ || changed;
  return true;
 }
 return allocateSlot(named_[name], width, height, format);
}
void ColorTargets::rebuildGbufferFbo() {
 if(!valid_ || slots_.empty()) {
  return;
 }
 gbufferFbo_.destroy();
 gbufferFbo_.addDepthAttachment(depth_.handle());
 const std::size_t count = std::min(slots_.size(), static_cast<std::size_t>(gbufferColorCount_));
 std::vector<int> drawBuffers;
 drawBuffers.reserve(count);
 for(std::size_t i = 0; i < count; ++i) {
  if(slots_[i].width != width_ || slots_[i].height != height_) {
   continue;
  }
  gbufferFbo_.addColorAttachment(static_cast<int>(i), slots_[i].readHandle());
  drawBuffers.push_back(static_cast<int>(i));
 }
 if(drawBuffers.empty()) {
  drawBuffers.push_back(0);
 }
 if(!gbufferFbo_.drawBuffers(drawBuffers)) {
  valid_ = false;
  return;
 }
 const unsigned status = gbufferFbo_.checkStatus();
 gl::GLCore::bindFramebuffer(static_cast<unsigned>(gl::framebuffer::Framebuffer), 0);
 if(status != static_cast<unsigned>(gl::framebuffer::Complete)) {
  char hex[32];
  std::snprintf(hex, sizeof(hex), "0x%X", status);
  net::minecraft::util::logging::Logger::getLogger("ColorTargets")
      .log(net::minecraft::util::logging::LogLevel::Severe,
           "rebuildGbufferFbo status failed: " + std::string(hex) + " count=" + std::to_string(count));
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
 std::vector<unsigned int> handles;
 handles.reserve(outputs.size());
 int attachW = 0;
 int attachH = 0;
 for(const std::string& name : outputs) {
  Slot* slot = findSlot(name);
  // allocated(), not just tex[0]: this hands back the WRITE buffer below, so
  // validating only the read one left the caller attaching a null texture.
  if(slot == nullptr || !slot->allocated()) {
   return false;
  }
  handles.push_back(slot->writeHandle());
  attachW = slot->width;
  attachH = slot->height;
 }
 auto found = writeFboCache_.find(outputs);
 // A generation bump means some slot was reallocated since this entry was built —
 // do not trust handle-number comparison to catch it (a freed GL id can be reissued
 // to an unrelated texture), rebuild and revalidate from scratch instead.
 if(found != writeFboCache_.end() && found->second.generation != writeFboGeneration_) {
  writeFboCache_.erase(found);
  found = writeFboCache_.end();
 }
 if(found == writeFboCache_.end()) {
  const auto it = writeFboCache_.try_emplace(outputs).first;
  WriteFboCacheEntry& entry = it->second;
  std::vector<int> drawBuffers;
  drawBuffers.reserve(handles.size());
  for(std::size_t i = 0; i < handles.size(); ++i) {
   entry.fbo.addColorAttachment(static_cast<int>(i), handles[i]);
   drawBuffers.push_back(static_cast<int>(i));
  }
  if(!entry.fbo.drawBuffers(drawBuffers)) {
   writeFboCache_.erase(it);
   return false;
  }
  entry.fbo.removeDepthAttachment();
  const unsigned status = entry.fbo.checkStatus();
  if(status != static_cast<unsigned>(gl::framebuffer::Complete)) {
   writeFboCache_.erase(it);
   return false;
  }
  entry.handles = handles;
  entry.generation = writeFboGeneration_;
  entry.fbo.bind();
 } else {
  // Same generation: the only way a write handle changes is Slot::flipBuffers,
  // which swaps between two textures allocated together and never deletes either
  // one, so a per-attachment handle diff is safe without re-running checkStatus.
  WriteFboCacheEntry& entry = found->second;
  for(std::size_t i = 0; i < handles.size(); ++i) {
   if(entry.handles[i] != handles[i]) {
    entry.fbo.addColorAttachment(static_cast<int>(i), handles[i]);
    entry.handles[i] = handles[i];
   }
  }
  entry.fbo.bind();
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
  writeFbo_.destroy();
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
 // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/targets/ClearPassCreator.java
 std::vector<unsigned int> mainTextures;
 std::vector<unsigned int> altTextures;
 std::vector<ColorFormat> formats;
 mainTextures.reserve(slots_.size());
 altTextures.reserve(slots_.size());
 formats.reserve(slots_.size());
 for(const Slot& slot : slots_) {
  mainTextures.push_back(slot.readHandle());
  altTextures.push_back(slot.writeHandle());
  formats.push_back(slot.format);
 }
 std::vector<bool> effective = enabled;
 if(full) {
  effective.assign(slots_.size(), true);
 }
 int previousFbo = 0;
 ::glGetIntegerv(static_cast<unsigned>(gl::query::FramebufferBinding), &previousFbo);
 clearAttachmentSet(mainTextures, formats, effective, colors);
 clearAttachmentSet(altTextures, formats, effective, colors);
 gl::GLCore::bindFramebuffer(static_cast<unsigned>(gl::framebuffer::Framebuffer),
                             static_cast<unsigned>(previousFbo));
 bindGbuffers();
}
void ColorTargets::clearNamedColors(const std::string& name, const std::array<float, 4>& color) {
 Slot* slot = findSlot(name);
 if(slot == nullptr || !slot->allocated()) {
  return;
 }
 int previousFbo = 0;
 ::glGetIntegerv(static_cast<unsigned>(gl::query::FramebufferBinding), &previousFbo);
 for(const gl::GlTexture& tex : slot->tex) {
  writeFbo_.destroy();
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
 return slot.readHandle();
}
unsigned int ColorTargets::writeTexture(int index) const noexcept {
 if(index < 0 || index >= colorCount()) {
  return 0;
 }
 const Slot& slot = slots_[static_cast<std::size_t>(index)];
 return slot.writeHandle();
}
unsigned int ColorTargets::readTexture(const std::string& name) const {
 const Slot* slot = findSlot(name);
 return slot == nullptr ? 0u : slot->readHandle();
}
unsigned int ColorTargets::writeTexture(const std::string& name) const {
 const Slot* slot = findSlot(name);
 return slot == nullptr ? 0u : slot->writeHandle();
}
ColorFormat ColorTargets::formatOf(const std::string& name) const {
 const Slot* slot = findSlot(name);
 return slot == nullptr ? ColorFormat::Rgba8 : slot->format;
}
bool ColorTargets::targetMatches(const std::string& name, int width, int height,
                                 ColorFormat format) const {
 const Slot* slot = findSlot(name);
 return slot != nullptr && slot->allocated() && slot->width == width && slot->height == height &&
        slot->format == format;
}
void ColorTargets::fillReadSamplers(std::unordered_map<std::string, int>& textures, bool fullscreenPass) const {
 // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/samplers/IrisSamplers.java
 for(int i = renderTargetSamplerStartIndex(fullscreenPass); i < colorCount(); ++i) {
  textures[colortexName(i)] = static_cast<int>(readTexture(i));
 }
 for(const auto& [name, slot] : named_) {
  textures[name] = static_cast<int>(slot.readHandle());
 }
}
void ColorTargets::fillImageBindings(std::unordered_map<std::string, int>& images) const {
 for(int i = 0; i < colorCount(); ++i) {
  images[colortexName(i)] = static_cast<int>(readTexture(i));
 }
 for(const auto& [name, slot] : named_) {
  images[name] = static_cast<int>(slot.readHandle());
 }
}
void ColorTargets::applySlotFilter(Slot& slot, int side) {
 const gl::GlTexture& tex = slot.tex[side];
 if(!tex) {
  return;
 }
 const bool linear = !render::isIntegerColorFormat(slot.format);
 const int mag = linear ? gl::filter::Linear : gl::filter::Nearest;
 const int min = slot.mipmapsOn[side]
                     ? (linear ? gl::filter::LinearMipmapLinear : gl::filter::NearestMipmapLinear)
                     : mag;
 core::bindTexture(gl::cap::Texture2D, static_cast<int>(tex.handle()));
 ::glTexParameteri(gl::cap::Texture2D, gl::tex::MinFilter, min);
 ::glTexParameteri(gl::cap::Texture2D, gl::tex::MagFilter, mag);
}
void ColorTargets::resetSlotFilters(Slot& slot) {
 for(int side = 0; side < 2; ++side) {
  if(slot.mipmapsOn[side]) {
   slot.mipmapsOn[side] = false;
   applySlotFilter(slot, side);
  }
 }
}
void ColorTargets::enableMipmaps(const std::string& name) {
 Slot* slot = findSlot(name);
 if(slot == nullptr || !slot->allocated() || gl::GLCore::generateMipmap == nullptr) {
  return;
 }
 slot->mipmapsOn[slot->main] = true;
 applySlotFilter(*slot, slot->main);
 gl::GLCore::generateMipmap(gl::cap::Texture2D);
 debug::RenderProfiler::instance().record(debug::RenderMetric::MipmapGenerations);
}
void ColorTargets::resetMipmaps() {
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
 slot->flipBuffers();
 if(colortexIndex(name) >= 0) {
  gbufferFboDirty_ = true;
 }
}
void ColorTargets::flipIfEnabled(const PackDefinition& definition, const std::string& passName,
                                 const std::string& bufferName) {
 // A written buffer flips unless an explicit `false` blocks it (CompositeRenderer.java:174).
 if(!flipExplicitlyBlocked(definition, passName, bufferName)) {
  this->flip(bufferName);
 }
}
bool ColorTargets::flipExplicitlyBlocked(const PackDefinition& definition,
                                         const std::string& passName, const std::string& bufferName) {
 // CompositeRenderer.java:174: only an explicit false blocks the flip.
 const auto declared = definition.flips.find(passName + "." + bufferName);
 return declared != definition.flips.end() && !declared->second;
}
std::vector<std::string> ColorTargets::explicitTrueFlips(const PackDefinition& definition,
                                                         const std::string& passName) {
 // CompositeRenderer.java:182-187: buffers explicitly flipped even when the pass
 // does not write them. Pre-flip keys like `composite.pre` are handled elsewhere.
 std::vector<std::string> buffers;
 const std::string prefix = passName + ".";
 for(const auto& [binding, shouldFlip] : definition.flips) {
  if(!shouldFlip || binding.rfind(prefix, 0) != 0) {
   continue;
  }
  buffers.push_back(binding.substr(prefix.size()));
 }
 std::sort(buffers.begin(), buffers.end());
 return buffers;
}
int ColorTargets::renderTargetSamplerStartIndex(bool fullscreenPass) {
 // IrisSamplers.java:55: colortex0-3 are only sampleable from fullscreen passes.
 return fullscreenPass ? 0 : 4;
}
void ColorTargets::applyPassFlips(const PackDefinition& definition,
                                  const std::string& passName, const std::vector<std::string>& outputs) {
 // CompositeRenderer.java:165-187: buffers the pass writes flip unless the pack
 // blocks it; buffers declared with an explicit true flip anyway.
 for(const std::string& name : outputs) {
  flipIfEnabled(definition, passName, name);
 }
 for(const std::string& name : explicitTrueFlips(definition, passName)) {
  flip(name);
 }
}
void ColorTargets::applyPreFlips(const PackDefinition& definition, const std::string& stage) {
 // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/pipeline/CompositeRenderer.java
 static constexpr std::array<std::pair<std::string_view, std::string_view>, 5> stages = {
     std::pair{"begin", "begin_pre."}, std::pair{"shadowcomp", "shadowcomp_pre."},
     std::pair{"prepare", "prepare_pre."}, std::pair{"deferred", "deferred_pre."},
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
