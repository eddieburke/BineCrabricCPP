#pragma once
#include <string>
#include <string_view>
#include "net/minecraft/client/render/shaders/ShaderTransform.hpp"
namespace net::minecraft::client::render {
// ColorWheel is an Iris addon that lets a shaderpack render terrain/entities in
// parallel "material" gbuffer programs (clrwl_gbuffers, clrwl_gbuffers_translucent,
// clrwl_gbuffers_damagedblock, clrwl_shadow, ...). The pack's own clrwl_* sources
// are written against the addon's FlwMaterial plumbing; mergeColorWheelMaterial
// merges that plumbing into them so the engine can compile them like any other
// program. Packs that do not ship clrwl_* programs are untouched.
[[nodiscard]] std::string mergeColorWheelMaterial(const std::string& programName, ShaderStage stage,
                                                  std::string source);
} // namespace net::minecraft::client::render
