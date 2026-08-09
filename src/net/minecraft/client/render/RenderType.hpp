#pragma once
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include "net/minecraft/client/gl/ShaderProgram.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/shaders/WorldProgramId.hpp"
namespace net::minecraft::client::gl {
class ShaderProgram;
}
namespace net::minecraft::client::render {
int resolveShaderObjectId(const std::string& kind, const std::string& name, int fallback = 0);
std::uint64_t shaderObjectIdRevision();
void setShaderBlockIds(const std::array<int, 256>& ids);
int resolveShaderBlockId(int id);
enum class DrawPhase {
 All,
 Opaque,
 Translucent
};
void setDrawPhase(DrawPhase phase);
// A rendering pass: a dedicated shader program plus the GL state it expects. Instances
// are immutable singletons (solid/cutout/translucent/gui/...). To draw with one, open a
// RenderPassScope — it binds the program and applies the GL state on construction and
// restores the previous state on destruction. Callers NEVER call setup/clear directly.
class RenderType {
 friend class RenderPassScope;

 public:
 // GL state description applied by setupRenderState(). Data-driven so the concrete
 // pass subclasses carry no duplicated code.
 struct State {
  bool blend = false;
  int blendSrc = 0x0302; // GL_SRC_ALPHA
  int blendDst = 0x0303; // GL_ONE_MINUS_SRC_ALPHA
  bool depthTest = false;
  bool depthWrite = true;
  bool cull = false;
  int cullMode = 0x0405; // GL_BACK
  bool alphaTest = false;
  float alphaRef = 0.1f;
  bool lighting = false;
 };
  RenderType(std::string_view name,
             int glMode,
             bool hasTexture,
             std::string_view programName,
             State state,
             std::optional<WorldProgramId> worldProgram = std::nullopt);
  virtual ~RenderType() = default;
  void setupRenderState() const;
  void restoreRenderState(const core::PassGlBits& saved,
                          gl::ShaderProgram* savedProgram,
                          std::optional<WorldProgramId> savedWorldProgram) const;
  [[nodiscard]] std::string_view name() const {
   return name_;
  }
 [[nodiscard]] int glMode() const {
  return glMode_;
 }
 [[nodiscard]] bool hasTexture() const {
  return hasTexture_;
 }
 static RenderType& solid();
 static RenderType& cutout();
 static RenderType& cutoutInterior();
 static RenderType& translucent();
 static RenderType& gui();
 static RenderType& guiTextured();
 static RenderType& guiItem3D();
 static RenderType& text();
 static RenderType& entityCutout();
 static RenderType& entityTranslucent();
 static RenderType& lightning();
 static RenderType& block();
 static RenderType& blockTranslucent();
 static RenderType& particles();
 static RenderType& particlesTranslucent();
 static RenderType& sky();
 static RenderType& skyTextured();
 static RenderType& basic();
 static RenderType& lines();
 static RenderType& clouds();
 static RenderType& weather();
 static RenderType& hand();
 static RenderType& damagedBlock();

 protected:
  std::string_view name_;
  int glMode_;
  bool hasTexture_;
  std::string_view programName_;
  State state_;
  // When non-empty, the program is resolved through the world program resolver each pass
  // (pack-driven); when empty, no program is bound.
  std::optional<WorldProgramId> worldProgram_;
};
// RAII guard: opens a pass for the lifetime of the scope. This is the ONLY supported way
// to use a RenderType for immediate-mode drawing. Mandatory restore, zero boilerplate.
class RenderPassScope {
 public:
 explicit RenderPassScope(const RenderType& type);
 ~RenderPassScope();
 RenderPassScope(const RenderPassScope&) = delete;
 RenderPassScope& operator=(const RenderPassScope&) = delete;

 private:
 const RenderType* type_;
 core::PassGlBits saved_;
 gl::ShaderProgram* savedProgram_ = nullptr;
 std::optional<WorldProgramId> savedWorldProgram_;
 bool savedDrawEnabled_ = true;
 core::RenderStage savedRenderStage_ = core::RenderStage::None;
 net::minecraft::client::render::core::WorldLightUniforms savedWorldLight_{};
 float savedConstColor_[4] = {1.0f, 1.0f, 1.0f, 1.0f};
 float savedAlphaTestRef_ = 0.1f;
 net::minecraft::util::math::Matrix4f savedPose_{};
};
} // namespace net::minecraft::client::render
