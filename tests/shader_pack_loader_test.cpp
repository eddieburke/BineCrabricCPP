#include <gtest/gtest.h>
#include <algorithm>
#include <unordered_map>
#include "net/minecraft/client/render/shaders/ComputeDispatcher.hpp"
#include "net/minecraft/client/render/GlState.hpp"
#include "net/minecraft/client/render/shaders/CoreGlslTransformer.hpp"
#include "net/minecraft/client/render/shaders/IncludeResolver.hpp"
#include "net/minecraft/client/render/shaders/PassIndex.hpp"
#include "net/minecraft/client/render/shaders/SourceProcessor.hpp"
#include "net/minecraft/client/render/shaderpack/Loader.hpp"
#include "net/minecraft/client/render/pipeline/Pipeline.hpp"
#include "net/minecraft/client/render/RenderType.hpp"
#include "support/glsl_snippets_test_fixture.hpp"
namespace net::minecraft::client::render {
namespace {
bool load(const std::unordered_map<std::string, std::string>& sources,
          PackDefinition& pack,
          std::unordered_map<std::string, PackSourceOption>& options,
          std::string& error,
          const std::unordered_map<std::string, std::string>& values = {}) {
 std::vector<std::string> paths;
 for(const auto& [path, ignored] : sources) paths.push_back(path);
 return PackLoader::load(paths, [&sources](std::string_view path) {
                              const auto found = sources.find(std::string(path));
                              return found == sources.end() ? std::string{} : found->second; }, pack, options, error, values);
}
std::string resolved(const PackDefinition& pack, const std::string& key) {
 ProgramEnabledCache cache;
 return resolveProgramKey(pack, {}, key, cache);
}
} // namespace
TEST(PackLoaderTest, CoreGlslTransformerInjectsBeforeNormalizedMain) {
 PackDefinition pack;
 ShaderTransformContext ctx{false, false, false, false};
 std::string source =
     "#version 430 compatibility\n"
     "#define VERTEX_SHADER\n"
     "//////Fragment Shader//////Fragment Shader//////\n"
     "#ifdef FRAGMENT_SHADER\n"
     "void main() { gl_FragData[0] = vec4(1.0); }\n"
     "#endif\n"
     "//////Vertex Shader//////Vertex Shader//////\n"
     "#ifdef VERTEX_SHADER\n"
     "void main() { gl_Position = ftransform(); }\n"
     "#endif\n";
 std::string result = prepareSource("prepare", ShaderStage::Vertex, pack, source, ctx);
 const std::size_t declPos = result.find("uniform mat4 projectionMatrix;");
 const std::size_t mainPos = result.find("void main()");
 EXPECT_NE(declPos, std::string::npos);
 EXPECT_NE(mainPos, std::string::npos);
 EXPECT_LT(declPos, mainPos);
 EXPECT_EQ(result.find("#ifdef FRAGMENT_SHADER"), std::string::npos);
 EXPECT_EQ(result.find("gl_FragData"), std::string::npos);
}
TEST(PackLoaderTest, ExpandsSharedIncludesOnce) {
 std::unordered_map<std::string, int> reads;
 const auto readText = [&reads](std::string_view path) {
  ++reads[std::string(path)];
  if(path == "shaders/lib/common.glsl") return std::string("float light;\n");
  return std::string("#include \"lib/common.glsl\"\nvoid main(){}\n");
 };
 std::unordered_map<std::string, std::string> memo;
 const std::string first = resolveShaderIncludes(readText, "shaders/gbuffers_a.fsh", false, memo);
 const std::string second = resolveShaderIncludes(readText, "shaders/gbuffers_b.fsh", false, memo);
 EXPECT_EQ(first, "float light;\nvoid main(){}\n");
 EXPECT_EQ(second, first);
 EXPECT_EQ(reads["shaders/lib/common.glsl"], 1);
}
TEST(PackLoaderTest, RejectsPacksWithoutPrograms) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 EXPECT_FALSE(load({{"shaders/lib/common.glsl", ""}}, pack, options, error));
 EXPECT_NE(error.find("program"), std::string::npos);
}
TEST(PackLoaderTest, ResolvesTerrainFallback) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "void main(){}"}},
                  pack,
                  options,
                  error));
 EXPECT_FALSE(pack.programs.contains("gbuffers_terrain"));
 EXPECT_EQ(resolved(pack, "gbuffers_terrain"), "gbuffers_basic");
}
TEST(PackLoaderTest, ReadsSourceOptions) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "#define BLOOM\n#define QUALITY 2 // [1 2 3]\nvoid main(){}"}},
                  pack,
                  options,
                  error));
 EXPECT_EQ(options.at("BLOOM").setting.type, SettingType::Bool);
 EXPECT_EQ(options.at("QUALITY").setting.defaultValue, "2");
}
TEST(PackLoaderTest, ReadsShadowResolutionFromShaderSource) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "const int shadowMapResolution = 2048;\nvoid main(){}"}},
                  pack,
                  options,
                  error));
 EXPECT_EQ(pack.shadowMapResolution, 2048);
 EXPECT_FALSE(options.contains("shadowMapResolution"));
}
TEST(PackLoaderTest, ParsesShadowPackConstants) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh",
                    "const bool shadowEntities = false;\n"
                    "const bool shadowHardwareFiltering = true;\n"
                    "const float shadowDistance = 64.0;\n"
                    "const float shadowDistanceRenderMul = 1.0;\n"
                    "const float shadowMapFov = 45.0;\n"
                    "const float shadowNearPlane = 0.1;\n"
                    "const float shadowFarPlane = 128.0;\n"
                    "const float shadowIntervalSize = 4.0;\n"
                    "const float voxelDistance = 32.0;\n"
                    "const float entityShadowDistanceMul = 0.5;\n"
                    "void main(){}"},
                   {"shaders/shadowcomp.vsh", "void main(){}"},
                   {"shaders/shadowcomp.fsh", "/* RENDERTARGETS: 0,2 */\nvoid main(){}"}},
                  pack,
                  options,
                  error))
     << error;
 // PackShadowDirectives equivalents (Loader scanPackConstants).
 // ShaderProperties.java:194 - shadowEntities is a shaders.properties key, never a GLSL const
 EXPECT_TRUE(pack.shadowEntities);
 EXPECT_TRUE(pack.shadowHardwareFiltering[0]);
 EXPECT_TRUE(pack.shadowHardwareFiltering[1]);
 EXPECT_FLOAT_EQ(pack.shadowDistance, 64.0f);
 EXPECT_FLOAT_EQ(pack.shadowDistanceRenderMul, 1.0f);
 EXPECT_FLOAT_EQ(pack.shadowMapFov, 45.0f);
 EXPECT_FLOAT_EQ(pack.shadowNearPlane, 0.1f);
 EXPECT_FLOAT_EQ(pack.shadowFarPlane, 128.0f);
 EXPECT_FLOAT_EQ(pack.shadowIntervalSize, 4.0f);
 EXPECT_FLOAT_EQ(pack.voxelDistance, 32.0f);
 EXPECT_FLOAT_EQ(pack.entityShadowDistanceMul, 0.5f);
 // Shadow composite outputs are remapped from colortex to shadowcolor buffers and
 // size the shadow color buffer count (Loader addPostPrograms).
 EXPECT_EQ(pack.shadowColorBuffers, 3);
 EXPECT_EQ(pack.gbufferColorBuffers, 1);
 const auto shadowcomp =
     std::find_if(pack.passes.begin(), pack.passes.end(),
                  [](const PackPass& pass) { return pass.type == "shadowcomp"; });
 ASSERT_NE(shadowcomp, pack.passes.end());
 ASSERT_EQ(shadowcomp->outputs.size(), 2u);
 EXPECT_EQ(shadowcomp->outputs[0], "shadowcolor0");
 EXPECT_EQ(shadowcomp->outputs[1], "shadowcolor2");
}
TEST(PackLoaderTest, LoadsIrisProgramsAndFeatureFlags) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
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
                    "iris.features.required=ENTITY_TRANSLUCENT BLOCK_EMISSION_ATTRIBUTE\n"
                    "iris.features.optional=BLOCK_EMISSION_ATTRIBUTE FADE_VARIABLE\n"}},
                  pack,
                  options,
                  error));
 EXPECT_TRUE(pack.programs.contains("shadow_solid"));
 EXPECT_TRUE(pack.requiredFeatures.contains("ENTITY_TRANSLUCENT"));
 EXPECT_TRUE(pack.optionalFeatures.contains("FADE_VARIABLE"));
 EXPECT_EQ(pack.shadowColorBuffers, 3);
 EXPECT_TRUE(std::any_of(pack.passes.begin(), pack.passes.end(),
                         [](const PackPass& pass) { return pass.type == "shadowcomp"; }));
 EXPECT_TRUE(std::any_of(pack.passes.begin(), pack.passes.end(),
                         [](const PackPass& pass) { return pass.type == "prepare"; }));
}
TEST(PackLoaderTest, UsesDocumentedProgramAndComputeNames) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
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
 EXPECT_TRUE(pack.programs.contains("gbuffers_entities_glowing"));
}
TEST(PackLoaderTest, ReadsMetadataFromResolvedActiveComputeBranch) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh",
                    "#if 0\nconst int shadowMapResolution = 8192;\n#else\n"
                    "const int shadowMapResolution = 1024;\n#endif\nvoid main(){}"},
                   {"shaders/gbuffers_basic.fsh",
                    "#if 0\nconst int colortex0Format = RGBA32F;\n#else\n"
                    "const int colortex0Format = RGBA16F;\n#endif\nvoid main(){}"},
                   {"shaders/composite.csh", "#define SELECT 1\n#include \"lib/compute.glsl\"\n"},
                   {"shaders/lib/compute.glsl",
                    "#if SELECT == 1\n"
                    "layout(local_size_x = 8, local_size_y = 4, local_size_z = 2) in;\n"
                    "const ivec3 workGroups = ivec3(2, 3, 4);\n"
                    "#else\n"
                    "layout(local_size_x = 64, local_size_y = 64, local_size_z = 64) in;\n"
                    "const ivec3 workGroups = ivec3(999, 999, 999);\n"
                    "#endif\n"}},
                  pack,
                  options,
                  error))
     << error;
 const auto pass = std::find_if(pack.passes.begin(), pack.passes.end(),
                                [](const PackPass& value) { return value.program == "composite#compute"; });
 ASSERT_NE(pass, pack.passes.end());
 // No local-size assertion: Java takes the local size from the linked program
 // (GL_COMPUTE_WORK_GROUP_SIZE), not from the source text, so the loader has no
 // business parsing `layout(local_size_...)` at all.
 EXPECT_EQ(pass->groups[0], 2);
 EXPECT_EQ(pass->groups[1], 3);
 EXPECT_EQ(pass->groups[2], 4);
 EXPECT_FALSE(pass->relativeGroups);
 EXPECT_EQ(pack.shadowMapResolution, 1024);
 EXPECT_EQ(pack.targets.at("colortex0").format, "RGBA16F");
}
TEST(PackLoaderTest, AppliesCurrentSettingsToDimensionComputeMetadata) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/dimension.properties", "dimension.world0=*\n"},
                   {"shaders/lib/common.glsl", "#define SIZE 1 //[1 2]\n"},
                   {"shaders/program/compute.glsl",
                    "#include \"/lib/common.glsl\"\n"
                    "#if SIZE == 1\nconst ivec3 workGroups = ivec3(64, 64, 64);\n"
                    "#else\nconst ivec3 workGroups = ivec3(8, 8, 8);\n#endif\n"},
                   {"shaders/world0/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/world0/gbuffers_basic.fsh", "void main(){}"},
                   {"shaders/world0/composite.csh", "#include \"/program/compute.glsl\"\n"}},
                  pack,
                  options,
                  error,
                  {{"SIZE", "2"}}))
     << error;
 const auto definition = pack.dimensionDefinitions.find("*");
 ASSERT_NE(definition, pack.dimensionDefinitions.end());
 const auto pass = std::find_if(definition->second->passes.begin(), definition->second->passes.end(),
                                [](const PackPass& value) { return value.program == "composite#compute"; });
 ASSERT_NE(pass, definition->second->passes.end());
 // SIZE=2 selects the #else branch, so the option value reached the compute
 // metadata preprocessor before the workGroups directive was read.
 EXPECT_FALSE(pass->relativeGroups);
 EXPECT_EQ(pass->groups[0], 8);
 EXPECT_EQ(pass->groups[1], 8);
 EXPECT_EQ(pass->groups[2], 8);
}
TEST(PackLoaderTest, CurrentParticleOrderingOverridesLegacyRegardlessOfOrder) {
 for(const std::string properties : {
         "particles.ordering=mixed\nparticles.before.deferred=false\n",
         "particles.before.deferred=false\nparticles.ordering=mixed\n"}) {
  PackDefinition pack;
  std::unordered_map<std::string, PackSourceOption> options;
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
TEST(PackLoaderTest, MatchesDocumentedStageNameGrammar) {
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
TEST(PackLoaderTest, ReadsFormatsClearFlipAndCustomTexture) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
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
TEST(PackLoaderTest, ReadsDocumentedNoiseTextureConfiguration) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "const int noiseTextureResolution = 512;\nvoid main(){}"},
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
TEST(PackLoaderTest, LoadsLegacyDimensionFolders) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
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
TEST(PackLoaderTest, LoadsDimensionOnlyPackWithWildcard) {
 // RenderPearl-shaped: no root programs; dimension.world_default=*
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
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
TEST(PackLoaderTest, LoadsDimensionOnlyPackWithRootInclude) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
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
TEST(PackLoaderTest, IgnoresMetadataFromUnsupportedIntegrationSources) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "void main(){gl_FragData[0]=vec4(1.0);}"},
                   {"shaders/dh_terrain.fsh",
                    "const int noiseTextureResolution=8192; const int colortex15Format=R32UI; "
                    "/* RENDERTARGETS: 15 */ void main(){}"},
                   {"shaders/clrwl_gbuffers.fsh",
                    "const int shadowMapResolution=16384; /* RENDERTARGETS: 14 */ void main(){}"},
                   {"shaders/voxy_opaque.glsl",
                    "const int colortex13Format=RGBA32I; /* RENDERTARGETS: 13 */"}},
                  pack,
                  options,
                  error))
     << error;
 EXPECT_EQ(pack.noiseTextureResolution, 256);
 EXPECT_EQ(pack.shadowMapResolution, 0);
 EXPECT_EQ(pack.gbufferColorBuffers, 1);
 EXPECT_FALSE(pack.programs.contains("dh_terrain"));
 EXPECT_FALSE(pack.programs.contains("clrwl_gbuffers"));
 EXPECT_FALSE(pack.programs.contains("voxy_opaque"));
 EXPECT_FALSE(pack.targets.contains("colortex13"));
 EXPECT_FALSE(pack.targets.contains("colortex14"));
 EXPECT_FALSE(pack.targets.contains("colortex15"));
}
TEST(PackLoaderTest, PropertiesUseIrisEnvironmentAndNormalizeLegacyFeatureSpelling) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "void main(){}"},
                   {"shaders/shaders.properties",
                    "#if defined(IS_IRIS) && defined(IRIS_FEATURE_ENTITY_TRANSLUCENT)\n"
                    "separateEntityDraws=true\n"
                    "#endif\n"
                    "iris.features.optional=TESSELATION_SHADERS NOT_A_REAL_FEATURE\n"}},
                  pack,
                  options,
                  error))
     << error;
 EXPECT_TRUE(pack.separateEntityDraws);
 EXPECT_TRUE(pack.optionalFeatures.contains("TESSELLATION_SHADERS"));
 EXPECT_FALSE(pack.optionalFeatures.contains("TESSELATION_SHADERS"));
 EXPECT_TRUE(pack.optionalFeatures.contains("NOT_A_REAL_FEATURE"));
}
TEST(PackLoaderTest, DimensionPropertiesMultiIdMapsEachKey) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
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
TEST(PackLoaderTest, DimensionFolderPackKeepsRootFormats) {
 // rethinking-voxels-shaped: colortexNFormat only in shaders/lib/, programs only under world0/.
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/lib/pipelineSettings.glsl",
                    "const int colortex0Format = R11F_G11F_B10F;\n"
                    "const int colortex9Format = R32UI;\n"},
                   {"shaders/world0/composite.vsh", "void main(){}"},
                   {"shaders/world0/composite.fsh", "void main(){}"}},
                  pack,
                  options,
                  error))
     << error;
 ASSERT_TRUE(pack.dimensionDefinitions.contains("minecraft:overworld"));
 // dimension scan never sees lib/pipelineSettings.glsl; colortex0 here is the implicit default.
 ASSERT_TRUE(pack.dimensionDefinitions.at("minecraft:overworld")->targets.contains("colortex0"));
 EXPECT_EQ(pack.dimensionDefinitions.at("minecraft:overworld")->targets.at("colortex0").format, "RGBA8");
 ASSERT_TRUE(pack.targets.contains("colortex0"));
 ASSERT_TRUE(pack.targets.contains("colortex9"));
 EXPECT_EQ(pack.targets.at("colortex0").format, "R11F_G11F_B10F");
 EXPECT_EQ(pack.targets.at("colortex9").format, "R32UI");
}
TEST(PackLoaderTest, ResolvesLineToDedicatedProgramThenBasic) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
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
TEST(PackLoaderTest, LineFallsBackToBasicWhenMissing) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "void main(){}"}},
                  pack,
                  options,
                  error));
 EXPECT_FALSE(pack.programs.contains("gbuffers_line"));
 EXPECT_EQ(resolved(pack, "gbuffers_line"), "gbuffers_basic");
}
TEST(PackLoaderTest, MatchingPairFallbackDoesNotMixStages) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_textured.vsh", "void main(){}"},
                   {"shaders/gbuffers_textured.fsh", "void main(){}"},
                   {"shaders/gbuffers_terrain.fsh", "void main(){}"}},
                  pack,
                  options,
                  error));
 EXPECT_TRUE(pack.programs.at("gbuffers_terrain").vertex.empty());
 EXPECT_EQ(pack.programs.at("gbuffers_terrain").fragment, "shaders/gbuffers_terrain.fsh");
 EXPECT_EQ(resolved(pack, "gbuffers_terrain"), "gbuffers_terrain");
}
TEST(PackLoaderTest, ReadsIrisGeometrySkipAndCullingDirectives) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
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
 EXPECT_EQ(pack.shadowCulling, ShadowCullState::Distance);
 EXPECT_FALSE(pack.renderClouds);
 EXPECT_FLOAT_EQ(pack.shadowDistance, 128.0f);
 EXPECT_FLOAT_EQ(pack.shadowDistanceRenderMul, 1.0f);
 EXPECT_EQ(pack.programEnabled.at("composite"), "false");
 EXPECT_EQ(pack.programEnabled.at("deferred"), "BLOOM");
}
TEST(PackLoaderTest, ReadsReversedShadowCulling) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "void main(){}"},
                   {"shaders/shaders.properties", "shadow.culling=reversed\n"}},
                  pack,
                  options,
                  error));
 EXPECT_EQ(pack.shadowCulling, ShadowCullState::SafeZone);
}
TEST(PackLoaderTest, ReadsIrisFourFactorBlendAndCloudsOff) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
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
TEST(PackLoaderTest, PreprocessesMcVersionInBlockProperties) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
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
TEST(PackLoaderTest, ReadsPerProgramMipmapEnabled) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
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
                                [](const PackPass& p) { return p.name == "composite1"; });
 ASSERT_NE(pass, pack.passes.end());
 ASSERT_EQ(pass->mipmapBuffers.size(), 1u);
 EXPECT_EQ(pass->mipmapBuffers.front(), "colortex6");
}
TEST(PackLoaderTest, ReadsShadowHardwareFilteringConstants) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
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
TEST(PackLoaderTest, ReadsExtendedPackConstants) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh",
                    "const float eyeBrightnessHalflife = 5.0;\n"
                    "const float shadowIntervalSize = 0.5;\n"
                    "const float shadowNearPlane = 0.1;\n"
                    "const float shadowFarPlane = 512.0;\n"
                    "const float ambientOcclusionLevel = 0.5;\n"
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
 EXPECT_TRUE(pack.shadowcolorMipmap[0]);
 EXPECT_FALSE(pack.shadowcolorMipmap[1]);
 EXPECT_EQ(pack.targets.at("shadowcolor0").format, "RGBA16F");
 EXPECT_FALSE(pack.targets.at("shadowcolor0").clear);
 EXPECT_FLOAT_EQ(pack.targets.at("shadowcolor0").clearColor[0], 1.0f);
 EXPECT_EQ(pack.targets.at("colortex20").format, "R32F");
}
TEST(PackLoaderTest, ReadsSizeBufferScaleAndAlphaTest) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
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
TEST(PackLoaderTest, ReadsRenderTargetsFromIncludedFragment) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
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
TEST(PackLoaderTest, ParseRenderTargetsAfterInactiveBranch) {
 const std::string source = R"(#if 0
/* RENDERTARGETS: 9 */
#endif
/* RENDERTARGETS: 1,2 */
void main() {}
)";
 const std::string normalized = normalizePackSource(PackDefinition{}, source);
 const std::vector<int> indices = parseRenderTargetIndices(normalized);
 ASSERT_EQ(indices.size(), 2u);
 EXPECT_EQ(indices[0], 1);
 EXPECT_EQ(indices[1], 2);
}
TEST(PackLoaderTest, ParseDrawBuffersDirective) {
 const std::vector<int> indices = parseRenderTargetIndices("/* DRAWBUFFERS: 0178 */\nvoid main(){}");
 ASSERT_EQ(indices.size(), 4u);
 EXPECT_EQ(indices[0], 0);
 EXPECT_EQ(indices[1], 1);
 EXPECT_EQ(indices[2], 7);
 EXPECT_EQ(indices[3], 8);
}
TEST(PackLoaderTest, DefaultRenderTargetIndices) {
 // A program without a RENDERTARGETS/DRAWBUFFERS directive draws to a single buffer
 // (colortex0), matching Java Iris' /* DRAWBUFFERS:0 */ default.
 // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shaderpack/properties/ProgramDirectives.java#L73-L76
 const std::vector<int> indices = defaultRenderTargetIndices();
 ASSERT_EQ(indices.size(), 1u);
 EXPECT_EQ(indices[0], 0);
}
TEST(PackLoaderTest, ParseRenderTargetsMissingDirectiveDefaultsToSingleBuffer) {
 const std::string source = "void main() {}\n";
 const std::vector<int> parsed = parseRenderTargetIndices(source);
 EXPECT_TRUE(parsed.empty());
 const std::vector<std::string> outputs = renderTargetOutputNames(source);
 ASSERT_EQ(outputs.size(), 1u);
 EXPECT_EQ(outputs[0], "colortex0");
}
TEST(PackLoaderTest, RenderTargetOutputNamesWithExplicitDirective) {
 EXPECT_EQ(renderTargetOutputNames("/* RENDERTARGETS: 0,2 */\nvoid main(){}"),
           (std::vector<std::string>{"colortex0", "colortex2"}));
 EXPECT_EQ(renderTargetOutputNames("/* DRAWBUFFERS: 02 */\nvoid main(){}"),
           (std::vector<std::string>{"colortex0", "colortex2"}));
}
TEST(PackLoaderTest, NormalizePackSourcePreservesComments) {
 const std::string source = "/* RENDERTARGETS: 0,1 */\nvoid main() {}\n";
 const std::string normalized = normalizePackSource(PackDefinition{}, source);
 EXPECT_NE(normalized.find("/* RENDERTARGETS: 0,1 */"), std::string::npos);
 const std::vector<int> indices = parseRenderTargetIndices(normalized);
 ASSERT_EQ(indices.size(), 2u);
 EXPECT_EQ(indices[0], 0);
 EXPECT_EQ(indices[1], 1);
}
TEST(PackLoaderTest, HoistsOnlyExtensionsFromActiveBranches) {
 const std::string source =
     "#define NVIDIA_PATH\n"
     "#ifdef NVIDIA_PATH\n"
     "#extension GL_NV_gpu_shader5 : require\n"
     "#endif\n"
     "#ifdef AMD_PATH\n"
     "#extension GL_AMD_gpu_shader_half_float : require\n"
     "#endif\n"
     "#if 0\n"
     "#define INTEL_PATH\n"
     "#endif\n"
     "#ifdef INTEL_PATH\n"
     "#extension GL_INTEL_shader_integer_functions2 : require\n"
     "#endif\n"
     "void main() {}\n";
 const std::string normalized = normalizePackSource(PackDefinition{}, source);
 EXPECT_TRUE(normalized.starts_with("#extension GL_NV_gpu_shader5 : require\n"));
 EXPECT_EQ(normalized.find("GL_AMD_gpu_shader_half_float"), std::string::npos);
 EXPECT_EQ(normalized.find("GL_INTEL_shader_integer_functions2"), std::string::npos);
 EXPECT_NE(normalized.find("void main() {}"), std::string::npos);
}
TEST(PackLoaderTest, HoistsExtensionsFromSelectedElifAndNestedBranches) {
 const std::string source =
     "#define MODE 2\n"
     "#if MODE == 1\n"
     "#extension GL_TEST_mode_one : require\n"
     "#elif MODE == 2\n"
     "#if defined(MODE)\n"
     "#extension GL_TEST_mode_two : enable\n"
     "#endif\n"
     "#else\n"
     "#extension GL_TEST_mode_other : require\n"
     "#endif\n";
 const std::string normalized = normalizePackSource(PackDefinition{}, source);
 EXPECT_TRUE(normalized.starts_with("#extension GL_TEST_mode_two : enable\n"));
 EXPECT_EQ(normalized.find("GL_TEST_mode_one"), std::string::npos);
 EXPECT_EQ(normalized.find("GL_TEST_mode_other"), std::string::npos);
}
TEST(PackLoaderTest, ResolvesFloatingPointPackOptionConditionsBeforeDriverCompilation) {
 const std::string source =
     "#define AUTO_EXP 1.000000\n"
     "#if AUTO_EXP\n"
     "float exposure = 1.0;\n"
     "#else\n"
     "float exposure = 0.0;\n"
     "#endif\n";
 const std::string normalized = normalizePackSource(PackDefinition{}, source);
 EXPECT_EQ(normalized.find("#if AUTO_EXP"), std::string::npos);
 EXPECT_NE(normalized.find("float exposure = 1.0;"), std::string::npos);
 EXPECT_EQ(normalized.find("float exposure = 0.0;"), std::string::npos);
}
TEST(PackLoaderTest, IgnoresFormatDirectivesInsideBlockComments) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
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
TEST(PackLoaderTest, ReadsScreenSlidersProfilesAndPackToggles) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
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
// Some packs have both `#define immut const` and a bodyless `#define immut`.
// Treating that name as a boolean option and rewriting it turns every
// `immut vec3 x = ...;` declaration into garbage, so the name must be dropped.
TEST(PackLoaderTest, DropsDefinesUsedWithConflictingShapes) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
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
     PackLoader::rewriteOptions("#define immut const\n#define immut\n", options, {{"immut", "1"}});
 EXPECT_EQ(rewritten, "#define immut const\n#define immut\n");
}
TEST(PackLoaderTest, BooleanOptionRewriteKeepsMacroBodyless) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "#define BLOOM // glow\nvoid main(){}"}},
                  pack,
                  options,
                  error))
     << error;
 EXPECT_EQ(PackLoader::rewriteOptions("#define BLOOM // glow\n", options, {{"BLOOM", "1"}}),
           "#define BLOOM // glow\n");
 EXPECT_EQ(PackLoader::rewriteOptions("#define BLOOM // glow\n", options, {{"BLOOM", "0"}}),
           "//#define BLOOM // glow\n");
}
TEST(PackSourcePreparation, CompatibilityGbuffersGetsEndOfMainDiscard) {
 net::minecraft::test::installTestGlslSnippets();
 PackDefinition pack;
 const std::string src = R"(#version 330 compatibility
void main() {
	gl_FragData[0] = vec4(1.0);
}
)";
 const std::string out = prepareSource(
     "gbuffers_terrain", ShaderStage::Fragment, pack, src);
 EXPECT_NE(out.find("uniform float alphaTestRef"), std::string::npos);
 EXPECT_NE(out.find("iris_FragData0.a > alphaTestRef"), std::string::npos);
 EXPECT_NE(out.find("layout(location = 0) out vec4 iris_FragData0;"), std::string::npos);
 const std::size_t write = out.find("iris_FragData0 = vec4");
 const std::size_t discard = out.find("discard");
 ASSERT_NE(write, std::string::npos);
 ASSERT_NE(discard, std::string::npos);
 EXPECT_LT(write, discard);
}
TEST(PackSourcePreparation, ModernOutputsAndPostPassesDoNotGainAlphaTest) {
 net::minecraft::test::installTestGlslSnippets();
 PackDefinition pack;
 const std::string core = R"(#version 430 core
layout(location=0) out vec4 outColor;
void main() { outColor = vec4(1.0); }
)";
 EXPECT_EQ(prepareSource(
               "gbuffers_terrain", ShaderStage::Fragment, pack, core)
               .find("discard"),
           std::string::npos);
 const std::string compat = R"(#version 330 compatibility
