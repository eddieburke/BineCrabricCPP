#pragma once
#include <functional>
#include <string>
#include <memory>
#include "net/minecraft/client/gl/ShaderProgram.hpp"
#include "net/minecraft/client/render/RenderSystem.hpp"
namespace net::minecraft::client::gl {
class ShaderProgram;
}
namespace net::minecraft::client::render {
// Resolves a canonical world program key (e.g. "gbuffers_terrain") to a compiled program,
// installed by ShaderPackManager. World RenderTypes call it every setupRenderState so the
// live pack (or the vanilla base pack) owns their shader. nullptr resolver / result means
// no program is bound. Set to nullptr on manager teardown.
using WorldProgramResolver = std::function<gl::ShaderProgram*(const std::string& key)>;
void setWorldProgramResolver(WorldProgramResolver resolver);
// A rendering pass: a dedicated shader program plus the GL state it expects. Instances
// are immutable singletons (solid/cutout/translucent/gui/...). To draw with one, open a
// RenderPassScope — it binds the program and applies the GL state on construction and
// restores the previous state on destruction. Callers NEVER call setup/clear directly.
class RenderType {
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
 RenderType(std::string name, int glMode, bool hasTexture, bool hasColor, bool hasNormals,
            bool hasLightmap, std::string programName, State state,
            std::string worldProgramKey = "");
 virtual ~RenderType() = default;
 void setupRenderState() const;
 void restoreRenderState(const RenderSystem::StateShadow& saved, gl::ShaderProgram* savedProgram) const;
 [[nodiscard]] std::string name() const { return name_; }
 [[nodiscard]] int glMode() const { return glMode_; }
 [[nodiscard]] bool hasTexture() const { return hasTexture_; }
 [[nodiscard]] bool hasColor() const { return hasColor_; }
 [[nodiscard]] bool hasNormals() const { return hasNormals_; }
 [[nodiscard]] bool hasLightmap() const { return hasLightmap_; }
 [[nodiscard]] gl::ShaderProgram* program() const { return program_; }
 static RenderType& solid();
 static RenderType& cutout();
 static RenderType& translucent();
 static RenderType& gui();
 static RenderType& guiTextured();
 static RenderType& guiItem3D();
 static RenderType& text();
 static RenderType& entityCutout();
 static RenderType& sky();

 protected:
 static gl::ShaderProgram* loadProgram(const std::string& baseName);
 std::string name_;
 int glMode_;
 bool hasTexture_;
 bool hasColor_;
 bool hasNormals_;
 bool hasLightmap_;
 std::string programName_;
 State state_;
 // When non-empty, the program is resolved through the world program resolver each pass
 // (pack-driven); when empty, programName_ names an engine-internal program (gui/text).
 std::string worldProgramKey_;
 mutable gl::ShaderProgram* program_ = nullptr;
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
 RenderSystem::StateShadow saved_;
 gl::ShaderProgram* savedProgram_ = nullptr;
};
} // namespace net::minecraft::client::render
