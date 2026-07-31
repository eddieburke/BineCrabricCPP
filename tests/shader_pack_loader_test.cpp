#include <gtest/gtest.h>
#include <algorithm>
#include <unordered_map>
#include "net/minecraft/client/render/shaderpack/ComputeDispatcher.hpp"
#include "net/minecraft/client/render/shaderpack/ShaderPackGl.hpp"
#include "net/minecraft/client/render/shaderpack/ShaderPackLoader.hpp"
namespace net::minecraft::client::render::shaderpack {
namespace {
bool load(const std::unordered_map<std::string, std::string>& sources,
          ShaderPackDefinition& pack,
          std::unordered_map<std::string, ShaderSourceOption>& options,
          std::string& error) {
 std::vector<std::string> paths;
 for(const auto& [path, ignored] : sources) paths.push_back(path);
 return ShaderPackLoader::load(paths, [&sources](std::string_view path) {
                              const auto found = sources.find(std::string(path));
                              return found == sources.end() ? std::string{} : found->second; }, pack, options, error);
}
} // namespace
TEST(ShaderPackLoaderTest, RejectsPacksWithoutPrograms) {
 ShaderPackDefinition pack;
 std::unordered_map<std::string, ShaderSourceOption> options;
 std::string error;
 EXPECT_FALSE(load({{"shaders/lib/common.glsl", ""}}, pack, options, error));
 EXPECT_NE(error.find("program"), std::string::npos);
}
TEST(ShaderPackLoaderTest, ResolvesTerrainFallback) {
 ShaderPackDefinition pack;
 std::unordered_map<std::string, ShaderSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "void main(){}"}},
                  pack,
                  options,
                  error));
 EXPECT_EQ(pack.programs.at("gbuffers_terrain").fragment, "shaders/gbuffers_basic.fsh");
}
TEST(ShaderPackLoaderTest, ReadsSourceOptions) {
 ShaderPackDefinition pack;
 std::unordered_map<std::string, ShaderSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "#define BLOOM\n#define QUALITY 2 // [1 2 3]\nvoid main(){}"}},
                  pack,
                  options,
                  error));
 EXPECT_EQ(options.at("BLOOM").setting.type, SettingType::Bool);
 EXPECT_EQ(options.at("QUALITY").setting.defaultValue, "2");
}
TEST(ShaderPackLoaderTest, ReadsShadowResolutionFromShaderSource) {
 ShaderPackDefinition pack;
 std::unordered_map<std::string, ShaderSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "const int shadowMapResolution = 2048;\nvoid main(){}"}},
                  pack,
                  options,
                  error));
 EXPECT_EQ(pack.shadowMapResolution, 2048);
 EXPECT_FALSE(options.contains("shadowMapResolution"));
}
TEST(ShaderPackLoaderTest, LoadsIrisProgramsAndFeatureFlags) {
 ShaderPackDefinition pack;
 std::unordered_map<std::string, ShaderSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "void main(){}"},
                   {"shaders/shadow.vsh", "void main(){}"},
                   {"shaders/shadow.fsh", "void main(){}"},
                   {"shaders/shadow_solid.vsh", "void main(){}"},
                   {"shaders/shadow_solid.fsh", "void main(){}"},
                   {"shaders/begin.csh", "layout(local_size_x = 8, local_size_y = 8) in;"},
                   {"shaders/prepare.fsh", "/* RENDERTARGETS: 2 */\nvoid main(){}"},
                   {"shaders/prepare.vsh", "void main(){}"},
                   {"shaders/shadowcomp.fsh", "/* RENDERTARGETS: 0,2 */\nvoid main(){}"},
                   {"shaders/shadowcomp.vsh", "void main(){}"},
                   {"shaders/shaders.properties",
                    "iris.features.required=COMPUTE_SHADERS SSBO\n"
                    "iris.features.optional=ENTITY_TRANSLUCENT BLOCK_EMISSION_ATTRIBUTE\n"}},
                  pack,
                  options,
                  error));
 EXPECT_TRUE(pack.programs.contains("shadow_solid"));
 EXPECT_TRUE(pack.requiredFeatures.contains("COMPUTE_SHADERS"));
 EXPECT_TRUE(pack.optionalFeatures.contains("BLOCK_EMISSION_ATTRIBUTE"));
 EXPECT_EQ(pack.shadowColorBuffers, 3);
 EXPECT_TRUE(std::any_of(pack.passes.begin(), pack.passes.end(),
                         [](const ShaderPass& pass) { return pass.type == "shadowcomp"; }));
 EXPECT_TRUE(std::any_of(pack.passes.begin(), pack.passes.end(),
                         [](const ShaderPass& pass) { return pass.type == "prepare"; }));
}
TEST(ShaderPackLoaderTest, UsesDocumentedProgramAndComputeNames) {
 ShaderPackDefinition pack;
 std::unordered_map<std::string, ShaderSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "void main(){}"},
                   {"shaders/final.csh", "layout(local_size_x = 1) in;"},
                   {"shaders/final1.vsh", "void main(){}"},
                   {"shaders/final1.fsh", "void main(){}"},
                   {"shaders/setup0.csh", "layout(local_size_x = 1) in;"},
                   {"shaders/setup1_a.csh", "layout(local_size_x = 1) in;"},
                   {"shaders/setup100.csh", "layout(local_size_x = 1) in;"},
                   {"shaders/gbuffers_entities_glowing.vsh", "void main(){}"},
                   {"shaders/gbuffers_entities_glowing.fsh", "void main(){}"}},
                  pack,
                  options,
                  error))
     << error;
 EXPECT_TRUE(pack.programs.contains("final#compute"));
 EXPECT_TRUE(pack.programs.contains("setup1_a#compute"));
 EXPECT_FALSE(pack.programs.contains("final1"));
 EXPECT_FALSE(pack.programs.contains("setup0#compute"));
 EXPECT_FALSE(pack.programs.contains("setup100#compute"));
 EXPECT_FALSE(pack.programs.contains("gbuffers_entities_glowing"));
}
TEST(ShaderPackLoaderTest, CurrentParticleOrderingOverridesLegacyRegardlessOfOrder) {
 for(const std::string properties : {
         "particles.ordering=mixed\nparticles.before.deferred=false\n",
         "particles.before.deferred=false\nparticles.ordering=mixed\n"}) {
  ShaderPackDefinition pack;
  std::unordered_map<std::string, ShaderSourceOption> options;
  std::string error;
  EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                    {"shaders/gbuffers_basic.fsh", "void main(){}"},
                    {"shaders/shaders.properties", properties}},
                   pack,
                   options,
                   error))
      << error;
  EXPECT_EQ(pack.particleOrdering, "mixed");
 }
}
TEST(ShaderPackLoaderTest, MatchesDocumentedStageNameGrammar) {
 EXPECT_TRUE(ComputeDispatcher::matchesStage("composite", "composite"));
 EXPECT_TRUE(ComputeDispatcher::matchesStage("composite1", "composite"));
 EXPECT_TRUE(ComputeDispatcher::matchesStage("composite99_z", "composite"));
 EXPECT_TRUE(ComputeDispatcher::matchesStage("final", "final"));
 EXPECT_TRUE(ComputeDispatcher::matchesStage("final_a", "final"));
 EXPECT_FALSE(ComputeDispatcher::matchesStage("composite0", "composite"));
 EXPECT_FALSE(ComputeDispatcher::matchesStage("composite100", "composite"));
 EXPECT_FALSE(ComputeDispatcher::matchesStage("composite1_A", "composite"));
 EXPECT_FALSE(ComputeDispatcher::matchesStage("final1", "final"));
 EXPECT_FALSE(ComputeDispatcher::matchesStage("voxelize", "composite"));
}
TEST(ShaderPackLoaderTest, ReadsFormatsClearFlipAndCustomTexture) {
 ShaderPackDefinition pack;
 std::unordered_map<std::string, ShaderSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh",
                    "const int colortex0Format = RGBA16F;\n"
                    "const bool colortex0Clear = false;\n"
                    "const vec4 colortex0ClearColor = vec4(1.0, 0.5, 0.25, 1.0);\n"
                    "/* RENDERTARGETS: 0,3 */\nvoid main(){}"},
                   {"shaders/shaders.properties",
                    "flip.composite.colortex0=false\n"
                    "customTexture.materialLut=textures/lut.png\n"}},
                  pack,
                  options,
                  error));
 EXPECT_EQ(pack.targets.at("colortex0").format, "RGBA16F");
 EXPECT_FALSE(pack.targets.at("colortex0").clear);
 EXPECT_TRUE(pack.targets.at("colortex0").customClearColor);
 EXPECT_FLOAT_EQ(pack.targets.at("colortex0").clearColor[1], 0.5f);
 EXPECT_FALSE(pack.flips.at("composite.colortex0"));
 ASSERT_EQ(pack.customTextures.size(), 1u);
 EXPECT_EQ(pack.customTextures.front().name, "materialLut");
 EXPECT_EQ(pack.gbufferColorBuffers, 4);
}
TEST(ShaderPackLoaderTest, ReadsDocumentedNoiseTextureConfiguration) {
 ShaderPackDefinition pack;
 std::unordered_map<std::string, ShaderSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "const int noiseTextureResolution = 512;\nvoid main(){}"},
                   {"shaders/gbuffers_basic.fsh", "void main(){}"},
                   {"shaders/shaders.properties", "texture.noise=textures/noise.png\n"}},
                  pack,
                  options,
                  error))
     << error;
 EXPECT_EQ(pack.noiseTextureResolution, 512);
 ASSERT_EQ(pack.customTextures.size(), 1u);
 EXPECT_EQ(pack.customTextures.front().name, "noisetex");
 EXPECT_EQ(pack.customTextures.front().path, "textures/noise.png");
 EXPECT_TRUE(pack.customTextures.front().encoded);
}
TEST(ShaderPackLoaderTest, LoadsLegacyDimensionFolders) {
 ShaderPackDefinition pack;
 std::unordered_map<std::string, ShaderSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "void main(){}"},
                   {"shaders/world-1/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/world-1/gbuffers_basic.fsh", "void main(){}"}},
                  pack,
                  options,
                  error));
 EXPECT_TRUE(pack.dimensionDefinitions.contains("minecraft:the_nether"));
 // Iris worldN legacy: base shaders/ programs are unused when any worldN folder exists.
 EXPECT_EQ(pack.dimensionDefinitions.at("minecraft:the_nether")->programs.at("gbuffers_basic").vertex,
           "shaders/world-1/gbuffers_basic.vsh");
}
TEST(ShaderPackLoaderTest, LoadsDimensionOnlyPackWithWildcard) {
 // RenderPearl-shaped: no root programs; dimension.world_default=*
 ShaderPackDefinition pack;
 std::unordered_map<std::string, ShaderSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/dimension.properties", "dimension.world_default=*\n"
                                                    "dimension.world_nether=minecraft:the_nether\n"},
                   {"shaders/world_default/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/world_default/gbuffers_basic.fsh", "void main(){}"},
                   {"shaders/world_default/composite.csh",
                    "layout(local_size_x = 8, local_size_y = 8) in;\nconst vec2 workGroupsRender = vec2(1.0, 1.0);"},
                   {"shaders/world_nether/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/world_nether/gbuffers_basic.fsh", "void main(){}"}},
                  pack,
                  options,
                  error))
     << error;
 EXPECT_TRUE(pack.dimensionDefinitions.contains("*"));
 EXPECT_TRUE(pack.dimensionDefinitions.contains("minecraft:the_nether"));
 EXPECT_FALSE(pack.programs.empty());
 EXPECT_EQ(pack.programs.at("gbuffers_basic").vertex, "shaders/world_default/gbuffers_basic.vsh");
 EXPECT_TRUE(pack.programs.contains("composite#compute"));
 EXPECT_EQ(pack.dimensionDefinitions.at("*")->programs.at("gbuffers_basic").fragment,
           "shaders/world_default/gbuffers_basic.fsh");
  EXPECT_EQ(pack.dimensionDefinitions.at("minecraft:the_nether")->programs.at("gbuffers_basic").fragment,
            "shaders/world_nether/gbuffers_basic.fsh");
}
TEST(ShaderPackLoaderTest, LoadsDimensionOnlyPackWithRootInclude) {
 ShaderPackDefinition pack;
 std::unordered_map<std::string, ShaderSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/dimension.properties", "dimension.world_default=*\n"},
                   {"shaders/prog/unlit.fsh", "void main(){}"},
                   {"shaders/world_default/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/world_default/gbuffers_basic.fsh", "#include \"/prog/unlit.fsh\"\n"}},
                  pack,
                  options,
                  error))
     << error;
}
TEST(ShaderPackLoaderTest, DimensionPropertiesMultiIdMapsEachKey) {
 ShaderPackDefinition pack;
 std::unordered_map<std::string, ShaderSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/dimension.properties",
                    "dimension.shared=minecraft:overworld minecraft:the_end\n"},
                   {"shaders/shared/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/shared/gbuffers_basic.fsh", "void main(){}"}},
                  pack,
                  options,
                  error))
     << error;
 EXPECT_TRUE(pack.dimensionDefinitions.contains("minecraft:overworld"));
 EXPECT_TRUE(pack.dimensionDefinitions.contains("minecraft:the_end"));
 EXPECT_EQ(pack.dimensionDefinitions.at("minecraft:overworld").get(),
           pack.dimensionDefinitions.at("minecraft:the_end").get());
}
TEST(ShaderPackLoaderTest, ResolvesLineToDedicatedProgramThenBasic) {
 ShaderPackDefinition pack;
 std::unordered_map<std::string, ShaderSourceOption> options;
 std::string error;
 // Iris: gbuffers_line → gbuffers_basic. Prefer dedicated line when present.
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "void main(){}"},
                   {"shaders/gbuffers_line.vsh", "void main(){}"},
                   {"shaders/gbuffers_line.fsh", "void main(){}"}},
                  pack,
                  options,
                  error));
 EXPECT_EQ(pack.programs.at("gbuffers_line").vertex, "shaders/gbuffers_line.vsh");
 EXPECT_EQ(pack.programs.at("gbuffers_line").fragment, "shaders/gbuffers_line.fsh");
}
TEST(ShaderPackLoaderTest, LineFallsBackToBasicWhenMissing) {
 ShaderPackDefinition pack;
 std::unordered_map<std::string, ShaderSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "void main(){}"}},
                  pack,
                  options,
                  error));
 EXPECT_EQ(pack.programs.at("gbuffers_line").fragment, "shaders/gbuffers_basic.fsh");
}
TEST(ShaderPackLoaderTest, MatchingPairFallbackDoesNotMixStages) {
 ShaderPackDefinition pack;
 std::unordered_map<std::string, ShaderSourceOption> options;
 std::string error;
 // Terrain fragment exists without a matching terrain vertex; textured has both.
 // Iris requires a matching pair — must not link terrain.fsh with textured.vsh.
 EXPECT_TRUE(load({{"shaders/gbuffers_textured.vsh", "void main(){}"},
                   {"shaders/gbuffers_textured.fsh", "void main(){}"},
                   {"shaders/gbuffers_terrain.fsh", "void main(){}"}},
                  pack,
                  options,
                  error));
 EXPECT_EQ(pack.programs.at("gbuffers_terrain").vertex, "shaders/gbuffers_textured.vsh");
 EXPECT_EQ(pack.programs.at("gbuffers_terrain").fragment, "shaders/gbuffers_textured.fsh");
}
TEST(ShaderPackLoaderTest, ReadsIrisGeometrySkipAndCullingDirectives) {
 ShaderPackDefinition pack;
 std::unordered_map<std::string, ShaderSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh",
                    "const float shadowDistance = 128.0;\n"
                    "const float shadowDistanceRenderMul = 1.0;\n"
                    "void main(){}"},
                   {"shaders/shaders.properties",
                    "shadowTerrain=false\n"
                    "shadowTranslucent=false\n"
                    "shadowBlockEntities=false\n"
                    "shadowEntities=false\n"
                    "skipAllRendering=true\n"
                    "shadow.culling=false\n"
                    "program.composite.enabled=false\n"
                    "program.deferred.enabled=BLOOM\n"
                    "clouds=false\n"}},
                  pack,
                  options,
                  error));
 EXPECT_FALSE(pack.shadowTerrain);
 EXPECT_FALSE(pack.shadowTranslucent);
 EXPECT_FALSE(pack.shadowBlockEntities);
 EXPECT_FALSE(pack.shadowEntities);
 EXPECT_TRUE(pack.skipAllRendering);
 EXPECT_FALSE(pack.shadowCulling);
 EXPECT_FALSE(pack.reversedShadowCulling);
 EXPECT_FALSE(pack.renderClouds);
 EXPECT_FLOAT_EQ(pack.shadowDistance, 128.0f);
 EXPECT_FLOAT_EQ(pack.shadowDistanceRenderMul, 1.0f);
 EXPECT_EQ(pack.programEnabled.at("composite"), "false");
 EXPECT_EQ(pack.programEnabled.at("deferred"), "BLOOM");
}
TEST(ShaderPackLoaderTest, ReadsReversedShadowCulling) {
 ShaderPackDefinition pack;
 std::unordered_map<std::string, ShaderSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "void main(){}"},
                   {"shaders/shaders.properties", "shadow.culling=reversed\n"}},
                  pack,
                  options,
                  error));
 EXPECT_TRUE(pack.shadowCulling);
 EXPECT_TRUE(pack.reversedShadowCulling);
}
TEST(ShaderPackLoaderTest, ReadsIrisFourFactorBlendAndCloudsOff) {
 ShaderPackDefinition pack;
 std::unordered_map<std::string, ShaderSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "void main(){}"},
                   {"shaders/shaders.properties",
                    "clouds=off\n"
                    "sun=false\n"
                    "underwaterOverlay=false\n"
                    "vignette=false\n"
                    "blend.gbuffers_terrain=ONE ZERO ONE ZERO\n"
                    "blend.gbuffers_water=off\n"
                    "flip.deferred_pre.colortex0=true\n"}},
                  pack,
                  options,
                  error));
 EXPECT_FALSE(pack.renderClouds);
 EXPECT_FALSE(pack.renderSun);
 EXPECT_FALSE(pack.underwaterOverlay);
 EXPECT_FALSE(pack.vignette);
 ASSERT_EQ(pack.bufferBlends.size(), 2u);
 const BufferBlend* terrain = nullptr;
 const BufferBlend* water = nullptr;
 for(const BufferBlend& blend : pack.bufferBlends) {
  if(blend.program == "gbuffers_terrain") terrain = &blend;
  if(blend.program == "gbuffers_water") water = &blend;
 }
 ASSERT_NE(terrain, nullptr);
 ASSERT_NE(water, nullptr);
 EXPECT_TRUE(terrain->enabled);
 EXPECT_EQ(terrain->source, "ONE");
 EXPECT_EQ(terrain->destination, "ZERO");
 EXPECT_EQ(terrain->sourceAlpha, "ONE");
 EXPECT_EQ(terrain->destinationAlpha, "ZERO");
 EXPECT_FALSE(water->enabled);
 EXPECT_TRUE(pack.flips.at("deferred_pre.colortex0"));
}
TEST(ShaderPackLoaderTest, PreprocessesMcVersionInBlockProperties) {
 ShaderPackDefinition pack;
 std::unordered_map<std::string, ShaderSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "void main(){}"},
                   {"shaders/block.properties",
                    "#if MC_VERSION >= 11400\n"
                    "block.1=modern_stone\n"
                    "#else\n"
                    "block.1=minecraft:stone\n"
                    "#endif\n"}},
                  pack,
                  options,
                  error));
 EXPECT_TRUE(pack.blockIds.contains("minecraft:stone"));
 EXPECT_FALSE(pack.blockIds.contains("modern_stone"));
 EXPECT_EQ(pack.blockIds.at("minecraft:stone"), 1);
}
TEST(ShaderPackLoaderTest, ReadsPerProgramMipmapEnabled) {
 ShaderPackDefinition pack;
 std::unordered_map<std::string, ShaderSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "void main(){}"},
                   {"shaders/composite1.vsh", "void main(){}"},
                   {"shaders/composite1.fsh",
                    "const bool colortex6MipmapEnabled = true;\n"
                    "/* DRAWBUFFERS:0 */\nvoid main(){}"}},
                  pack,
                  options,
                  error));
 const auto pass = std::find_if(pack.passes.begin(), pack.passes.end(),
                                [](const ShaderPass& p) { return p.name == "composite1"; });
 ASSERT_NE(pass, pack.passes.end());
 ASSERT_EQ(pass->mipmapBuffers.size(), 1u);
 EXPECT_EQ(pass->mipmapBuffers.front(), "colortex6");
}
TEST(ShaderPackLoaderTest, ReadsShadowHardwareFilteringConstants) {
 ShaderPackDefinition pack;
 std::unordered_map<std::string, ShaderSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh",
                    "const bool shadowHardwareFiltering0 = true;\n"
                    "const bool shadowHardwareFiltering1 = false;\n"
                    "const float sunPathRotation = -40.0f;\n"
                    "void main(){}"}},
                  pack,
                  options,
                  error));
 EXPECT_TRUE(pack.shadowHardwareFiltering[0]);
 EXPECT_FALSE(pack.shadowHardwareFiltering[1]);
 EXPECT_FLOAT_EQ(pack.sunPathRotation, -40.0f);
}
TEST(ShaderPackLoaderTest, ReadsExtendedPackConstants) {
 ShaderPackDefinition pack;
 std::unordered_map<std::string, ShaderSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh",
                    "const float eyeBrightnessHalflife = 5.0;\n"
                    "const float shadowIntervalSize = 0.5;\n"
                    "const float shadowNearPlane = 0.1;\n"
                    "const float shadowFarPlane = 512.0;\n"
                    "const float ambientOcclusionLevel = 0.5;\n"
                    "const bool generateShadowMipmap = true;\n"
                    "const bool shadowcolor0Mipmap = true;\n"
                    "const int shadowcolor0Format = RGBA16F;\n"
                    "const bool shadowcolor0Clear = false;\n"
                    "const vec4 shadowcolor0ClearColor = vec4(1.0, 1.0, 1.0, 1.0);\n"
                    "const int colortex20Format = R32F;\n"
                    "void main(){}"}},
                  pack,
                  options,
                  error));
 EXPECT_FLOAT_EQ(pack.eyeBrightnessHalflife, 5.0f);
 EXPECT_FLOAT_EQ(pack.shadowIntervalSize, 0.5f);
 EXPECT_FLOAT_EQ(pack.shadowNearPlane, 0.1f);
 EXPECT_FLOAT_EQ(pack.shadowFarPlane, 512.0f);
 EXPECT_FLOAT_EQ(pack.ambientOcclusionLevel, 0.5f);
 EXPECT_TRUE(pack.shadowtexMipmap[0]);
 EXPECT_TRUE(pack.shadowtexMipmap[1]);
 EXPECT_TRUE(pack.shadowcolorMipmap[0]);
 EXPECT_FALSE(pack.shadowcolorMipmap[1]);
 EXPECT_EQ(pack.targets.at("shadowcolor0").format, "RGBA16F");
 EXPECT_FALSE(pack.targets.at("shadowcolor0").clear);
 EXPECT_FLOAT_EQ(pack.targets.at("shadowcolor0").clearColor[0], 1.0f);
 EXPECT_EQ(pack.targets.at("colortex20").format, "R32F");
}
TEST(ShaderPackLoaderTest, ReadsSizeBufferScaleAndAlphaTest) {
 ShaderPackDefinition pack;
 std::unordered_map<std::string, ShaderSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "const float centerDepthHalflife = 2.5;\nvoid main(){}"},
                   {"shaders/shaders.properties",
                    "size.buffer.colortex5=0.5 0.25\n"
                    "size.buffer.colortex6=512 256\n"
                    "scale.composite=0.5 0.1 0.2\n"
                    "alphaTest.gbuffers_terrain=GREATER 0.1\n"
                    "alphaTest.gbuffers_basic=off\n"}},
                  pack,
                  options,
                  error));
 EXPECT_FLOAT_EQ(pack.targets.at("colortex5").scaleX, 0.5f);
 EXPECT_FLOAT_EQ(pack.targets.at("colortex5").scaleY, 0.25f);
 EXPECT_EQ(pack.targets.at("colortex6").absoluteWidth, 512);
 EXPECT_EQ(pack.targets.at("colortex6").absoluteHeight, 256);
 EXPECT_FLOAT_EQ(pack.programScales.at("composite").scale, 0.5f);
 EXPECT_FLOAT_EQ(pack.programScales.at("composite").offsetX, 0.1f);
 ASSERT_EQ(pack.alphaTests.size(), 2u);
 const AlphaTestDirective* terrainAlpha = nullptr;
 const AlphaTestDirective* basicAlpha = nullptr;
 for(const AlphaTestDirective& d : pack.alphaTests) {
  if(d.program == "gbuffers_terrain") terrainAlpha = &d;
  if(d.program == "gbuffers_basic") basicAlpha = &d;
 }
 ASSERT_NE(terrainAlpha, nullptr);
 ASSERT_NE(basicAlpha, nullptr);
 EXPECT_TRUE(terrainAlpha->enabled);
 EXPECT_EQ(terrainAlpha->func, "GREATER");
 EXPECT_FLOAT_EQ(terrainAlpha->ref, 0.1f);
 EXPECT_FALSE(basicAlpha->enabled);
 EXPECT_FLOAT_EQ(pack.centerDepthHalflife, 2.5f);
}
TEST(ShaderPackLoaderTest, ReadsRenderTargetsFromIncludedFragment) {
 ShaderPackDefinition pack;
 std::unordered_map<std::string, ShaderSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "void main(){}"},
                   {"shaders/gbuffers_terrain.vsh", "void main(){}"},
                   {"shaders/gbuffers_terrain.fsh", "#include \"prog/lit.fsh\"\nvoid main(){}"},
                   {"shaders/prog/lit.fsh", "/* RENDERTARGETS: 1,2 */\nvoid main(){}"}},
                  pack,
                  options,
                  error))
     << error;
 EXPECT_EQ(pack.gbufferColorBuffers, 3);
}
TEST(ShaderPackLoaderTest, ParseRenderTargetsAfterInactiveBranch) {
 const std::string source = R"(#if 0
/* RENDERTARGETS: 9 */
#endif
/* RENDERTARGETS: 1,2 */
void main() {}
)";
 const std::string preamble = "#version 330 compatibility\n";
 const std::string normalized = glutil::normalizePackSource(source, preamble);
 const std::vector<int> indices = glutil::parseRenderTargetIndices(normalized);
 ASSERT_EQ(indices.size(), 2u);
 EXPECT_EQ(indices[0], 1);
 EXPECT_EQ(indices[1], 2);
}
TEST(ShaderPackLoaderTest, ParseDrawBuffersDirective) {
 const std::vector<int> indices = glutil::parseRenderTargetIndices("/* DRAWBUFFERS: 0178 */\nvoid main(){}");
 ASSERT_EQ(indices.size(), 4u);
 EXPECT_EQ(indices[0], 0);
 EXPECT_EQ(indices[1], 1);
 EXPECT_EQ(indices[2], 7);
 EXPECT_EQ(indices[3], 8);
}
TEST(ShaderPackLoaderTest, DefaultRenderTargetIndices) {
 const std::vector<int> indices = glutil::defaultRenderTargetIndices();
 ASSERT_EQ(indices.size(), 8u);
 for(int i = 0; i < 8; ++i) {
  EXPECT_EQ(indices[static_cast<std::size_t>(i)], i);
 }
}
TEST(ShaderPackLoaderTest, NormalizePackSourcePreservesComments) {
 const std::string source = "/* RENDERTARGETS: 0,1 */\nvoid main() {}\n";
 const std::string preamble = "#version 330 compatibility\n";
 const std::string normalized = glutil::normalizePackSource(source, preamble);
 EXPECT_NE(normalized.find("/* RENDERTARGETS: 0,1 */"), std::string::npos);
 const std::vector<int> indices = glutil::parseRenderTargetIndices(normalized);
 ASSERT_EQ(indices.size(), 2u);
 EXPECT_EQ(indices[0], 0);
 EXPECT_EQ(indices[1], 1);
}
TEST(ShaderPackLoaderTest, IgnoresFormatDirectivesInsideBlockComments) {
 ShaderPackDefinition pack;
 std::unordered_map<std::string, ShaderSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh",
                    "/* const int colortex0Format = RGBA16F; */\n"
                    "const int colortex0Format = RGBA8;\n"
                    "void main(){}"}},
                  pack,
                  options,
                  error))
     << error;
 EXPECT_EQ(pack.targets.at("colortex0").format, "RGBA8");
}
TEST(ShaderPackLoaderTest, ReadsScreenSlidersProfilesAndPackToggles) {
 ShaderPackDefinition pack;
 std::unordered_map<std::string, ShaderSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh",
                    "#define FOO 1 //[0 1]\n"
                    "#define BAR 0.5 //[0.0 1.0]\n"
                    "void main(){}"},
                   {"shaders/shaders.properties",
                    "sky=false\n"
                    "stars=false\n"
                    "weather=false\n"
                    "oldHandLight=true\n"
                    "shadowLightBlockEntities=false\n"
                    "rain.depth=true\n"
                    "frustum.culling=false\n"
                    "occlusion.culling=false\n"
                    "backFace.solid=false\n"
                    "sliders=BAR\n"
                    "screen=<profile> [LIGHTING] FOO\n"
                    "screen.LIGHTING=BAR\n"
                    "profile.Low=FOO:0 BAR:0.1\n"}},
                  pack,
                  options,
                  error))
     << error;
 EXPECT_FALSE(pack.renderSky);
 EXPECT_FALSE(pack.renderStars);
 EXPECT_FALSE(pack.renderWeather);
 EXPECT_TRUE(pack.oldHandLight);
 EXPECT_FALSE(pack.shadowLightBlockEntities);
 EXPECT_TRUE(pack.rainDepth);
 EXPECT_FALSE(pack.frustumCulling);
 EXPECT_FALSE(pack.occlusionCulling);
 EXPECT_FALSE(pack.backFaceSolid);
 EXPECT_TRUE(pack.sliderKeys.count("BAR") != 0);
 ASSERT_EQ(pack.screenRoot.size(), 3u);
 EXPECT_EQ(pack.screenRoot[0], "<profile>");
 ASSERT_EQ(pack.screenPages["LIGHTING"].size(), 1u);
 ASSERT_EQ(pack.profiles.size(), 1u);
 EXPECT_EQ(pack.profiles[0].name, "Low");
 EXPECT_EQ(pack.profiles[0].values.at("FOO"), "0");
 bool barSlider = false;
 for(const PackSetting& setting : pack.settings) {
  if(setting.key == "BAR") barSlider = setting.asSlider;
 }
 EXPECT_TRUE(barSlider);
}
// RenderPearl has both `#define immut const` and a bodyless `#define immut`.
// Treating that name as a boolean option and rewriting it turns every
// `immut vec3 x = ...;` declaration into garbage, so the name must be dropped.
TEST(ShaderPackLoaderTest, DropsDefinesUsedWithConflictingShapes) {
 ShaderPackDefinition pack;
 std::unordered_map<std::string, ShaderSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh",
                    "#define immut const\n"
                    "#define immut\n"
                    "#define BLOOM\n"
                    "void main(){}"}},
                  pack,
                  options,
                  error))
     << error;
 EXPECT_FALSE(options.contains("immut"));
 EXPECT_TRUE(options.contains("BLOOM"));
 const std::string rewritten =
     ShaderPackLoader::rewriteOptions("#define immut const\n#define immut\n", options, {{"immut", "1"}});
 EXPECT_EQ(rewritten, "#define immut const\n#define immut\n");
}
TEST(ShaderPackLoaderTest, BooleanOptionRewriteKeepsMacroBodyless) {
 ShaderPackDefinition pack;
 std::unordered_map<std::string, ShaderSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "#define BLOOM // glow\nvoid main(){}"}},
                  pack,
                  options,
                  error))
     << error;
 EXPECT_EQ(ShaderPackLoader::rewriteOptions("#define BLOOM // glow\n", options, {{"BLOOM", "1"}}),
           "#define BLOOM // glow\n");
 EXPECT_EQ(ShaderPackLoader::rewriteOptions("#define BLOOM // glow\n", options, {{"BLOOM", "0"}}),
           "//#define BLOOM // glow\n");
}
TEST(ShaderPackSourcePreparation, CompatibilityGbuffersGetsEndOfMainDiscard) {
 ShaderPackDefinition pack;
 const std::string src = R"(#version 330 compatibility
void main() {
	gl_FragData[0] = vec4(1.0);
}
)";
 const std::string out = glutil::prepareSource(
     "gbuffers_terrain", glutil::ShaderStage::Fragment, pack, src, "#version 330 core\n");
 EXPECT_NE(out.find("uniform float alphaTestRef"), std::string::npos);
 EXPECT_NE(out.find("gl_FragData[0].a > alphaTestRef"), std::string::npos);
 const std::size_t write = out.find("gl_FragData[0] = vec4");
 const std::size_t discard = out.find("discard");
 ASSERT_NE(write, std::string::npos);
 ASSERT_NE(discard, std::string::npos);
 EXPECT_LT(write, discard);
}
TEST(ShaderPackSourcePreparation, ModernOutputsAndPostPassesDoNotGainAlphaTest) {
 ShaderPackDefinition pack;
 const std::string core = R"(#version 430 core
layout(location=0) out vec4 outColor;
void main() { outColor = vec4(1.0); }
)";
 EXPECT_EQ(glutil::prepareSource(
               "gbuffers_terrain", glutil::ShaderStage::Fragment, pack, core, "#version 430 core\n")
               .find("discard"),
           std::string::npos);
 const std::string compat = R"(#version 330 compatibility
