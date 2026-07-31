#pragma once
#include "net/minecraft/client/gl/ShaderProgram.hpp"
#include "net/minecraft/client/render/shaderpack/ShaderUniforms.hpp"
namespace net::minecraft::client::render::shaderpack {
struct WorldProgramBindContext {
 const FrameUniformSet* uniforms = nullptr;
 unsigned int* lightmapTexture = nullptr;
 unsigned int normalTexture = 0;
 unsigned int specularTexture = 0;
 unsigned int noiseTexture = 0;
 int atlasWidth = 1;
 int atlasHeight = 1;
 int shadowDepthTexture = -1;
 int shadowOpaqueDepthTexture = -1;
 const int* shadowColorTextures = nullptr;
 int shadowColorTextureCount = 0;
 bool bindTextureAtlases = false;
 bool clearShadowBindsWhenNoPack = false;
 class ShaderPackInstance* pack = nullptr;
};
void bindWorldProgram(gl::ShaderProgram& program, const WorldProgramBindContext& context);
} // namespace net::minecraft::client::render::shaderpack
