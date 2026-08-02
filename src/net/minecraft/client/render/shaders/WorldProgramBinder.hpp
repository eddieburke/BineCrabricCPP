#pragma once
#include "net/minecraft/client/gl/ShaderProgram.hpp"
#include "net/minecraft/client/render/uniforms/Uniforms.hpp"
namespace net::minecraft::client::render {
struct WorldProgramBindContext {
 const PackUniformValues* uniforms = nullptr;
 unsigned int* lightmapTexture = nullptr;
 unsigned int normalTexture = 0;
 unsigned int specularTexture = 0;
 unsigned int noiseTexture = 0;
 unsigned int overlayTexture = 0;
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
} // namespace net::minecraft::client::render
