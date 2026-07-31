#pragma once
// Single glutil surface: GlState + IncludeResolver + SourceProcessor.
#include "net/minecraft/client/render/GlState.hpp"
#include "net/minecraft/client/render/IncludeResolver.hpp"
#include "net/minecraft/client/render/SourceProcessor.hpp"

namespace net::minecraft::client::render::shaderpack {
namespace glutil {
using render::glutil::kTexture2D;
using render::glutil::kTexture3D;
using render::glutil::ShaderReadText;
using render::glutil::ShaderStage;
using render::glutil::hasGlContext;
using render::glutil::maxTextureUnits;
using render::glutil::releaseSamplers;
using render::glutil::featureSupported;
using render::glutil::parseFormat;
using render::glutil::colorFormatName;
using render::glutil::pixelFormat;
using render::glutil::pixelType;
using render::glutil::internalFormat;
using render::glutil::textureTarget;
using render::glutil::blendFactor;
using render::glutil::applyAlphaTest;
using render::glutil::normalizeSettingValue;
using render::glutil::resolveShaderIncludes;
using render::glutil::isBufferFormatDirective;
using render::glutil::versionPreamble;
using render::glutil::normalizePackSource;
using render::glutil::prepareSource;
using render::glutil::isCompositeStyleProgramName;
using render::glutil::defaultCompositeVertexShader;
using render::glutil::parseRenderTargetIndices;
using render::glutil::defaultRenderTargetIndices;
using render::glutil::renderTargetOutputNames;

inline unsigned int samplerObject(bool compare) {
 return render::glutil::samplerObject(compare);
}

inline void bindSamplers(gl::ShaderProgram& program,
                         const std::unordered_map<std::string, int>& textures,
                         const std::unordered_map<std::string, int>& volumeTextures,
                         int maxUnits) {
 render::glutil::bindSamplers(program, textures, volumeTextures, maxUnits, nullptr);
}

inline void refreshTextureAliases(std::unordered_map<std::string, int>& textures) {
 render::glutil::refreshTextureAliases(textures, false);
}

inline unsigned int bindColorImages(gl::ShaderProgram& program,
                                    const std::unordered_map<std::string, int>& colorTextures,
                                    const ShaderPackDefinition* definition = nullptr) {
 return render::glutil::bindColorImages(program, colorTextures, definition, nullptr);
}

inline void applyBufferBlends(const ShaderPackDefinition& pack, const std::string& program) {
 render::glutil::applyBufferBlends(pack, program);
}
} // namespace glutil
} // namespace net::minecraft::client::render::shaderpack