void main() { gl_FragData[0] = vec4(1.0); }
)";
 EXPECT_EQ(glutil::prepareSource(
               "composite", glutil::ShaderStage::Fragment, pack, compat, "#version 330 core\n")
               .find("discard"),
           std::string::npos);
 EXPECT_EQ(glutil::prepareSource(
               "gbuffers_water", glutil::ShaderStage::Fragment, pack, compat, "#version 330 core\n")
               .find("discard"),
           std::string::npos);
}
TEST(ShaderPackSourcePreparation, ChunkFadeMatchesTheAdvertisedFeatureAbi) {
 ShaderPackDefinition pack;
 pack.optionalFeatures.insert("FADE_VARIABLE");
 const std::string source =
     "#version 430 compatibility\nvoid main() { float fade = mc_chunkFade; }\n";
 const auto prepare = [&](const std::string& program) {
  return glutil::prepareSource(
      program, glutil::ShaderStage::Vertex, pack, source, "#version 430 core\n");
 };
 EXPECT_NE(prepare("gbuffers_terrain").find("in float mc_chunkFade;"), std::string::npos);
 EXPECT_NE(prepare("gbuffers_entities").find("const float mc_chunkFade = -1.0;"),
           std::string::npos);
 EXPECT_EQ(prepare("shadow").find("mc_chunkFade ="), std::string::npos);
}
TEST(ShaderPackLoaderTest, InfersColortexFormatsFromImageAndUsamplerLayouts) {
 // RenderPearl leaves const colortexNFormat commented; formats come from layouts.
 // https://github.com/Luracasmus/renderpearl/blob/main/DEV.md
 // https://www.khronos.org/opengl/wiki/Image_Load_Store
 ShaderPackDefinition pack;
 std::unordered_map<std::string, ShaderSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "void main(){}"},
                   {"shaders/deferred.csh",
                    "layout(local_size_x = 8) in;\n"
                    "uniform usampler2D colortex2;\n"
                    "uniform layout(rgba16f) restrict image2D colorimg1;\n"
                    "void main(){}\n"},
                   {"shaders/gbuffers_entities.vsh", "void main(){}"},
                   {"shaders/gbuffers_entities.fsh",
                    "/* RENDERTARGETS: 1,2 */\n"
                    "layout(location = 0) out f16vec4 colortex1;\n"
                    "layout(location = 1) out uvec4 colortex2;\n"
                    "void main(){}\n"}},
                  pack,
                  options,
                  error))
     << error;
 ASSERT_TRUE(pack.targets.contains("colortex1"));
 ASSERT_TRUE(pack.targets.contains("colortex2"));
 EXPECT_EQ(pack.targets.at("colortex1").format, "RGBA16F");
 EXPECT_EQ(pack.targets.at("colortex2").format, "RGBA32UI");
 EXPECT_GE(pack.gbufferColorBuffers, 3);
}
} // namespace net::minecraft::client::render::shaderpack
