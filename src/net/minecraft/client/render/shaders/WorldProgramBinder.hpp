#pragma once
#include "net/minecraft/client/gl/ShaderProgram.hpp"
#include "net/minecraft/client/render/uniforms/Uniforms.hpp"
namespace net::minecraft::client::render {
class ColorTargets;
struct WorldProgramBindContext {
 const PackUniformValues* uniforms = nullptr;
 unsigned int lightmapTexture = 0;
 unsigned int normalTexture = 0;
 unsigned int specularTexture = 0;
 unsigned int overlayTexture = 0;
 const ColorTargets* sceneTargets = nullptr;
 int sceneDepthTexture = -1;
 int opaqueDepthTexture = -1;
 int handDepthTexture = -1;
 int atlasWidth = 1;
 int atlasHeight = 1;
 int shadowDepthTexture = -1;
 int shadowOpaqueDepthTexture = -1;
 const int* shadowColorTextures = nullptr;
 int shadowColorTextureCount = 0;
 bool bindTextureAtlases = false;
 bool clearShadowBindsWhenNoPack = false;
 class PackInstance* pack = nullptr;
};
void bindWorldProgram(gl::ShaderProgram& program, const WorldProgramBindContext& context);
void bindProgramMaterialTextures(gl::ShaderProgram& program, const WorldProgramBindContext& context);
} // namespace net::minecraft::client::render