void main() { gl_FragData[0] = vec4(1.0); }
)";
 EXPECT_EQ(prepareSource(
               "composite", ShaderStage::Fragment, pack, compat)
               .find("discard"),
           std::string::npos);
 EXPECT_NE(prepareSource(
               "gbuffers_water", ShaderStage::Fragment, pack, compat)
               .find("discard"),
           std::string::npos);
 EXPECT_NE(prepareSource(
               "gbuffers_entities_translucent", ShaderStage::Fragment, pack, compat)
               .find("discard"),
           std::string::npos);
 const std::string modernWater = R"(#version 430 core
layout(location=0) out f16vec4 colortex1;
void main() { colortex1 = f16vec4(1.0); }
)";
 const std::string preparedWater = prepareSource(
     "gbuffers_water", ShaderStage::Fragment, pack, modernWater);
 EXPECT_EQ(preparedWater.find("uniform float alphaTestRef"), std::string::npos);
 EXPECT_EQ(preparedWater.find("discard"), std::string::npos);
}
TEST(PackSourcePreparation, ImmutableProjectionHelperResultsAreRuntimeValues) {
 net::minecraft::test::installTestGlslSnippets();
 PackDefinition pack;
 const std::string source = R"(#version 430 core
#define immut const
uniform mat4 projectionMatrix;
in vec3 vaPosition;
vec4 proj_mmul(mat4 projection, vec3 view) { return projection * vec4(view, 1.0); }
vec3 proj(mat4 projection, vec3 view) {
 immut vec4 clip = proj_mmul(projection, view);
 return clip.xyz / clip.w;
}
void main() {
 vec3 view = vaPosition;
 immut vec4 clip = proj_mmul(mat4(projectionMatrix), view);
 gl_Position = clip;
}
)";
 const std::string prepared = prepareSource("gbuffers_water", ShaderStage::Vertex, pack, source);
 EXPECT_EQ(prepared.find("immut vec4 clip = proj_mmul"), std::string::npos);
 EXPECT_NE(prepared.find("vec4 clip = proj_mmul"), std::string::npos);
}
TEST(PackSourcePreparation, ChunkFadeMatchesTheAdvertisedFeatureAbi) {
 net::minecraft::test::installTestGlslSnippets();
 PackDefinition pack;
 pack.optionalFeatures.insert("FADE_VARIABLE");
 const std::string source =
     "#version 430 compatibility\nvoid main() { float fade = mc_chunkFade; }\n";
 const auto prepare = [&](const std::string& program) {
  return prepareSource(
      program, ShaderStage::Vertex, pack, source);
 };
 EXPECT_NE(prepare("gbuffers_terrain").find("in float mc_chunkFade;"), std::string::npos);
 // Water and ice are chunk geometry, so gbuffers_water takes the terrain attribute too.
 EXPECT_NE(prepare("gbuffers_water").find("in float mc_chunkFade;"), std::string::npos);
 // ColorWheel material programs reuse the pack's terrain/water vertex bodies, so they
 // take the same attribute form (the engine binds location 12 to a constant 1.0).
 EXPECT_NE(prepare("clrwl_gbuffers").find("in float mc_chunkFade;"), std::string::npos);
 EXPECT_NE(prepare("clrwl_gbuffers_translucent").find("in float mc_chunkFade;"), std::string::npos);
 EXPECT_NE(prepare("clrwl_gbuffers_damagedblock").find("in float mc_chunkFade;"), std::string::npos);
 EXPECT_NE(prepare("gbuffers_entities").find("const float mc_chunkFade = -1.0;"),
           std::string::npos);
 // Shadow programs get the -1.0 const, not nothing. This assertion used to require the
 // opposite; that encoded the port's own behaviour rather than Iris'. SodiumTransformer
 // declares `const float mc_chunkFade = -1.0;` on its `parameters.shadow` branch, and
 // leaving the symbol undeclared is a compile failure for any pack that shares a vertex
 // body between gbuffers_water and shadow_water.
 // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/pipeline/transform/transformer/SodiumTransformer.java
 EXPECT_NE(prepare("shadow").find("const float mc_chunkFade = -1.0;"), std::string::npos);
 EXPECT_NE(prepare("shadow_water").find("const float mc_chunkFade = -1.0;"), std::string::npos);
 EXPECT_NE(prepare("clrwl_shadow").find("const float mc_chunkFade = -1.0;"), std::string::npos);
}
TEST(PackLoaderTest, InfersColortexFormatsFromImageAndUsamplerLayouts) {
 // Some packs leave const colortexNFormat commented; formats come from layouts.
 // https://github.com/Luracasmus/renderpearl/blob/main/DEV.md
 // https://www.khronos.org/opengl/wiki/Image_Load_Store
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
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
                    "layout(location = 1) out uvec4 colortex2;\n"
                    "layout(location = 0) out f16vec4 colortex1;\n"
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
TEST(PackLoaderTest, ParsesShaderStorageBuffersAllForms) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "void main(){}"},
                   {"shaders/shaders.properties",
                    "iris.features.optional=SSBO\n"
                    "bufferObject.0=6\n"
                    "bufferObject.1=784 buffers/llq.bin\n"
                    "bufferObject.3=16 true 0.5 0.5\n"
                    "bufferObject.12=4\n"
                    "bufferObject.13=4\n"
                    "bufferObject.4=0\n"
                    "bufferObject.5=8 false 1.0 2.0\n"}},
                  pack,
                  options,
                  error))
     << error;
 ASSERT_EQ(pack.bufferObjects.size(), 5u);
 EXPECT_EQ(pack.bufferObjects[0].index, 0);
 EXPECT_EQ(pack.bufferObjects[0].byteSize, 6u);
 EXPECT_FALSE(pack.bufferObjects[0].relative);
 EXPECT_TRUE(pack.bufferObjects[0].initPath.empty());
 EXPECT_EQ(pack.bufferObjects[1].index, 1);
 EXPECT_EQ(pack.bufferObjects[1].byteSize, 784u);
 EXPECT_EQ(pack.bufferObjects[1].initPath, "buffers/llq.bin");
 EXPECT_FALSE(pack.bufferObjects[1].relative);
 EXPECT_EQ(pack.bufferObjects[2].index, 3);
 EXPECT_TRUE(pack.bufferObjects[2].relative);
 EXPECT_EQ(pack.bufferObjects[2].scaleX, 0.5f);
 EXPECT_EQ(pack.bufferObjects[2].scaleY, 0.5f);
 EXPECT_EQ(pack.bufferObjects[3].index, 12);
 EXPECT_EQ(pack.bufferObjects[3].byteSize, 4u);
 EXPECT_FALSE(pack.bufferObjects[3].relative);
 EXPECT_EQ(pack.bufferObjects[4].index, 5);
 EXPECT_FALSE(pack.bufferObjects[4].relative);
 EXPECT_EQ(pack.bufferObjects[4].scaleX, 1.0f);
 EXPECT_EQ(pack.bufferObjects[4].scaleY, 2.0f);
}
TEST(PackLoaderTest, RepeatedBufferDirectiveReplacesPreviousEntry) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "void main(){}"},
                   {"shaders/shaders.properties",
                    "iris.features.optional=SSBO\n"
                    "bufferObject.1=784\nbufferObject.1=1552\n"}},
                  pack,
                  options,
                  error))
     << error;
 ASSERT_EQ(pack.bufferObjects.size(), 1u);
 EXPECT_EQ(pack.bufferObjects[0].index, 1);
 EXPECT_EQ(pack.bufferObjects[0].byteSize, 1552u);
}
TEST(PackLoaderTest, ParsesIndirectDispatchPointers) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "void main(){}"},
                   {"shaders/shaders.properties",
                    "indirect.composite=3 0\n"
                    "indirect.setup=12 16\n"
                    "indirect.bad=abc 0\n"
                    "indirect.two=3 13\n"
                    "indirect.three=13 0\n"}},
                  pack,
                  options,
                  error))
     << error;
 const auto composite = pack.indirectDispatches.find("composite");
 ASSERT_NE(composite, pack.indirectDispatches.end());
 EXPECT_EQ(composite->second.buffer, 3);
 EXPECT_EQ(composite->second.offset, 0u);
 const auto setup = pack.indirectDispatches.find("setup");
 ASSERT_NE(setup, pack.indirectDispatches.end());
 EXPECT_EQ(setup->second.buffer, 12);
 EXPECT_EQ(setup->second.offset, 16u);
 EXPECT_EQ(pack.indirectDispatches.size(), 2u);
}
TEST(PackLoaderTest, ReadsWeatherParticlesAndNewToggleDirectives) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "void main(){}"},
                   {"shaders/shaders.properties",
                    "weather=true false\n"
                    "dhClouds=off\n"
                    "dhShadow.enabled=false\n"
                    "prepareBeforeShadow=true\n"
                    "breaksAnisotropy=true\n"
                    "fallbackTex=2\n"}},
                  pack,
                  options,
                  error))
     << error;
 EXPECT_TRUE(pack.renderWeather);
 EXPECT_FALSE(pack.renderWeatherParticles);
 EXPECT_EQ(pack.dhCloudsMode, "off");
 EXPECT_FALSE(pack.dhShadowEnabled);
 EXPECT_TRUE(pack.prepareBeforeShadow);
 EXPECT_TRUE(pack.breaksAnisotropy);
 EXPECT_EQ(pack.fallbackTex, 2);
}
TEST(PackLoaderTest, WeatherSecondTokenControlsWeatherParticles) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "void main(){}"},
                   {"shaders/shaders.properties", "weather=false true\n"}},
                  pack,
                  options,
                  error))
     << error;
 EXPECT_FALSE(pack.renderWeather);
 EXPECT_TRUE(pack.renderWeatherParticles);
}
TEST(PackLoaderTest, ReadsScreenColumnCountsSeparateFromPages) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "void main(){}"},
                   {"shaders/shaders.properties",
                    "screen.columns=3\n"
                    "screen.LIGHTING.columns=2\n"
                    "screen.LIGHTING=BAR BAZ\n"}},
                  pack,
                  options,
                  error))
     << error;
 EXPECT_EQ(pack.screenColumns, 3);
 EXPECT_EQ(pack.screenPageColumns.at("LIGHTING"), 2);
 EXPECT_EQ(pack.screenPages.at("LIGHTING").size(), 2u);
 EXPECT_FALSE(pack.screenPages.contains("columns"));
}
TEST(PackLoaderTest, ReadsShadowCullingSafeZone) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "void main(){}"},
                   {"shaders/shaders.properties", "shadow.culling=safe_zone\n"}},
                  pack,
                  options,
                  error))
     << error;
 EXPECT_EQ(pack.shadowCulling, ShadowCullState::SafeZone);
}
TEST(PackLoaderTest, SizeBufferAllowsMixedAbsoluteRelativeAxes) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "void main(){}"},
                   {"shaders/shaders.properties", "size.buffer.colortex5=0.5 256\n"}},
                  pack,
                  options,
                  error))
     << error;
 EXPECT_FLOAT_EQ(pack.targets.at("colortex5").scaleX, 0.5f);
 EXPECT_FLOAT_EQ(pack.targets.at("colortex5").scaleY, 1.0f);
 EXPECT_EQ(pack.targets.at("colortex5").absoluteHeight, 256);
}
TEST(PackLoaderTest, ScaleAcceptsValuesAboveOne) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "void main(){}"},
                   {"shaders/shaders.properties", "scale.composite=1.5 0.1 0.2\n"}},
                  pack,
                  options,
                  error))
     << error;
 EXPECT_FLOAT_EQ(pack.programScales.at("composite").scale, 1.5f);
}
TEST(PackLoaderTest, RejectsSsbosWithoutFeatureFlag) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 EXPECT_FALSE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                    {"shaders/gbuffers_basic.fsh", "void main(){}"},
                    {"shaders/shaders.properties", "bufferObject.0=1024\n"}},
                   pack,
                   options,
                   error));
 EXPECT_NE(error.find("SSBO"), std::string::npos);
}
TEST(PackLoaderTest, RejectsImagesWithoutCustomImagesFlag) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 EXPECT_FALSE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                    {"shaders/gbuffers_basic.fsh", "void main(){}"},
                    {"shaders/shaders.properties",
                     "image.0=colortex0 RGBA8 RGBA8 UNSIGNED_BYTE true false 512 512\n"}},
                   pack,
                   options,
                   error));
 EXPECT_NE(error.find("CUSTOM_IMAGES"), std::string::npos);
}
TEST(PackLoaderTest, RejectsUnsupportedRequiredFeatures) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 EXPECT_FALSE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                    {"shaders/gbuffers_basic.fsh", "void main(){}"},
                    {"shaders/shaders.properties", "iris.features.required=NOT_A_FEATURE\n"}},
                   pack,
                   options,
                   error));
 EXPECT_NE(error.find("NOT_A_FEATURE"), std::string::npos);
}
TEST(PackLoaderTest, ParticlesBeforeDeferredFalseLeavesDefault) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "void main(){}"},
                   {"shaders/shaders.properties", "particles.before.deferred=false\n"}},
                  pack,
                  options,
                  error))
     << error;
 // Java: only a true value with no other ordering directive moves particles before deferred.
 EXPECT_TRUE(pack.particleOrdering.empty());
}
TEST(PackLoaderTest, AlphaTestFalseDisables) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "void main(){}"},
                   {"shaders/shaders.properties", "alphaTest.gbuffers_basic=false\n"}},
                  pack,
                  options,
                  error))
     << error;
 ASSERT_EQ(pack.alphaTests.size(), 1u);
 EXPECT_FALSE(pack.alphaTests.front().enabled);
 EXPECT_EQ(pack.alphaTests.front().func, "ALWAYS");
}
TEST(PackLoaderTest, BlendResolvesLegacyBufferNames) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "void main(){}"},
                   {"shaders/shaders.properties",
                    "blend.gbuffers_terrain.gdepth=ONE ZERO ONE ZERO\n"
                    "blend.gbuffers_terrain.gaux3=off\n"}},
                  pack,
                  options,
                  error))
     << error;
 const BufferBlend* gdepth = nullptr;
 const BufferBlend* gaux3 = nullptr;
 for(const BufferBlend& blend : pack.bufferBlends) {
  if(blend.program == "gbuffers_terrain" && blend.buffer == 1) gdepth = &blend;
  if(blend.program == "gbuffers_terrain" && blend.buffer == 6) gaux3 = &blend;
 }
 ASSERT_NE(gdepth, nullptr);
 ASSERT_NE(gaux3, nullptr);
 EXPECT_TRUE(gdepth->enabled);
 EXPECT_EQ(gdepth->source, "ONE");
 EXPECT_FALSE(gaux3->enabled);
}
TEST(PackLoaderTest, LegacyTargetNamesAliasColortexSlots) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shaderpack/properties/PackRenderTargetDirectives.java#L95-L101
 // Legacy buffer names are scanned like the colortexN spelling and write the same
 // slot: gcolor=0, gdepth=1, gnormal=2, composite=3, gaux1..4=4..7.
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh",
                    "const int gcolorFormat = RGBA16;\n"
                    "const bool gdepthClear = false;\n"
                    "const int compositeFormat = RGBA32F;\n"
                    "const vec4 gaux4ClearColor = vec4(0.25, 0.5, 0.75, 1.0);\n"
                    "void main(){}"}},
                  pack,
                  options,
                  error))
     << error;
 EXPECT_EQ(pack.targets.at("colortex0").format, "RGBA16");
 EXPECT_FALSE(pack.targets.at("colortex1").clear);
 EXPECT_EQ(pack.targets.at("colortex3").format, "RGBA32F");
 const PackTarget& gaux4 = pack.targets.at("colortex7");
 EXPECT_TRUE(gaux4.customClearColor);
 EXPECT_FLOAT_EQ(gaux4.clearColor[0], 0.25f);
 EXPECT_FLOAT_EQ(gaux4.clearColor[1], 0.5f);
 EXPECT_FLOAT_EQ(gaux4.clearColor[2], 0.75f);
 EXPECT_FLOAT_EQ(gaux4.clearColor[3], 1.0f);
}
TEST(PackLoaderTest, ProfilesParseFullTokenGrammar) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "void main(){}"},
                   {"shaders/shaders.properties",
                    "profile.Test=!program.composite FOO:1 BAR=2 !LIGHT PLAIN\n"}},
                  pack,
                  options,
                  error))
     << error;
 ASSERT_EQ(pack.profiles.size(), 1u);
 const PackProfile& profile = pack.profiles.front();
 EXPECT_EQ(profile.name, "Test");
 EXPECT_EQ(profile.values.at("FOO"), "1");
 EXPECT_EQ(profile.values.at("BAR"), "2");
 EXPECT_EQ(profile.values.at("LIGHT"), "false");
 EXPECT_EQ(profile.values.at("PLAIN"), "true");
 ASSERT_EQ(profile.disabledPrograms.size(), 1u);
 EXPECT_EQ(profile.disabledPrograms.front(), "composite");
}
TEST(PackLoaderTest, ImageWithSevenFieldsIs1DAndNoneSamplerIsEmpty) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "void main(){}"},
                   {"shaders/shaders.properties",
                    "iris.features.optional=CUSTOM_IMAGES\n"
                    "image.0=colortex0 RGBA8 RGBA8 UNSIGNED_BYTE true false 64\n"
                    "image.1=none RGBA8 RGBA8 UNSIGNED_BYTE true true 0.5 0.5\n"}},
                  pack,
                  options,
                  error))
     << error;
 ASSERT_EQ(pack.images.size(), 2u);
 const CustomImage& oneD = pack.images.front();
 EXPECT_EQ(oneD.name, "0");
 EXPECT_EQ(oneD.sampler, "colortex0");
 EXPECT_FLOAT_EQ(oneD.width, 64.0f);
 EXPECT_FLOAT_EQ(oneD.height, 1.0f);
 const CustomImage& noneSampler = pack.images.back();
 EXPECT_EQ(noneSampler.sampler, "");
 EXPECT_FLOAT_EQ(noneSampler.width, 0.5f);
}
TEST(PackLoaderTest, ParsesEntityAndItemIdProperties) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "void main(){}"},
                   {"shaders/entity.properties",
                    "entity.1=cow pig\n"
                    "entity.2=minecraft:zombie\n"
                    "entity.-1=player current_player\n"
                    "entity.3=creeper:ignited=true\n"
                    "entity.4=ZombiePigman\n"
                    "garbage.5=ignored\n"
                    "entity.abc=not_a_number\n"},
                   {"shaders/item.properties",
                    "item.256=iron_sword diamond_sword\n"
                    "item.-2=ender_pearl\n"
                    "item.257=golden_sword=bad\n"}},
                  pack,
                  options,
                  error))
     << error;
 EXPECT_EQ(pack.entityIds.at("cow"), 1);
 EXPECT_EQ(pack.entityIds.at("pig"), 1);
 EXPECT_EQ(pack.entityIds.at("minecraft:zombie"), 2);
 // Negative ids are legal (LegacyIdMap uses -123; SEUS PTGI reads mc_Entity.x == -123).
 EXPECT_EQ(pack.entityIds.at("player"), -1);
 EXPECT_EQ(pack.entityIds.at("current_player"), -1);
 EXPECT_EQ(pack.entityIds.at("zombiepigman"), 4);
 // State-property parts ('=') are skipped, exactly like Java parseIdMap.
 EXPECT_FALSE(pack.entityIds.contains("creeper"));
 EXPECT_FALSE(pack.entityIds.contains("ignored"));
 EXPECT_FALSE(pack.entityIds.contains("not_a_number"));
 EXPECT_EQ(pack.itemIds.at("iron_sword"), 256);
 EXPECT_EQ(pack.itemIds.at("diamond_sword"), 256);
 EXPECT_EQ(pack.itemIds.at("ender_pearl"), -2);
 EXPECT_FALSE(pack.itemIds.contains("golden_sword"));
 EXPECT_FALSE(pack.itemIds.contains("golden_sword=bad"));
}
TEST(PackLoaderTest, ParsesBlockIdPropertiesAndRenderLayers) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "void main(){}"},
                   {"shaders/block.properties",
                    "block.1=stone granite\n"
                    "block.9=water flowing_water\n"
                    "block.-123=emerald_block\n"
                    "block.2=grass:waterlogged=true\n"
                    "layer.solid=stone\n"
                    "layer.translucent=water minecraft:ice\n"
                    "layer.cutout_mipped=grass\n"
                    "layer.bogus=bedrock\n"}},
                  pack,
                  options,
                  error))
     << error;
 ASSERT_TRUE(pack.hasBlockProperties);
 EXPECT_EQ(pack.blockIds.at("stone"), 1);
 EXPECT_EQ(pack.blockIds.at("granite"), 1);
 EXPECT_EQ(pack.blockIds.at("water"), 9);
 EXPECT_EQ(pack.blockIds.at("flowing_water"), 9);
 EXPECT_EQ(pack.blockIds.at("emerald_block"), -123);
 // Predicate'd entries cannot match beta block states; Java's addBlockStates
 // finds no such property on beta blocks and adds nothing.
 EXPECT_FALSE(pack.blockIds.contains("grass"));
 EXPECT_FALSE(pack.blockIds.contains("grass:waterlogged=true"));
 // layer.* resolves through the id map AND the vanilla registry ("tile.*" keys,
 // "minecraft:" prefix tolerated, exactly like Java's parseRenderTypeMap).
 EXPECT_EQ(pack.blockRenderLayers.at(1), 0);
 EXPECT_EQ(pack.blockRenderLayers.at(2), 1);
 EXPECT_EQ(pack.blockRenderLayers.at(9), 2);
 EXPECT_EQ(pack.blockRenderLayers.at(79), 2);
 EXPECT_FALSE(pack.blockRenderLayers.contains(7));
}
TEST(PackLoaderTest, IdMapsAreOptionalAndEmptyWithoutPropertiesFiles) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "void main(){}"}},
                  pack,
                  options,
                  error))
     << error;
 EXPECT_FALSE(pack.hasBlockProperties);
 EXPECT_TRUE(pack.entityIds.empty());
 EXPECT_TRUE(pack.itemIds.empty());
 EXPECT_TRUE(pack.blockIds.empty());
}
TEST(PipelineBlockIdsTest, CustomBlockPropertiesUseMinusOneForUnmappedBlocks) {
 Pipeline pipeline(nullptr);
 PackDefinition pack;
 pack.hasBlockProperties = true;
 pack.blockIds.emplace("sand", 2);
 pack.blockIds.emplace("water", 32000);
 const std::uint64_t revision = pipeline.objectIdRevision();
 pipeline.applyBlockIds(pack);
 EXPECT_GT(pipeline.objectIdRevision(), revision);
 EXPECT_EQ(resolveShaderBlockId(1), -1);
 EXPECT_EQ(resolveShaderBlockId(8), 32000);
 EXPECT_EQ(resolveShaderBlockId(9), 32000);
 EXPECT_EQ(resolveShaderBlockId(12), 2);
 pipeline.applyBlockIds(PackDefinition{});
}
TEST(PipelineBlockIdsTest, MissingBlockPropertiesUseLegacyNumericIds) {
 Pipeline pipeline(nullptr);
 PackDefinition pack;
 pipeline.applyBlockIds(pack);
 EXPECT_EQ(resolveShaderBlockId(1), 1);
 EXPECT_EQ(resolveShaderBlockId(12), 12);
 pipeline.applyBlockIds(PackDefinition{});
}
TEST(PackSourcePreparation, MultiTexCoordAliasesMatchIrisTransformers) {
 net::minecraft::test::installTestGlslSnippets();
 PackDefinition pack;
 const std::string src = R"(#version 330 compatibility
void main() {
	gl_Position = ftransform();
	vec4 a = gl_MultiTexCoord0;
	vec4 b = gl_MultiTexCoord1;
	vec4 c = gl_MultiTexCoord2;
	vec4 d = gl_MultiTexCoord3;
	vec4 e = gl_MultiTexCoord5;
}
)";
 const std::string out = prepareSource(
     "gbuffers_terrain", ShaderStage::Vertex, pack, src);
 // gl_MultiTexCoord2 is an alias of gl_MultiTexCoord1 (lightmap), Iris issue 1149.
 EXPECT_EQ(out.find("gl_MultiTexCoord2"), std::string::npos);
 EXPECT_EQ(out.find("gl_MultiTexCoord1"), std::string::npos);
 EXPECT_NE(out.find("vec4(vaUV2, 0.0, 1.0)"), std::string::npos);
 // gl_MultiTexCoord3 is a legacy alias of mc_midTexCoord (CommonTransformer.patchMultiTexCoord3).
 EXPECT_EQ(out.find("gl_MultiTexCoord3"), std::string::npos);
 EXPECT_NE(out.find("mc_midTexCoord"), std::string::npos);
 EXPECT_NE(out.find("in vec4 mc_midTexCoord;"), std::string::npos);
 // Coordinates 4..7 collapse to zero (replaceGlMultiTexCoordBounded(4, 7)).
 EXPECT_EQ(out.find("gl_MultiTexCoord5"), std::string::npos);
 EXPECT_NE(out.find("vec4(0.0, 0.0, 0.0, 1.0)"), std::string::npos);
 // The rewritten references pull in the va* / matrix declarations.
 EXPECT_NE(out.find("in vec3 vaPosition;"), std::string::npos);
 EXPECT_NE(out.find("in vec2 vaUV0;"), std::string::npos);
 EXPECT_NE(out.find("uniform vec3 chunkOffset;"), std::string::npos);
}
TEST(PackSourcePreparation, MidTexCoordAliasUsesCoreInputAndSkipsDeclared) {
 net::minecraft::test::installTestGlslSnippets();
 PackDefinition pack;
 const std::string legacy = "void main() { vec4 d = gl_MultiTexCoord3; }\n";
 const std::string out = prepareSource(
     "gbuffers_terrain", ShaderStage::Vertex, pack, legacy);
 EXPECT_EQ(out.find("gl_MultiTexCoord3"), std::string::npos);
 EXPECT_NE(out.find("in vec4 mc_midTexCoord;"), std::string::npos);
 // A shader that already declares mc_midTexCoord keeps gl_MultiTexCoord3 as-is
 // (Java cannot rewrite an existing declaration either).
 const std::string declared = R"(#version 330 compatibility
in vec4 mc_midTexCoord;
void main() { vec4 d = gl_MultiTexCoord3; }
)";
 const std::string kept = prepareSource(
     "gbuffers_terrain", ShaderStage::Vertex, pack, declared);
 EXPECT_NE(kept.find("gl_MultiTexCoord3"), std::string::npos);
 EXPECT_EQ(kept.find("in vec4 mc_midTexCoord;"), kept.rfind("in vec4 mc_midTexCoord;"));
}
TEST(PackSourcePreparation, FogFragCoordAlwaysRewrittenForCoreTarget) {
 net::minecraft::test::installTestGlslSnippets();
 PackDefinition pack;
 const std::string modern = R"(#version 330 compatibility
void main() { gl_FogFragCoord = gl_Position.z; }
)";
 const std::string vertex = prepareSource(
     "gbuffers_terrain", ShaderStage::Vertex, pack, modern);
 // Java CommonTransformer routes core-profile shaders through a plain in/out pair.
 EXPECT_NE(vertex.find("iris_FogFragCoord = gl_Position.z;"), std::string::npos);
 EXPECT_NE(vertex.find("out float iris_FogFragCoord;"), std::string::npos);
 EXPECT_NE(vertex.find("iris_FogFragCoord = 0.0f;"), std::string::npos);
 const std::string fragment = prepareSource(
     "gbuffers_terrain", ShaderStage::Fragment, pack,
     "#version 330 compatibility\nvoid main() { float d = gl_FogFragCoord; }\n");
 EXPECT_NE(fragment.find("in float iris_FogFragCoord;"), std::string::npos);
 const std::string legacy = "void main() { gl_FogFragCoord = gl_Position.z; }\n";
 const std::string kept = prepareSource(
     "gbuffers_terrain", ShaderStage::Vertex, pack, legacy);
 EXPECT_EQ(kept.find("gl_FogFragCoord"), std::string::npos);
 EXPECT_NE(kept.find("iris_FogFragCoord"), std::string::npos);
}
TEST(PackSourcePreparation, CustomProgramNamesGetVertexRewrite) {
 net::minecraft::test::installTestGlslSnippets();
 // Iris applies the transformers to every program name; the old gate that
 // skipped non-gbuffer/composite/shadow names is gone.
 PackDefinition pack;
 const std::string src = R"(#version 330 compatibility
void main() {
	gl_Position = ftransform();
	vec4 a = gl_MultiTexCoord0;
}
)";
 const std::string out = prepareSource(
     "interface", ShaderStage::Vertex, pack, src);
 EXPECT_EQ(out.find("gl_Vertex"), std::string::npos);
 EXPECT_EQ(out.find("ftransform()"), std::string::npos);
 EXPECT_EQ(out.find("gl_MultiTexCoord0"), std::string::npos);
 EXPECT_NE(out.find("vec4(vaPosition, 1.0)"), std::string::npos);
 EXPECT_NE(out.find("vec4(vaUV0, 0.0, 1.0)"), std::string::npos);
 EXPECT_NE(out.find("in vec3 vaPosition;"), std::string::npos);
}
TEST(PackSourcePreparation, CustomProgram120SourcesUseCoreDialect) {
 net::minecraft::test::installTestGlslSnippets();
 PackDefinition pack;
 const std::string src = "void main() { vec4 v = gl_Vertex; }\n";
 const std::string out = prepareSource(
     "interface", ShaderStage::Vertex, pack, src);
 EXPECT_EQ(out.find("gl_Vertex"), std::string::npos);
 EXPECT_EQ(out.find("attribute vec3 vaPosition;"), std::string::npos);
 EXPECT_NE(out.find("in vec3 vaPosition;"), std::string::npos);
 EXPECT_EQ(out.find("chunkOffset"), std::string::npos);
}
TEST(PackSourcePreparation, SynthesizedRasterVertexIsNativeCoreSource) {
 net::minecraft::test::installTestGlslSnippets();
 PackDefinition pack;
 const std::string out = prepareSource("gbuffers_terrain", ShaderStage::Vertex, pack,
                                       defaultRasterVertexShader());
 EXPECT_EQ(out.find("ftransform"), std::string::npos);
 EXPECT_EQ(out.find("gl_TextureMatrix"), std::string::npos);
 EXPECT_NE(out.find("vaPosition + chunkOffset"), std::string::npos);
 EXPECT_NE(out.find("iris_lightmapTextureMatrix"), std::string::npos);
}
TEST(PackSourcePreparation, CoreDialectRewriteOnlyTouchesStorageQualifiers) {
 PackDefinition pack;
 const std::string source = R"(#version 120
#define attribute keep_macro
attribute vec3 position;
void main() {
 int attribute = 2;
 gl_Position = vec4(position * float(attribute), 1.0);
}
)";
 const std::string out = prepareSource("interface", ShaderStage::Vertex, pack, source);
 EXPECT_NE(out.find("#define attribute keep_macro"), std::string::npos) << out;
 EXPECT_NE(out.find("in vec3 position;"), std::string::npos) << out;
 EXPECT_NE(out.find("int attribute = 2;"), std::string::npos) << out;
}
TEST(PackSourcePreparation, Glsl330RewritesLegacyFragmentSurface) {
 net::minecraft::test::installTestGlslSnippets();
 PackDefinition pack;
 const std::string source = R"(#version 120
varying vec2 uv;
uniform sampler2D image;
void main() {
 gl_FragData [ 2 ] = texture2D(image, gl_TexCoord[0].xy) * gl_Color;
 gl_FragColor = shadow2DLod(shadowtex0, vec3(uv, 0.5), 0.0);
}
)";
 const std::string out = prepareSource(
     "composite", ShaderStage::Fragment, pack, source);
 EXPECT_EQ(out.find("varying"), std::string::npos);
 EXPECT_EQ(out.find("texture2D"), std::string::npos);
 EXPECT_EQ(out.find("gl_TexCoord"), std::string::npos);
 EXPECT_EQ(out.find("gl_Color"), std::string::npos);
 EXPECT_EQ(out.find("gl_FragData"), std::string::npos);
 EXPECT_EQ(out.find("gl_FragColor"), std::string::npos);
 EXPECT_NE(out.find("in vec4 irs_texCoords[3];"), std::string::npos);
 EXPECT_NE(out.find("in vec4 irs_Color;"), std::string::npos);
 EXPECT_NE(out.find("layout(location = 0) out vec4 iris_FragData0;"), std::string::npos);
 EXPECT_NE(out.find("layout(location = 2) out vec4 iris_FragData2;"), std::string::npos);
 EXPECT_NE(out.find("vec4 iris_shadow2DLod"), std::string::npos);
}
TEST(PackSourcePreparation, RasterVersionNegotiatesEveryStage) {
 PackDefinition pack;
 const std::string negotiated = versionPreambleForStages(
     pack,
     {"#version 330 compatibility\n", "#version 440 compatibility\n", "#version 400 core\n"},
     400);
 EXPECT_TRUE(negotiated.starts_with("#version 440 core\n"));
 const std::string floor = versionPreambleForStages(pack, {"#version 120\n"}, 400);
 EXPECT_TRUE(floor.starts_with("#version 400 core\n"));
}
TEST(PackSourcePreparation, EntityOverlayAndIdsShareStageContext) {
 net::minecraft::test::installTestGlslSnippets();
 PackDefinition pack;
 const ShaderTransformContext context{true, true, false, false};
 const std::string vertex = prepareSource(
     "gbuffers_entities", ShaderStage::Vertex, pack,
     "uniform int entityId;\nvoid main() { int id = entityId; gl_Position = vec4(0.0); }\n",
     context);
 const std::string fragment = prepareSource(
     "gbuffers_entities", ShaderStage::Fragment, pack,
     "uniform vec4 entityColor;\nuniform int entityId;\nvoid main() { vec4 c = entityColor; int id = entityId; }\n",
     context);
 EXPECT_NE(vertex.find("uniform sampler2D iris_overlay;"), std::string::npos);
 EXPECT_NE(vertex.find("in ivec2 iris_UV1;"), std::string::npos);
 EXPECT_NE(vertex.find("in ivec3 iris_Entity;"), std::string::npos);
 EXPECT_NE(vertex.find("texelFetch(iris_overlay, iris_UV1, 0)"), std::string::npos);
 EXPECT_EQ(fragment.find("uniform vec4 entityColor;"), std::string::npos);
 EXPECT_EQ(fragment.find("uniform int entityId;"), std::string::npos);
 EXPECT_NE(fragment.find("in vec4 entityColor;"), std::string::npos);
 EXPECT_NE(fragment.find("flat in ivec3 iris_entityInfo;"), std::string::npos);
 EXPECT_NE(fragment.find("iris_entityInfo.x"), std::string::npos);
}
TEST(PackLoaderTest, TexturedLitFallsBackToTextured) {
 // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shaderpack/loading/ProgramId.java
 // TexturedLit -> Textured -> Basic.
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "void main(){}"},
                   {"shaders/gbuffers_textured.vsh", "void main(){}"},
                   {"shaders/gbuffers_textured.fsh", "void main(){}"}},
                  pack,
                  options,
                  error));
 EXPECT_FALSE(pack.programs.contains("gbuffers_textured_lit"));
 EXPECT_EQ(resolved(pack, "gbuffers_textured_lit"), "gbuffers_textured");
}
TEST(PackLoaderTest, ItemFallsBackToTexturedLitNotEntities) {
 // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shaderpack/loading/ProgramId.java
 // Item -> TexturedLit -> Textured -> Basic: entities is not in the chain.
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "void main(){}"},
                   {"shaders/gbuffers_textured_lit.vsh", "void main(){}"},
                   {"shaders/gbuffers_textured_lit.fsh", "void main(){}"},
                   {"shaders/gbuffers_entities.vsh", "void main(){}"},
                   {"shaders/gbuffers_entities.fsh", "void main(){}"}},
                  pack,
                  options,
                  error));
 EXPECT_FALSE(pack.programs.contains("gbuffers_item"));
 EXPECT_EQ(resolved(pack, "gbuffers_item"), "gbuffers_textured_lit");
}
TEST(PackLoaderTest, EntitiesGlowingLoadsOwnSourceAndFallsBackToEntities) {
 // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shaderpack/loading/ProgramId.java
 // EntitiesGlowing -> Entities -> TexturedLit -> Textured -> Basic.
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "void main(){}"},
                   {"shaders/gbuffers_entities_glowing.vsh", "void main(){}"},
                   {"shaders/gbuffers_entities_glowing.fsh", "void main(){}"}},
                  pack,
                  options,
                  error));
 EXPECT_EQ(pack.programs.at("gbuffers_entities_glowing").fragment,
           "shaders/gbuffers_entities_glowing.fsh");
 PackDefinition fallbackPack;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "void main(){}"},
                   {"shaders/gbuffers_entities.vsh", "void main(){}"},
                   {"shaders/gbuffers_entities.fsh", "void main(){}"}},
                  fallbackPack,
                  options,
                  error));
 EXPECT_FALSE(fallbackPack.programs.contains("gbuffers_entities_glowing"));
 EXPECT_EQ(resolved(fallbackPack, "gbuffers_entities_glowing"), "gbuffers_entities");
}
TEST(PackLoaderTest, ShadowWaterFallsBackToShadow) {
 // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shaderpack/loading/ProgramId.java
 // ShadowWater -> Shadow; ShadowSolid/ShadowBlock -> Shadow.
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "void main(){}"},
                   {"shaders/shadow.vsh", "void main(){}"},
                   {"shaders/shadow.fsh", "void main(){}"}},
                  pack,
                  options,
                  error));
 EXPECT_FALSE(pack.programs.contains("shadow_water"));
 EXPECT_FALSE(pack.programs.contains("shadow_solid"));
 EXPECT_FALSE(pack.programs.contains("shadow_block"));
 EXPECT_EQ(resolved(pack, "shadow_water"), "shadow");
 EXPECT_EQ(resolved(pack, "shadow_solid"), "shadow");
 EXPECT_EQ(resolved(pack, "shadow_block"), "shadow");
}
TEST(PackLoaderTest, ShadowLightningFallsBackToEntitiesThenShadow) {
 // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shaderpack/loading/ProgramId.java
 // ShadowLightning -> ShadowEntities -> Shadow.
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "void main(){}"},
                   {"shaders/shadow.vsh", "void main(){}"},
                   {"shaders/shadow.fsh", "void main(){}"},
                   {"shaders/shadow_entities.vsh", "void main(){}"},
                   {"shaders/shadow_entities.fsh", "void main(){}"}},
                  pack,
                  options,
                  error));
 EXPECT_FALSE(pack.programs.contains("shadow_lightning"));
 EXPECT_EQ(resolved(pack, "shadow_lightning"), "shadow_entities");
 PackDefinition rootOnly;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "void main(){}"},
                   {"shaders/shadow.vsh", "void main(){}"},
                   {"shaders/shadow.fsh", "void main(){}"}},
                  rootOnly,
                  options,
                  error));
 EXPECT_EQ(resolved(rootOnly, "shadow_lightning"), "shadow");
}
TEST(PackLoaderTest, ShadowProgramsDefaultBlendOff) {
 // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shaderpack/loading/ProgramId.java
 // Shadow group programs default to BlendModeOverride.OFF; pack blend. wins.
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "void main(){}"},
                   {"shaders/shadow.vsh", "void main(){}"},
                   {"shaders/shadow.fsh", "void main(){}"},
                   {"shaders/shaders.properties", "blend.shadow=ONE ZERO ONE ZERO\n"}},
                  pack,
                  options,
                  error));
 const BufferBlend* shadow = nullptr;
 const BufferBlend* shadowCutout = nullptr;
 for(const BufferBlend& blend : pack.bufferBlends) {
  if(blend.program == "shadow") shadow = &blend;
  if(blend.program == "shadow_cutout") shadowCutout = &blend;
 }
 ASSERT_NE(shadow, nullptr);
 EXPECT_TRUE(shadow->enabled);
 EXPECT_EQ(shadow->source, "ONE");
 EXPECT_EQ(shadowCutout, nullptr);
 EXPECT_EQ(resolved(pack, "shadow_cutout"), "shadow");
}
TEST(PackLoaderTest, SpiderEyesDefaultBlendOverride) {
 // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shaderpack/loading/ProgramId.java
 // SpiderEyes defaults to SRC_ALPHA, ONE / ZERO, ONE unless the pack overrides.
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "void main(){}"},
                   {"shaders/gbuffers_spidereyes.vsh", "void main(){}"},
                   {"shaders/gbuffers_spidereyes.fsh", "void main(){}"}},
                  pack,
                  options,
                  error));
 const BufferBlend* eyes = nullptr;
 for(const BufferBlend& blend : pack.bufferBlends) {
  if(blend.program == "gbuffers_spidereyes") eyes = &blend;
 }
 ASSERT_NE(eyes, nullptr);
 EXPECT_TRUE(eyes->enabled);
 EXPECT_EQ(eyes->source, "srcalpha");
 EXPECT_EQ(eyes->destination, "one");
 EXPECT_EQ(eyes->sourceAlpha, "zero");
 EXPECT_EQ(eyes->destinationAlpha, "one");
 // A pack blend.gbuffers_spidereyes directive wins over the default.
 PackDefinition overridden;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "void main(){}"},
                   {"shaders/gbuffers_spidereyes.vsh", "void main(){}"},
                   {"shaders/gbuffers_spidereyes.fsh", "void main(){}"},
                   {"shaders/shaders.properties", "blend.gbuffers_spidereyes=off\n"}},
                  overridden,
                  options,
                  error));
 bool foundOverride = false;
 for(const BufferBlend& blend : overridden.bufferBlends) {
  if(blend.program == "gbuffers_spidereyes") {
   foundOverride = true;
   EXPECT_FALSE(blend.enabled);
  }
 }
 EXPECT_TRUE(foundOverride);
}
TEST(PackLoaderTest, ProgramEnabledFalseDisablesPassBucket) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "void main(){}"},
                   {"shaders/composite.vsh", "void main(){}"},
                   {"shaders/composite.fsh", "void main(){}"},
                   {"shaders/composite1.vsh", "void main(){}"},
                   {"shaders/composite1.fsh", "void main(){}"},
                   {"shaders/deferred.vsh", "void main(){}"},
                   {"shaders/deferred.fsh", "void main(){}"},
                   {"shaders/shaders.properties",
                    "program.composite.enabled=false\n"
                    "program.composite1.enabled=false\n"}},
                  pack,
                  options,
                  error));
 PackPassBuckets buckets;
 ProgramEnabledCache cache;
 indexPackPasses(pack, {}, buckets, cache);
 EXPECT_TRUE(buckets.postPasses.empty());
 ASSERT_EQ(buckets.deferredPasses.size(), 1u);
 EXPECT_EQ(pack.passes.at(buckets.deferredPasses.front()).name, "deferred");
}
TEST(PackLoaderTest, ProgramEnabledExpressionsMatchBooleanParser) {
 // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shaderpack/parsing/BooleanParser.java
 // Literals true/1/false/0; other tokens are option lookups defaulting to true;
 // parse errors default the whole expression to true.
 const auto evaluateWith =
     [](const std::string& enabledValue, std::unordered_map<std::string, std::string> settings) {
      PackDefinition pack;
      std::unordered_map<std::string, PackSourceOption> options;
      std::string error;
      EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "#define BLOOM\nvoid main(){}"},
                        {"shaders/gbuffers_basic.fsh", "void main(){}"},
                        {"shaders/shaders.properties",
                         "program.deferred.enabled=" + enabledValue + "\n"}},
                       pack,
                       options,
                       error));
      return isProgramEnabled(pack, settings, "deferred");
     };
 EXPECT_FALSE(evaluateWith("!BLOOM", {{"BLOOM", "1"}}));
 EXPECT_TRUE(evaluateWith("!BLOOM", {{"BLOOM", "0"}}));
 // Unset option: the #define default applies (BLOOM defaults to enabled).
 EXPECT_FALSE(evaluateWith("!BLOOM", {}));
 EXPECT_FALSE(evaluateWith("BLOOM && !UNSET", {{"BLOOM", "1"}}));
 EXPECT_TRUE(evaluateWith("BLOOM || !UNSET", {{"BLOOM", "1"}}));
 EXPECT_FALSE(evaluateWith("BLOOM || !UNSET", {{"BLOOM", "0"}}));
 EXPECT_TRUE(evaluateWith("false || (true && 1)", {}));
 // Unknown options default to true.
 EXPECT_TRUE(evaluateWith("UNKNOWN_OPTION", {}));
 // Parse errors default to true.
 EXPECT_TRUE(evaluateWith("BLOOM &&", {{"BLOOM", "0"}}));
 EXPECT_TRUE(evaluateWith("", {}));
 // Compute passes resolve through the #compute suffix: program.composite.enabled
 // controls the composite.csh pass as well.
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "void main(){}"},
                   {"shaders/composite.csh", "layout(local_size_x = 1) in;"},
                   {"shaders/shaders.properties", "program.composite.enabled=false\n"}},
                  pack,
                  options,
                  error));
 EXPECT_FALSE(isProgramEnabled(pack, {}, "composite#compute"));
 EXPECT_TRUE(isProgramEnabled(pack, {}, "deferred#compute"));
}
TEST(PackLoaderTest, IndexPackPassesBucketsStages) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "void main(){}"},
                   {"shaders/begin.vsh", "void main(){}"},
                   {"shaders/begin.fsh", "void main(){}"},
                   {"shaders/shadowcomp.vsh", "void main(){}"},
                   {"shaders/shadowcomp.fsh", "void main(){}"},
                   {"shaders/prepare.vsh", "void main(){}"},
                   {"shaders/prepare.fsh", "void main(){}"},
                   {"shaders/deferred.vsh", "void main(){}"},
                   {"shaders/deferred.fsh", "void main(){}"},
                   {"shaders/composite.vsh", "void main(){}"},
                   {"shaders/composite.fsh", "void main(){}"},
                   {"shaders/final.vsh", "void main(){}"},
                   {"shaders/final.fsh", "void main(){}"}},
                  pack,
                  options,
                  error));
 PackPassBuckets buckets;
 ProgramEnabledCache cache;
 indexPackPasses(pack, {}, buckets, cache);
 const auto names = [&pack](const std::vector<std::size_t>& indexes) {
  std::vector<std::string> result;
  for(std::size_t index : indexes) result.push_back(pack.passes.at(index).name);
  return result;
 };
 EXPECT_EQ(names(buckets.beginPasses), (std::vector<std::string>{"begin"}));
 EXPECT_EQ(names(buckets.shadowCompositePasses), (std::vector<std::string>{"shadowcomp"}));
 EXPECT_EQ(names(buckets.preparePasses), (std::vector<std::string>{"prepare"}));
 EXPECT_EQ(names(buckets.deferredPasses), (std::vector<std::string>{"deferred"}));
 // final lands in the post bucket together with composite (Iris CompositeRenderer +
 // FinalPassRenderer both run in the post-process stage).
 EXPECT_EQ(names(buckets.postPasses), (std::vector<std::string>{"composite", "final"}));
}
TEST(PackLoaderTest, LessComputeOrderMatchesIrisSuffixGrammar) {
 // Mirrors addComputePrograms: pass.order is seeded from computePassOrder(name).
 const auto order = [](std::string a, std::string b) {
  PackPass passA;
  passA.name = std::move(a);
  passA.order = ComputeDispatcher::computePassOrder(passA.name);
  PackPass passB;
  passB.name = std::move(b);
  passB.order = ComputeDispatcher::computePassOrder(passB.name);
  return ComputeDispatcher::lessComputeOrder(passA, passB);
 };
 EXPECT_TRUE(order("composite", "composite_a"));
 EXPECT_TRUE(order("composite_a", "composite_b"));
 EXPECT_TRUE(order("composite_b", "composite1"));
 EXPECT_TRUE(order("composite1", "composite1_a"));
 EXPECT_TRUE(order("composite1_a", "composite1_b"));
 EXPECT_TRUE(order("final", "final_a"));
 EXPECT_FALSE(order("deferred99_z", "composite"));
 EXPECT_FALSE(order("composite", "composite"));
 // Explicit computeOrder directives take precedence over the name grammar.
 PackPass explicitA;
 explicitA.name = "composite_z";
 explicitA.order = 1;
 PackPass explicitB;
 explicitB.name = "composite_a";
 explicitB.order = 0;
 EXPECT_FALSE(ComputeDispatcher::lessComputeOrder(explicitA, explicitB));
 EXPECT_TRUE(ComputeDispatcher::lessComputeOrder(explicitB, explicitA));
}
TEST(PackLoaderTest, ShadowProgramMappingMatchesIrisPipelines) {
 // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/pipeline/IrisPipelines.java
 EXPECT_EQ(irisShadowProgramForGbuffers("gbuffers_terrain_solid"), "shadow_solid");
 EXPECT_EQ(irisShadowProgramForGbuffers("gbuffers_terrain_cutout"), "shadow_cutout");
 EXPECT_EQ(irisShadowProgramForGbuffers("gbuffers_water"), "shadow_water");
 EXPECT_EQ(irisShadowProgramForGbuffers("gbuffers_entities"), "shadow_entities");
 EXPECT_EQ(irisShadowProgramForGbuffers("gbuffers_entities_translucent"), "shadow_entities");
 EXPECT_EQ(irisShadowProgramForGbuffers("gbuffers_item"), "shadow_entities");
 EXPECT_EQ(irisShadowProgramForGbuffers("gbuffers_armor_glint"), "shadow_entities");
 EXPECT_EQ(irisShadowProgramForGbuffers("gbuffers_spidereyes"), "shadow_entities");
 EXPECT_EQ(irisShadowProgramForGbuffers("gbuffers_text"), "shadow_entities");
 EXPECT_EQ(irisShadowProgramForGbuffers("gbuffers_lightning"), "shadow_lightning");
 EXPECT_EQ(irisShadowProgramForGbuffers("gbuffers_block"), "shadow_block");
 EXPECT_EQ(irisShadowProgramForGbuffers("gbuffers_block_translucent"), "shadow_block");
 // CRUMBLING (damagedblock) renders with SHADOW_TEX -> the root shadow program.
 EXPECT_EQ(irisShadowProgramForGbuffers("gbuffers_damagedblock"), "shadow");
 EXPECT_EQ(irisShadowProgramForGbuffers("gbuffers_particles"), "shadow");
 EXPECT_EQ(irisShadowProgramForGbuffers("gbuffers_basic"), "shadow");
 // No shadow mapping in Iris: sky, clouds, hands and GUI variants.
 EXPECT_EQ(irisShadowProgramForGbuffers("gbuffers_skybasic"), "");
 EXPECT_EQ(irisShadowProgramForGbuffers("gbuffers_clouds"), "");
 EXPECT_EQ(irisShadowProgramForGbuffers("gbuffers_hand"), "");
 EXPECT_EQ(irisShadowProgramForGbuffers("gbuffers_gui"), "");
 PackDefinition programs;
 programs.programs["shadow"] = {};
 programs.programs["shadow_entities"] = {};
 programs.programs["shadow_cutout"] = {};
 programs.programs["shadow_solid"] = {};
 EXPECT_EQ(resolved(programs, irisShadowProgramForGbuffers("gbuffers_terrain_solid")), "shadow_solid");
 EXPECT_EQ(resolved(programs, irisShadowProgramForGbuffers("gbuffers_lightning")), "shadow_entities");
 EXPECT_EQ(resolved(programs, irisShadowProgramForGbuffers("gbuffers_skybasic")), "");
 PackDefinition rootOnly;
 rootOnly.programs["shadow"] = {};
 EXPECT_EQ(resolved(rootOnly, irisShadowProgramForGbuffers("gbuffers_lightning")), "shadow");
}
TEST(PackLoaderTest, ProgramFallbackKeyMatchesProgramIdEnum) {
 // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shaderpack/loading/ProgramId.java
 EXPECT_EQ(programFallbackKey("gbuffers_textured_lit"), "gbuffers_textured");
 EXPECT_EQ(programFallbackKey("gbuffers_textured"), "gbuffers_basic");
 EXPECT_EQ(programFallbackKey("gbuffers_terrain_solid"), "gbuffers_terrain");
 EXPECT_EQ(programFallbackKey("gbuffers_terrain"), "gbuffers_textured_lit");
 EXPECT_EQ(programFallbackKey("gbuffers_block_translucent"), "gbuffers_block");
 EXPECT_EQ(programFallbackKey("gbuffers_block"), "gbuffers_terrain");
 EXPECT_EQ(programFallbackKey("gbuffers_hand_water"), "gbuffers_hand");
 EXPECT_EQ(programFallbackKey("gbuffers_hand"), "gbuffers_textured_lit");
 EXPECT_EQ(programFallbackKey("gbuffers_entities_glowing"), "gbuffers_entities");
 EXPECT_EQ(programFallbackKey("gbuffers_entities"), "gbuffers_textured_lit");
 EXPECT_EQ(programFallbackKey("gbuffers_particles_translucent"), "gbuffers_particles");
 EXPECT_EQ(programFallbackKey("gbuffers_spidereyes"), "gbuffers_textured");
 EXPECT_EQ(programFallbackKey("shadow_lightning"), "shadow_entities");
 EXPECT_EQ(programFallbackKey("shadow_cutout"), "shadow");
 EXPECT_EQ(programFallbackKey("gbuffers_basic"), "");
 EXPECT_EQ(programFallbackKey("shadow"), "");
 EXPECT_EQ(programFallbackKey("final"), "");
}
TEST(PackLoaderTest, ProgramResolverSkipsMissingAndDisabledSources) {
 PackDefinition pack;
 pack.programs["gbuffers_basic"] = {};
 pack.programs["gbuffers_textured"] = {};
 pack.programs["gbuffers_terrain"] = {};
 pack.programEnabled["gbuffers_terrain"] = "false";
 EXPECT_EQ(resolved(pack, "gbuffers_terrain_solid"), "gbuffers_textured");
}
TEST(PackLoaderTest, DefaultsMatchJavaPackDirectives) {
 // A pack with no shadow/weather directives must inherit the Java defaults:
 // PackShadowDirectives.java:48-92 (resolution 1024 via loadProgramSet fallback,
 // distance 160, nearPlane ShadowMatrices.NEAR = -100.05, farPlane 156,
 // shouldRenderPlayer/shouldRenderLightBlockEntities false) and
 // PackDirectives.java:59-66 (wetnessHalfLife 600, eyeBrightnessHalfLife 10,
 // centerDepthHalfLife 1). drynessHalfLife is final at 200 and has no PackDefinition
 // field because no pack can reach it.
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "void main(){}"}},
                  pack,
                  options,
                  error))
     << error;
 EXPECT_FALSE(pack.shadowPlayer);
 EXPECT_TRUE(pack.shadowEntities);
 EXPECT_TRUE(pack.shadowTerrain);
 EXPECT_TRUE(pack.shadowTranslucent);
 EXPECT_TRUE(pack.shadowBlockEntities);
 EXPECT_FALSE(pack.shadowLightBlockEntities);
 EXPECT_FLOAT_EQ(pack.shadowDistance, 160.0f);
 EXPECT_FLOAT_EQ(pack.shadowNearPlane, -100.05f);
 EXPECT_FLOAT_EQ(pack.shadowFarPlane, 156.0f);
 EXPECT_FLOAT_EQ(pack.voxelDistance, 0.0f);
 EXPECT_FLOAT_EQ(pack.entityShadowDistanceMul, 1.0f);
 EXPECT_FLOAT_EQ(pack.shadowDistanceRenderMul, -1.0f);
 EXPECT_FLOAT_EQ(pack.shadowIntervalSize, 2.0f);
 EXPECT_FLOAT_EQ(pack.wetnessHalflife, 600.0f);
 EXPECT_FLOAT_EQ(pack.centerDepthHalflife, 1.0f);
 EXPECT_FLOAT_EQ(pack.eyeBrightnessHalflife, 10.0f);
}
TEST(PackLoaderTest, DrynessHalflifeDirectiveAssignsWetnessLikeJava) {
 // PackDirectives.java:283-284 - the drynessHalflife lambda assigns this.wetnessHalfLife,
 // so the later of the two directives wins and dryness stays pinned at its final 200.
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh",
                    "const float wetnessHalflife = 600.0;\n"
                    "const float drynessHalflife = 300.0;\n"
                    "void main(){}"}},
                  pack,
                  options,
                  error))
     << error;
 EXPECT_FLOAT_EQ(pack.wetnessHalflife, 300.0f);
}
TEST(PackLoaderTest, ConstDirectivesAreStoredRawLikeJava) {
 // PackShadowDirectives.java:302-321 and PackDirectives.java:270-289 assign every one of
 // these straight through. ambientOcclusionLevel is the ONLY clamped directive (:277), so
 // nothing else may be clamped at parse time - sizes are clamped where they allocate.
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh",
                    "const float shadowDistance = -16.0;\n"
                    "const float voxelDistance = -1.0;\n"
                    "const float shadowIntervalSize = -2.0;\n"
                    "const float entityShadowDistanceMul = 0.005;\n"
                    "const float wetnessHalflife = -5.0;\n"
                    "const float eyeBrightnessHalflife = -3.0;\n"
                    "const float centerDepthHalflife = -1.0;\n"
                    "const float ambientOcclusionLevel = 4.0;\n"
                    "const int noiseTextureResolution = 8192;\n"
                    "const int shadowMapResolution = 99999;\n"
                    "void main(){}"}},
                  pack,
                  options,
                  error))
     << error;
 EXPECT_FLOAT_EQ(pack.shadowDistance, -16.0f);
 EXPECT_FLOAT_EQ(pack.voxelDistance, -1.0f);
 EXPECT_FLOAT_EQ(pack.shadowIntervalSize, -2.0f);
 EXPECT_FLOAT_EQ(pack.entityShadowDistanceMul, 0.005f);
 EXPECT_FLOAT_EQ(pack.wetnessHalflife, -5.0f);
 EXPECT_FLOAT_EQ(pack.eyeBrightnessHalflife, -3.0f);
 EXPECT_FLOAT_EQ(pack.centerDepthHalflife, -1.0f);
 EXPECT_TRUE(pack.usesCenterDepthSmooth);
 EXPECT_EQ(pack.noiseTextureResolution, 8192);
 EXPECT_EQ(pack.shadowMapResolution, 99999);
 EXPECT_FLOAT_EQ(pack.ambientOcclusionLevel, 1.0f);
}
TEST(PackLoaderTest, RawCustomTextureWithUnreadableFormatFailsTheLoad) {
 // ShaderProperties.java:467 - InternalTextureFormat/PixelFormat/PixelType .fromString()
 // .orElseThrow() sits in no try/catch, so a malformed raw texture takes the pack down
 // rather than being silently dropped.
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 EXPECT_FALSE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                    {"shaders/gbuffers_basic.fsh", "void main(){}"},
                    {"shaders/shaders.properties",
                     "texture.composite.noisetex=noise.png TEXTURE_2D NOT_A_FORMAT 32 32 RGBA "
                     "UNSIGNED_BYTE\n"}},
                   pack,
                   options,
                   error));
 EXPECT_NE(error.find("unreadable"), std::string::npos);
}
TEST(PackLoaderTest, ImageBooleanFieldsUseJavaParseBooleanNotShaderPropertiesBoolean) {
 // ShaderProperties.java:537-539 reads image.N's clear/relative fields with
 // Boolean.parseBoolean directly, NOT the handleBooleanValue helper the rest of
 // shaders.properties uses - "1" is true for a shaders.properties boolean key but
 // false here, since Boolean.parseBoolean only recognizes case-insensitive "true".
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "void main(){}"},
                   {"shaders/shaders.properties",
                    "iris.features.optional=CUSTOM_IMAGES\n"
                    "image.0=colortex0 RGBA8 RGBA8 UNSIGNED_BYTE 1 1 64\n"
                    "image.1=colortex0 RGBA8 RGBA8 UNSIGNED_BYTE TRUE FALSE 64\n"}},
                  pack,
                  options,
                  error))
     << error;
 ASSERT_EQ(pack.images.size(), 2u);
 EXPECT_FALSE(pack.images[0].clearEachFrame);
 EXPECT_FALSE(pack.images[0].relative);
 EXPECT_TRUE(pack.images[1].clearEachFrame);
 EXPECT_FALSE(pack.images[1].relative);
}
TEST(PackLoaderTest, BufferObjectRelativeFieldUsesJavaParseBoolean) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "void main(){}"},
                   {"shaders/shaders.properties",
                    "iris.features.optional=SSBO\nbufferObject.0=16 1 0.5 0.5\n"}},
                  pack,
                  options,
                  error))
     << error;
 ASSERT_EQ(pack.bufferObjects.size(), 1u);
 EXPECT_FALSE(pack.bufferObjects.front().relative);
}
TEST(PackLoaderTest, EncodedCustomTextureNeedsNoFormatFields) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "void main(){}"},
                   {"shaders/shaders.properties", "texture.composite.noisetex=noise.png\n"}},
                  pack,
                  options,
                  error))
     << error;
 ASSERT_EQ(pack.customTextures.size(), 1u);
 EXPECT_TRUE(pack.customTextures.front().encoded);
}
TEST(PackLoaderTest, ShadowNearFarPlanesAcceptNegativeNearLikeJava) {
 // PackShadowDirectives.java:309-310 assigns the raw const value with no clamp;
 // the Java default near plane itself is negative (ShadowMatrices.NEAR = -100.05).
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh",
                    "const float shadowNearPlane = -100.05;\n"
                    "const float shadowFarPlane = 156.0;\n"
                    "void main(){}"}},
                  pack,
                  options,
                  error))
     << error;
 EXPECT_FLOAT_EQ(pack.shadowNearPlane, -100.05f);
 EXPECT_FLOAT_EQ(pack.shadowFarPlane, 156.0f);
}
TEST(PackLoaderTest, OptionValuesReachScannedPackConstants) {
 // Engine-side shadow tuning must follow the pack options the user set, the same
 // way the compiled GLSL does (Iris parses programs after option replacement).
 // RenderPearl computes shadowDistance through a `#if SM_DIST == N` ladder in
 // directive.glsl; SM_DIST itself is a #define option in config.glsl. Constants
 // scanned from raw text always picked the pack's shipped SM_DIST, so moving the
 // slider changed the shader-side fade but never the shadow camera or the culling
 // sphere.
 const std::unordered_map<std::string, std::string> sources = {
     {"shaders/gbuffers_basic.vsh", "void main(){}"},
     {"shaders/gbuffers_basic.fsh",
      "#define SM_DIST 10 // [0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32]\n"
      "#if SM_DIST == 1\n"
      "\tconst float shadowDistance = 16;\n"
      "#elif SM_DIST == 10\n"
      "\tconst float shadowDistance = 160;\n"
      "#elif SM_DIST == 16\n"
      "\tconst float shadowDistance = 256;\n"
      "#endif\n"
      "void main(){}"}};
 const auto run = [&sources](const std::unordered_map<std::string, std::string>& values,
                             PackDefinition& pack,
                             std::unordered_map<std::string, PackSourceOption>& options) {
  std::vector<std::string> paths;
  for(const auto& [path, ignored] : sources) paths.push_back(path);
  std::string error;
  return PackLoader::load(paths, [&sources](std::string_view path) {
                           const auto found = sources.find(std::string(path));
                           return found == sources.end() ? std::string{} : found->second; }, pack, options, error, values);
 };
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 ASSERT_TRUE(run({}, pack, options));
 EXPECT_FLOAT_EQ(pack.shadowDistance, 160.0f);
 PackDefinition changed;
 std::unordered_map<std::string, PackSourceOption> changedOptions;
 ASSERT_TRUE(run({{"SM_DIST", "16"}}, changed, changedOptions));
 EXPECT_FLOAT_EQ(changed.shadowDistance, 256.0f);
}
TEST(PackLoaderTest, DynamicHandLightIsParityParsed) {
 // Java ShaderProperties.java:83,190,756-757 parses dynamicHandLight and never
 // consumes it; the C++ side keeps the parity parse so packs behave identically.
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "void main(){}"},
                   {"shaders/shaders.properties", "dynamicHandLight=true\n"}},
                  pack,
                  options,
                  error))
     << error;
 EXPECT_TRUE(pack.dynamicHandLight);
}
// mc_chunkFade must follow the geometry source, not the program-name prefix. Iris routes
// every chunk-mesher program through SodiumTransformer, which computes a real fade (1.0
// for a loaded chunk); gbuffers_water is chunk geometry and belongs in that set. Giving
// it the non-terrain `const float mc_chunkFade = -1.0;` made the pack's
// `alpha = mc_chunkFade` negative, which packed to 0 and let `blend.gbuffers_water=
// SRC_ALPHA ...` erase every water and ice fragment.
// https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/pipeline/transform/transformer/SodiumTransformer.java
TEST(PackLoaderTest, ChunkFadeFollowsChunkMesherProgramsNotNamePrefix) {
 net::minecraft::test::installTestGlslSnippets();
 PackDefinition pack;
 pack.optionalFeatures.insert("FADE_VARIABLE");
 const ShaderTransformContext context{};
 const std::string body = "void main(){}\n";
 const auto vertexFor = [&](const char* program) {
  return canonicalizeCoreSource(program, ShaderStage::Vertex, pack, body, context);
 };
 // Chunk-mesher programs get the real attribute, which RenderCore seeds to 1.0.
 for(const char* program :
     {"gbuffers_terrain", "gbuffers_terrain_solid", "gbuffers_terrain_cutout", "gbuffers_water"}) {
  const std::string out = vertexFor(program);
  EXPECT_NE(out.find("in float mc_chunkFade;"), std::string::npos) << program;
  EXPECT_EQ(out.find("const float mc_chunkFade"), std::string::npos) << program;
 }
 // Everything else — and the shadow pass, whatever its geometry — gets the -1.0 const.
 for(const char* program : {"gbuffers_entities", "gbuffers_hand", "gbuffers_clouds", "shadow",
                            "shadow_water", "shadow_solid"}) {
  const std::string out = vertexFor(program);
  EXPECT_NE(out.find("const float mc_chunkFade = -1.0;"), std::string::npos) << program;
  EXPECT_EQ(out.find("in float mc_chunkFade;"), std::string::npos) << program;
 }
 // A pack that declares the symbol itself is left alone.
 PackDefinition declaredPack;
 declaredPack.optionalFeatures.insert("FADE_VARIABLE");
 const std::string declared = canonicalizeCoreSource(
     "gbuffers_water", ShaderStage::Vertex, declaredPack, "in float mc_chunkFade;\n" + body, context);
 EXPECT_EQ(declared.find("const float mc_chunkFade"), std::string::npos);
 // No FADE_VARIABLE means no injection at all (the vanilla and SEUS case).
 PackDefinition noFeature;
 const std::string untouched =
     canonicalizeCoreSource("gbuffers_water", ShaderStage::Vertex, noFeature, body, context);
 EXPECT_EQ(untouched.find("mc_chunkFade"), std::string::npos);
}
// A dimension folder must inherit consts that live in the pack-wide includes. Some packs
// keeps its programs in world_default/ but declares `const int shadowMapResolution` in
// prelude/config.glsl; the dimension scan only saw its own folder, came out at 0, fell
// through to the 1024 default, and then overrode the root's correct value — allocating a
// 1024 shadow map against GLSL compiled for 2048, which reads as shadow acne.
TEST(PackLoaderTest, DimensionInheritsShadowMapResolutionFromSharedInclude) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/dimension.properties", "dimension.world_default=*\n"},
                   {"shaders/prelude/config.glsl", "const int shadowMapResolution = 2048;\n"},
                   {"shaders/prog/lit.fsh", "#include \"/prelude/config.glsl\"\nvoid main(){}\n"},
                   {"shaders/world_default/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/world_default/gbuffers_basic.fsh", "#include \"/prog/lit.fsh\"\n"},
                   {"shaders/world_default/shadow.vsh", "void main(){}"},
                   {"shaders/world_default/shadow.fsh", "#include \"/prog/lit.fsh\"\n"}},
                  pack,
                  options,
                  error))
     << error;
 EXPECT_EQ(pack.shadowMapResolution, 2048);
 const auto dimension = pack.dimensionDefinitions.find("*");
 ASSERT_NE(dimension, pack.dimensionDefinitions.end());
 ASSERT_NE(dimension->second, nullptr);
 // Not 1024: the dimension resolves includes before scanning, so the shared const wins
 // and the "pack ships a shadow program but declared no resolution" default never fires.
 EXPECT_EQ(dimension->second->shadowMapResolution, 2048);
}
TEST(PackLoaderTest, RethinkingVoxelsPrepareProgramTransformsCleanly) {
 PackDefinition pack;
 ShaderTransformContext ctx{false, false, false, false};
 std::string prepareGlsl =
     "layout(r32ui) uniform writeonly uimage2D colorimg9;\n"
     "void main() {\n"
     "    imageStore(colorimg9, ivec2(gl_FragCoord.xy), uvec4(1<<31));\n"
     "    gl_FragData[0] = vec4(2);\n"
     "}\n"
     "#ifdef VERTEX_SHADER\n"
     "void main() {\n"
     "    gl_Position = ftransform();\n"
     "}\n"
     "#endif\n";
 std::string prepareVsh =
     "#version 430 compatibility\n"
     "#define VERTEX_SHADER\n" +
     prepareGlsl;
 std::string transformed = prepareSource("prepare", ShaderStage::Vertex, pack, prepareVsh, ctx);
 EXPECT_NE(transformed.find("uniform mat4 projectionMatrix;"), std::string::npos);
 EXPECT_NE(transformed.find("uniform mat4 modelViewMatrix;"), std::string::npos);
 EXPECT_NE(transformed.find("in vec3 vaPosition;"), std::string::npos);
}
} // namespace net::minecraft::client::render
