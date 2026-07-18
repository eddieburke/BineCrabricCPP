#pragma once
#include <initializer_list>
#include "net/minecraft/client/gl/EnginePipeline.hpp"
#include "net/minecraft/client/gl/GlDraw.hpp"
namespace net::minecraft::client::gl {
// RAII state scopes: save/restore slices of fixed-function state around a preset.
class CapScope {
public:
  CapScope(std::initializer_list<int> caps) {
    previousPipeline_ = g_pipeline;
    for(int cap : caps) {
      if(count_ < kMax) {
        // All FF-mirrored fields are restored from one CPU shadow copy.
        if(!isPipelineStateCap(cap)) {
          entries_[count_++] = Entry{cap, capEnabled(cap)};
        }
      }
    }
  }
  CapScope(const CapScope&) = delete;
  CapScope& operator=(const CapScope&) = delete;
  ~CapScope() {
    g_pipeline = previousPipeline_;
    g_pipeline.dirty = true;
    for(int i = count_ - 1; i >= 0; --i) {
      setCap(entries_[i].cap, entries_[i].enabled);
    }
  }

private:
  static constexpr int kMax = 32;
  struct Entry {
    int cap;
    bool enabled;
  };
  Entry entries_[kMax]{};
  int count_ = 0;
  PipelineState previousPipeline_{};
};
class DepthMaskScope {
public:
  DepthMaskScope() {
    GLboolean value = GL_TRUE;
    ::glGetBooleanv(GL_DEPTH_WRITEMASK, &value);
    previous_ = value == GL_TRUE;
  }
  DepthMaskScope(const DepthMaskScope&) = delete;
  DepthMaskScope& operator=(const DepthMaskScope&) = delete;
  ~DepthMaskScope() {
    depthMask(previous_);
  }

private:
  bool previous_ = true;
};
class ShadeModelScope {
public:
  ShadeModelScope() : previous_(g_pipeline.smoothShading) {}
  ShadeModelScope(const ShadeModelScope&) = delete;
  ShadeModelScope& operator=(const ShadeModelScope&) = delete;
  ~ShadeModelScope() {
    g_pipeline.smoothShading = previous_;
    g_pipeline.dirty = true;
  }

private:
  bool previous_ = true;
};
class BlendFuncScope {
public:
  BlendFuncScope() {
    GLint src = blend::One;
    GLint dst = blend::Zero;
    ::glGetIntegerv(query::BlendSrc, &src);
    ::glGetIntegerv(query::BlendDst, &dst);
    src_ = static_cast<int>(src);
    dst_ = static_cast<int>(dst);
  }
  BlendFuncScope(const BlendFuncScope&) = delete;
  BlendFuncScope& operator=(const BlendFuncScope&) = delete;
  ~BlendFuncScope() {
    blendFunc(src_, dst_);
  }

private:
  int src_ = blend::One;
  int dst_ = blend::Zero;
};
class DepthFuncScope {
public:
  DepthFuncScope() {
    GLint mode = compare::Lequal;
    ::glGetIntegerv(query::DepthFunc, &mode);
    previous_ = static_cast<int>(mode);
  }
  DepthFuncScope(const DepthFuncScope&) = delete;
  DepthFuncScope& operator=(const DepthFuncScope&) = delete;
  ~DepthFuncScope() {
    depthFunc(previous_);
  }

private:
  int previous_ = compare::Lequal;
};
class AlphaFuncScope {
public:
  AlphaFuncScope() : ref_(g_pipeline.alphaRef) {}
  AlphaFuncScope(const AlphaFuncScope&) = delete;
  AlphaFuncScope& operator=(const AlphaFuncScope&) = delete;
  ~AlphaFuncScope() {
    alphaFunc(compare::Greater, ref_);
  }

private:
  float ref_ = 0.1f;
};
class BoundTextureScope {
public:
  BoundTextureScope() {
    int bound = 0;
    getIntegerv(tex::Binding2D, &bound);
    previous_ = static_cast<unsigned>(bound);
  }
  BoundTextureScope(const BoundTextureScope&) = delete;
  BoundTextureScope& operator=(const BoundTextureScope&) = delete;
  ~BoundTextureScope() {
    bindTexture(cap::Texture2D, static_cast<int>(previous_));
  }

private:
  unsigned previous_ = 0;
};
class TextureStateScope {
public:
  TextureStateScope() {
    getIntegerv(tex::ActiveTexture, &previousActive_);
    getIntegerv(tex::Binding2D, &previousBinding_);
  }
  TextureStateScope(const TextureStateScope&) = delete;
  TextureStateScope& operator=(const TextureStateScope&) = delete;
  ~TextureStateScope() {
    bindTexture(cap::Texture2D, previousBinding_);
    activeTexture(previousActive_);
  }

private:
  int previousActive_ = tex::Texture0;
  int previousBinding_ = 0;
};
namespace pass {
inline void applyOrthoProjection(const util::UiScale& scale) {
  matrixMode(matrix_::Projection);
  loadIdentity();
  ortho(0.0, scale.rawWidth, scale.rawHeight, 0.0, 1000.0, 3000.0);
  matrixMode(matrix_::ModelView);
  loadIdentity();
  translatef(0.0f, 0.0f, -2000.0f);
}
inline void applyHudEnables() {
  disable(cap::CullFace);
  enable(cap::Texture2D);
  enable(cap::AlphaTest);
  alphaFunc(compare::Greater, 0.1f);
  color4f(1.0f, 1.0f, 1.0f, 1.0f);
}
inline void applyScreenEnables() {
  disable(cap::CullFace);
  disable(cap::Lighting);
  disable(cap::Fog);
  enable(cap::Texture2D);
  enable(cap::AlphaTest);
  alphaFunc(compare::Greater, 0.1f);
  color4f(1.0f, 1.0f, 1.0f, 1.0f);
}
inline void bindAtlas2D(int textureId) {
  enable(cap::Texture2D);
  bindTexture(cap::Texture2D, static_cast<unsigned>(textureId));
  color4f(1.0f, 1.0f, 1.0f, 1.0f);
}
inline void beginHud(const util::UiScale& scale) {
  clear(attrib::DepthBufferBit);
  applyOrthoProjection(scale);
  applyHudEnables();
}
inline void beginScreen(const util::UiScale& scale, int viewportW, int viewportH) {
  viewport(0, 0, viewportW, viewportH);
  applyScreenEnables();
  clear(attrib::DepthBufferBit);
  applyOrthoProjection(scale);
  clear(attrib::DepthBufferBit);
}
} // namespace pass
// Blits the currently bound 2D texture over the given viewport using the core-profile
// fullscreen-triangle shader (see EnginePipeline::blitFullscreen).
inline void drawFullscreenTexturedQuad(int width, int height) {
  int savedViewport[4]{0, 0, width, height};
  getIntegerv(query::Viewport, savedViewport);
  const CapScope caps{cap::DepthTest, cap::CullFace, cap::Blend, cap::ScissorTest};
  const DepthMaskScope depthMaskScope;
  viewport(0, 0, width, height);
  setCap(cap::DepthTest, false);
  setCap(cap::CullFace, false);
  setCap(cap::Blend, false);
  setCap(cap::ScissorTest, false);
  depthMask(false);
  activeTexture(tex::Texture0);
  engine_pipeline::blitFullscreen();
  viewport(savedViewport[0], savedViewport[1], savedViewport[2], savedViewport[3]);
}
namespace detail {
inline void alphaBlend(bool alphaTest = false) {
  setCap(cap::Blend, true);
  blendFunc(blend::SrcAlpha, blend::OneMinusSrcAlpha);
  setCap(cap::AlphaTest, alphaTest);
}
inline void hudOverlay() {
  setCap(cap::DepthTest, false);
  depthMask(false);
  alphaBlend(false);
}
} // namespace detail
namespace preset {
struct ModDraw {
  CapScope caps{cap::Lighting,
                cap::Texture2D,
                cap::DepthTest,
                cap::Blend,
                cap::AlphaTest,
                cap::Fog,
                cap::CullFace,
                cap::ScissorTest};
};
struct ModLuaDraw {
  CapScope caps{cap::Lighting,
                cap::Texture2D,
                cap::DepthTest,
                cap::Blend,
                cap::AlphaTest,
                cap::Fog,
                cap::CullFace,
                cap::ScissorTest};
  DepthMaskScope depth;
  ShadeModelScope shade;
  ModLuaDraw(bool textured, bool blend, bool cull, bool depthTest, bool depthWrite) {
    setCap(cap::Texture2D, textured);
    if(blend) {
      detail::alphaBlend(textured);
    } else {
      setCap(cap::Blend, false);
    }
    setCap(cap::CullFace, cull);
    setCap(cap::DepthTest, depthTest);
    depthMask(depthWrite);
    shadeModel(shade::Flat);
  }
};
struct ModLuaBillboardDraw {
  CapScope caps{cap::Texture2D, cap::CullFace, cap::Blend, cap::DepthTest};
  DepthMaskScope depth;
  BlendFuncScope blend;
  ModLuaBillboardDraw(bool additive, bool depthTest, bool depthWrite) {
    setCap(cap::Texture2D, false);
    setCap(cap::CullFace, false);
    setCap(cap::Blend, true);
    blendFunc(blend::SrcAlpha, additive ? blend::One : blend::OneMinusSrcAlpha);
    setCap(cap::DepthTest, depthTest);
    depthMask(depthWrite);
  }
};
struct ModLuaGuiDraw {
  CapScope caps{cap::Lighting,
                cap::Texture2D,
                cap::DepthTest,
                cap::Blend,
                cap::AlphaTest,
                cap::Fog,
                cap::CullFace,
                cap::ScissorTest};
  ModLuaGuiDraw(bool textured = true) {
    setCap(cap::DepthTest, false);
    setCap(cap::Texture2D, textured);
    setCap(cap::Lighting, false);
  }
};
using ModLuaGuiTextDraw = ModLuaGuiDraw;
struct ModLuaGuiItemDraw {
  CapScope caps{cap::Fog,
                cap::Blend,
                cap::Texture2D,
                cap::RescaleNormal,
                cap::DepthTest,
                cap::Lighting,
                cap::ColorMaterial};
  ModLuaGuiItemDraw() {
    setCap(cap::Fog, false);
    setCap(cap::Blend, false);
    setCap(cap::Texture2D, true);
    setCap(cap::RescaleNormal, true);
    setCap(cap::DepthTest, true);
    setCap(cap::Lighting, true);
    setCap(cap::ColorMaterial, true);
  }
};
struct ModLuaGuiSpriteDraw {
  CapScope caps{cap::Fog, cap::Texture2D};
  ModLuaGuiSpriteDraw() {
    setCap(cap::Fog, false);
    color4f(1.0f, 1.0f, 1.0f, 1.0f);
    setCap(cap::Texture2D, true);
  }
};
struct TexturedGuiNoAlphaTest {
  CapScope caps{cap::Blend, cap::Texture2D, cap::AlphaTest};
  TexturedGuiNoAlphaTest() {
    detail::alphaBlend(false);
  }
};
struct TextDraw {
  CapScope caps{cap::Lighting, cap::ColorMaterial, cap::Blend, cap::Texture2D};
  TextDraw() {
    setCap(cap::Texture2D, true);
    detail::alphaBlend(false);
  }
};
struct LabelDraw {
  CapScope caps{cap::DepthTest, cap::Blend, cap::Texture2D};
  DepthMaskScope depth;
  LabelDraw() {
    depthMask(false);
    setCap(cap::DepthTest, false);
    detail::alphaBlend(false);
  }
};
struct SignTextDraw {
  CapScope caps{cap::DepthTest, cap::Blend, cap::Texture2D};
  DepthMaskScope depth;
  SignTextDraw() {
    depthMask(false);
    setCap(cap::DepthTest, true);
    detail::alphaBlend(false);
  }
};
struct TextureOff {
  CapScope caps{cap::Texture2D};
  TextureOff() {
    setCap(cap::Texture2D, false);
  }
};
using LabelTextureOff = TextureOff;
struct WorldOverlay {
  CapScope caps{cap::Blend, cap::AlphaTest, cap::Texture2D, cap::DepthTest, cap::PolygonOffsetFill};
  BlendFuncScope blend;
  WorldOverlay() {
    setCap(cap::Blend, true);
    setCap(cap::AlphaTest, true);
    blendFunc(blend::Zero, blend::SrcColor);
  }
};
struct WorldCrackOverlay {
  CapScope caps{cap::AlphaTest, cap::PolygonOffsetFill};
  WorldCrackOverlay() {
    setCap(cap::AlphaTest, false);
    polygonOffset(-3.0f, -3.0f);
    setCap(cap::PolygonOffsetFill, true);
    setCap(cap::AlphaTest, true);
  }
};
struct TexturedBlend {
  CapScope caps{cap::Blend, cap::Texture2D};
};
using ModChunkMesh = TexturedBlend;
struct SolidFill {
  CapScope caps{cap::Blend, cap::Texture2D};
  SolidFill() {
    detail::alphaBlend(false);
    setCap(cap::Texture2D, false);
  }
};
struct GradientFill {
  CapScope caps{cap::Blend, cap::Texture2D, cap::AlphaTest};
  ShadeModelScope shade;
  GradientFill() {
    setCap(cap::Texture2D, false);
    detail::alphaBlend(false);
    shadeModel(shade::Smooth);
  }
};
struct ContainerScreen {
  CapScope caps{cap::Fog, cap::RescaleNormal, cap::Lighting, cap::DepthTest};
  ContainerScreen() {
    setCap(cap::Fog, false);
  }
};
struct HandledSlotItem {
  CapScope caps{cap::RescaleNormal, cap::Lighting, cap::DepthTest, cap::Fog, cap::Texture2D};
  HandledSlotItem() {
    setCap(cap::RescaleNormal, true);
  }
};
struct HandledSlotDecoration {
  CapScope caps{cap::RescaleNormal, cap::Lighting, cap::DepthTest};
  HandledSlotDecoration() {
    setCap(cap::RescaleNormal, false);
    setCap(cap::Lighting, false);
    setCap(cap::DepthTest, false);
  }
};
struct HandledTooltipDraw {
  CapScope caps{cap::Fog, cap::Texture2D, cap::Lighting};
  HandledTooltipDraw() {
    setCap(cap::Fog, false);
    setCap(cap::Texture2D, true);
    setCap(cap::Lighting, false);
  }
};
struct SlotHighlight {
  CapScope caps{cap::Lighting, cap::DepthTest};
  SlotHighlight() {
    setCap(cap::Lighting, false);
    setCap(cap::DepthTest, false);
  }
};
struct FireOverlay {
  CapScope caps{cap::Blend, cap::Texture2D};
  DepthMaskScope depth;
  FireOverlay() {
    detail::alphaBlend(false);
  }
};
struct EntityShadow {
  CapScope caps{cap::Blend, cap::Texture2D};
  DepthMaskScope depth;
  EntityShadow() {
    detail::alphaBlend(false);
    setCap(cap::Texture2D, true);
  }
};
struct HudOverlay {
  CapScope caps{cap::DepthTest, cap::Blend, cap::AlphaTest};
  DepthMaskScope depth;
  HudOverlay() {
    detail::hudOverlay();
  }
};
struct HudVignette {
  CapScope caps{cap::DepthTest, cap::Blend, cap::AlphaTest};
  DepthMaskScope depth;
  BlendFuncScope blend;
  HudVignette() {
    setCap(cap::DepthTest, false);
    depthMask(false);
    setCap(cap::Blend, true);
    setCap(cap::AlphaTest, false);
    blendFunc(blend::Zero, blend::OneMinusSrcColor);
  }
};
struct HudPass {
  CapScope caps{cap::Blend};
  HudPass() {
    setCap(cap::Blend, true);
    blendFunc(blend::SrcAlpha, blend::OneMinusSrcAlpha);
  }
};
struct HudFadeText {
  CapScope caps{cap::Blend, cap::AlphaTest};
  HudFadeText() {
    detail::alphaBlend(false);
  }
};
using HudChatBackground = TextureOff;
struct HudCrosshairBlend {
  CapScope caps{cap::Blend};
  BlendFuncScope blend;
  HudCrosshairBlend() {
    setCap(cap::Blend, true);
    blendFunc(blend::OneMinusDstColor, blend::OneMinusSrcColor);
  }
};
struct HudSleepOverlay {
  CapScope caps{cap::DepthTest, cap::AlphaTest};
  HudSleepOverlay() {
    setCap(cap::DepthTest, false);
    setCap(cap::AlphaTest, false);
  }
};
struct PlayerPreview {
  CapScope caps{cap::RescaleNormal, cap::DepthTest};
  DepthMaskScope depth;
  PlayerPreview() {
    setCap(cap::RescaleNormal, true);
    setCap(cap::DepthTest, true);
    depthMask(true);
  }
};
struct GuiItemRescale {
  CapScope caps{cap::RescaleNormal, cap::Lighting, cap::DepthTest};
  GuiItemRescale() {
    setCap(cap::RescaleNormal, true);
    setCap(cap::Lighting, true);
    setCap(cap::DepthTest, true);
  }
};
struct GuiItemLabel {
  CapScope caps{cap::Lighting, cap::DepthTest};
  GuiItemLabel() {
    setCap(cap::Lighting, false);
    setCap(cap::DepthTest, false);
  }
};
struct GuiDurabilityBar {
  CapScope caps{cap::Lighting, cap::Fog, cap::DepthTest, cap::Texture2D};
  GuiDurabilityBar() {
    setCap(cap::Lighting, false);
    setCap(cap::Fog, false);
    setCap(cap::DepthTest, false);
    setCap(cap::Texture2D, false);
  }
};
struct TranslucentTerrain {
  CapScope caps{cap::Blend, cap::CullFace};
  TranslucentTerrain() {
    setCap(cap::Blend, true);
    blendFunc(blend::SrcAlpha, blend::OneMinusSrcAlpha);
    setCap(cap::CullFace, false);
  }
};
struct BlockOverlay {
  CapScope caps{cap::AlphaTest};
  BlockOverlay() {
    setCap(cap::AlphaTest, false);
  }
};
struct SkyDomeDraw {
  CapScope caps{cap::Texture2D, cap::Fog, cap::AlphaTest, cap::Blend};
  DepthMaskScope depth;
  ShadeModelScope shade;
  BlendFuncScope blend;
  SkyDomeDraw() {
    setCap(cap::Texture2D, false);
    depthMask(false);
    setCap(cap::Fog, true);
    setCap(cap::AlphaTest, false);
    setCap(cap::Blend, true);
    blendFunc(blend::SrcAlpha, blend::OneMinusSrcAlpha);
  }
};
struct SkyDomeBackgroundFan {
  CapScope caps{cap::Fog, cap::AlphaTest, cap::Blend};
  SkyDomeBackgroundFan() {
    setCap(cap::Fog, false);
    setCap(cap::AlphaTest, false);
    blendAlpha();
  }
};
struct SkyDomeCelestial {
  CapScope caps{cap::Texture2D, cap::Fog, cap::Blend};
  BlendFuncScope blend;
  SkyDomeCelestial() {
    setCap(cap::Texture2D, true);
    setCap(cap::Fog, false);
    blendFunc(blend::SrcAlpha, blend::One);
  }
};
struct SkyDomeStarsPass {
  CapScope caps{cap::Texture2D, cap::Fog};
  SkyDomeStarsPass() {
    setCap(cap::Texture2D, false);
    setCap(cap::Fog, false);
  }
};
struct AchievementMap {
  CapScope caps{cap::Texture2D,
                cap::Lighting,
                cap::RescaleNormal,
                cap::ColorMaterial,
                cap::DepthTest,
                cap::Blend,
                cap::CullFace};
  DepthFuncScope depthFuncScope;
  AchievementMap() {
    gl::depthFunc(compare::Gequal);
    setCap(cap::Texture2D, true);
    setCap(cap::Lighting, false);
    setCap(cap::RescaleNormal, true);
    setCap(cap::ColorMaterial, true);
  }
};
struct AchievementMapLines {
  CapScope caps{cap::DepthTest, cap::Texture2D};
  DepthFuncScope depthFuncScope;
  AchievementMapLines() {
    setCap(cap::DepthTest, true);
    depthFunc(compare::Lequal);
    setCap(cap::Texture2D, false);
  }
};
struct AchievementMapIcons {
  CapScope caps{cap::Lighting, cap::RescaleNormal, cap::ColorMaterial, cap::Texture2D, cap::DepthTest, cap::Blend};
  DepthFuncScope depthFuncScope;
  BlendFuncScope blendScope;
  AchievementMapIcons() {
    setCap(cap::Lighting, false);
    setCap(cap::RescaleNormal, true);
    setCap(cap::ColorMaterial, true);
    setCap(cap::Texture2D, true);
    setCap(cap::DepthTest, true);
    setCap(cap::Blend, true);
    depthFunc(compare::Lequal);
    blendFunc(blend::SrcAlpha, blend::OneMinusSrcAlpha);
  }
};
struct AchievementIconItem {
  CapScope caps{cap::Lighting, cap::CullFace};
  AchievementIconItem() {
    setCap(cap::Lighting, true);
    setCap(cap::CullFace, true);
  }
};
struct AchievementMapFrame {
  CapScope caps{cap::DepthTest, cap::Blend, cap::Texture2D};
  AchievementMapFrame() {
    setCap(cap::DepthTest, false);
    setCap(cap::Blend, true);
    setCap(cap::Texture2D, true);
  }
};
struct ScreenTextOverlay {
  CapScope caps{cap::Lighting, cap::DepthTest, cap::Texture2D};
  ScreenTextOverlay() {
    setCap(cap::Lighting, false);
    setCap(cap::DepthTest, false);
    setCap(cap::Texture2D, true);
  }
};
struct ScreenFogOff {
  CapScope caps{cap::Fog};
  ScreenFogOff() {
    setCap(cap::Fog, false);
  }
};
using FogOff = ScreenFogOff;
struct ColorMaterialAmbient {
  CapScope caps{cap::ColorMaterial};
  ColorMaterialAmbient() {
    setCap(cap::ColorMaterial, true);
    colorMaterial(face::Front, light::Ambient);
  }
};
struct ClientInitGl {
  CapScope caps{cap::Texture2D, cap::DepthTest, cap::AlphaTest, cap::CullFace};
  ClientInitGl() {
    setCap(cap::Texture2D, true);
    shadeModel(shade::Smooth);
    setCap(cap::DepthTest, true);
    depthFunc(compare::Lequal);
    setCap(cap::AlphaTest, true);
    alphaFunc(compare::Greater, 0.1f);
    cullFace(face::Back);
  }
};
struct ListDraw {
  CapScope caps{cap::Fog, cap::DepthTest, cap::Texture2D};
  ListDraw() {
    setCap(cap::Fog, false);
    setCap(cap::DepthTest, false);
  }
};
struct ListScrollbar {
  CapScope caps{cap::Fog, cap::DepthTest, cap::Blend, cap::AlphaTest, cap::Texture2D};
  ShadeModelScope shade;
  ListScrollbar() {
    detail::alphaBlend(false);
    shadeModel(shade::Smooth);
  }
};
using ListRowHighlight = TextureOff;
struct EntityPassThrough {
  CapScope caps{cap::CullFace, cap::Blend, cap::Texture2D};
  EntityPassThrough() {
    setCap(cap::CullFace, false);
  }
};
struct EntityHurtOverlay {
  CapScope caps{cap::Texture2D, cap::Blend, cap::AlphaTest};
  DepthFuncScope depthFuncScope;
  EntityHurtOverlay() {
    setCap(cap::Texture2D, false);
    detail::alphaBlend(false);
    depthFunc(compare::Equal);
  }
};
struct BlockOutline {
  CapScope caps{cap::Blend, cap::Texture2D};
  DepthMaskScope depth;
  BlockOutline() {
    detail::alphaBlend(false);
  }
};
struct CloudDraw {
  CapScope caps{cap::CullFace, cap::Blend, cap::DepthTest, cap::Texture2D, cap::AlphaTest, cap::Fog};
  DepthMaskScope depth;
  DepthFuncScope depthFuncScope;
  CloudDraw() {
    setCap(cap::DepthTest, true);
    depthFunc(compare::Lequal);
    depthMask(true);
    setCap(cap::Texture2D, true);
    setCap(cap::CullFace, false);
    color4f(1.0f, 1.0f, 1.0f, 1.0f);
  }
};
struct ToastDraw {
  CapScope caps{
      cap::DepthTest, cap::Fog, cap::CullFace, cap::Texture2D, cap::Lighting, cap::RescaleNormal, cap::ColorMaterial};
  DepthMaskScope depth;
  ToastDraw() {
    setCap(cap::DepthTest, false);
    depthMask(false);
    setCap(cap::Fog, false);
    setCap(cap::CullFace, false);
    setCap(cap::Texture2D, true);
    setCap(cap::Lighting, false);
  }
};
struct ToastItemIcon {
  CapScope caps{cap::RescaleNormal, cap::ColorMaterial, cap::Lighting};
  ToastItemIcon() {
    setCap(cap::RescaleNormal, true);
    setCap(cap::ColorMaterial, true);
    setCap(cap::Lighting, true);
  }
};
struct DebugHud {
  CapScope caps{cap::Fog, cap::Texture2D};
  DebugHud() {
    setCap(cap::Fog, false);
    setCap(cap::Texture2D, true);
  }
};
struct ModItemDraw {
  CapScope caps{cap::Lighting, cap::Texture2D, cap::Blend, cap::CullFace};
  ModItemDraw(bool disableLighting = false) {
    detail::alphaBlend(false);
    setCap(cap::Lighting, !disableLighting);
    setCap(cap::CullFace, false);
  }
};
using ModBlockInventory = ModItemDraw;
struct FirstPersonDepth {
  CapScope caps{cap::DepthTest};
  DepthMaskScope depth;
  FirstPersonDepth() {
    setCap(cap::DepthTest, true);
    depthMask(true);
  }
};
struct HeldItemUnderwater {
  CapScope caps{cap::Blend};
  HeldItemUnderwater() {
    detail::alphaBlend(false);
  }
};
struct HeldItemFirstPerson {
  CapScope caps{cap::RescaleNormal, cap::AlphaTest, cap::Blend};
  HeldItemFirstPerson() {
    setCap(cap::RescaleNormal, true);
    detail::alphaBlend(false);
  }
};
using ProgressBarSolid = TextureOff;
struct LoadingScreenDraw {
  CapScope caps{cap::Texture2D, cap::AlphaTest};
  LoadingScreenDraw() {
    setCap(cap::Texture2D, true);
    setCap(cap::AlphaTest, false);
  }
};
struct ParticleDecal {
  CapScope caps{cap::Fog, cap::Blend};
  ParticleDecal() {
    setCap(cap::Fog, false);
    detail::alphaBlend(false);
  }
};
struct PrecipitationDraw {
  CapScope caps{cap::CullFace, cap::Blend};
  PrecipitationDraw() {
    setCap(cap::CullFace, false);
    detail::alphaBlend(false);
  }
};
struct PistonTranslucent {
  CapScope caps{cap::Blend, cap::CullFace};
  PistonTranslucent() {
    detail::alphaBlend(false);
    setCap(cap::CullFace, false);
  }
};
struct LightningFlash {
  CapScope caps{cap::Texture2D, cap::Blend};
  BlendFuncScope blend;
  LightningFlash() {
    setCap(cap::Texture2D, false);
    setCap(cap::Blend, true);
    blendFunc(blend::SrcAlpha, blend::One);
  }
};
struct TntFlash {
  CapScope caps{cap::Texture2D, cap::Blend};
  TntFlash() {
    setCap(cap::Texture2D, false);
    setCap(cap::Blend, true);
  }
};
using FishingLine = TextureOff;
using OutlineTextureOff = TextureOff;
struct DebugChart {
  CapScope caps{cap::CullFace, cap::Texture2D};
  DebugChart() {
    setCap(cap::CullFace, false);
    setCap(cap::Texture2D, false);
  }
};
struct GuiTextureOn {
  CapScope caps{cap::Texture2D};
  GuiTextureOn() {
    setCap(cap::Texture2D, true);
  }
};
} // namespace preset
} // namespace net::minecraft::client::gl
namespace gl = net::minecraft::client::gl;
