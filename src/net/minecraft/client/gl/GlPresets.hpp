#pragma once
#include "net/minecraft/client/gl/GlScopes.hpp"
namespace net::minecraft::client::gl {
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
                cap::ColorMaterial,
                light::Light0,
                light::Light1};
  ModLuaGuiItemDraw() {
    setCap(cap::Fog, false);
    setCap(cap::Blend, false);
    setCap(cap::Texture2D, true);
    setCap(cap::RescaleNormal, true);
    setCap(cap::DepthTest, true);
    setCap(cap::Lighting, true);
    setCap(cap::ColorMaterial, true);
    setCap(light::Light0, true);
    setCap(light::Light1, true);
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
struct GuiItemDurability {
  CapScope caps{cap::Lighting, cap::Texture2D};
  GuiItemDurability() {
    setCap(cap::Lighting, false);
    setCap(cap::Texture2D, false);
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
struct SkyDomeHorizon {
  CapScope caps{cap::Blend, cap::AlphaTest, cap::Fog, cap::Texture2D};
  DepthMaskScope depth;
  SkyDomeHorizon() {
    setCap(cap::Blend, false);
    setCap(cap::AlphaTest, true);
    setCap(cap::Fog, true);
    setCap(cap::Texture2D, false);
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
struct FogOn {
  CapScope caps{cap::Fog};
  FogOn() {
    setCap(cap::Fog, true);
  }
};
struct ColorMaterialOn {
  CapScope caps{cap::ColorMaterial};
  ColorMaterialOn() {
    setCap(cap::ColorMaterial, true);
    colorMaterial(face::Front, light::AmbientAndDiffuse);
  }
};
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
struct EntityPassRestore {
  CapScope caps{cap::Blend, cap::CullFace};
  EntityPassRestore() {
    setCap(cap::Blend, false);
    setCap(cap::CullFace, true);
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
struct FrameSetup {
  CapScope caps{cap::CullFace, cap::DepthTest, cap::Fog, cap::Blend};
  DepthMaskScope depth;
  ShadeModelScope shade;
  FrameSetup() {
    setCap(cap::CullFace, true);
    setCap(cap::DepthTest, true);
    depthMask(true);
  }
};
struct FrameEndCleanup {
  CapScope caps{cap::Fog, cap::CullFace, cap::Blend, cap::AlphaTest};
  FrameEndCleanup() {
    setCap(cap::Fog, false);
    setCap(cap::CullFace, true);
    setCap(cap::Blend, false);
    alphaFunc(compare::Greater, 0.1f);
    color4f(1.0f, 1.0f, 1.0f, 1.0f);
  }
};
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
struct CreeperChargeBlend {
  CapScope caps{cap::Blend};
  BlendFuncScope blend;
  CreeperChargeBlend() {
    setCap(cap::Blend, true);
    blendFunc(blend::OneMinusDstColor, blend::OneMinusSrcColor);
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
struct SlimeOuterShell {
  CapScope caps{cap::Normalize, cap::Blend};
  SlimeOuterShell() {
    setCap(cap::Normalize, true);
    detail::alphaBlend(false);
  }
};
struct SlimeInnerRestore {
  CapScope caps{cap::Normalize, cap::Blend};
  SlimeInnerRestore() {
    setCap(cap::Normalize, false);
    setCap(cap::Blend, false);
  }
};
using FishingLine = TextureOff;
struct ChunkTranslucentDraw {
  CapScope caps{cap::Blend, cap::Texture2D};
  ChunkTranslucentDraw() {
    detail::alphaBlend(false);
  }
};
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
