#include <gtest/gtest.h>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>
#include "net/minecraft/client/render/camera/FrameRenderCamera.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/shaders/CustomUniforms.hpp"
#include "net/minecraft/client/render/shaders/WorldProgramId.hpp"
#include "net/minecraft/client/render/uniforms/FrameData.hpp"
namespace net::minecraft::test {
namespace {
std::string read(const std::filesystem::path& path) {
 std::ifstream input(path, std::ios::binary);
 return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}
// P-FOGMODE: Java Iris uploads GL fog-mode constants (0 / GL_LINEAR / GL_EXP2)
// where the engine keeps internal 1/2/3. The FrameData producer transforms the
// internal mode before it reaches the shader.
client::render::PackUniformValues frameForFog(const client::render::core::FogUniforms& fog) {
 client::render::core::setFog(fog);
 client::render::FrameRenderCamera camera;
 client::render::FrameRenderCamera shadow;
 return client::render::buildShaderFrameData(1280, 720, 42.25f, 0, false, false, camera, shadow, nullptr);
}
// The FogMode column of one ShaderKey row, by enum-constant name.
std::string irisFogMode(const std::string& source, std::string_view key) {
 std::size_t line = 0;
 while(line < source.size()) {
  const std::size_t end = source.find('\n', line);
  const std::string_view row(source.data() + line, (end == std::string::npos ? source.size() : end) - line);
  line = end == std::string::npos ? source.size() : end + 1;
  const std::size_t name = row.find_first_not_of(" \t");
  if(name == std::string_view::npos) continue;
  const std::size_t open = row.find('(', name);
  if(open == std::string_view::npos || row.substr(name, open - name) != key) continue;
  const std::size_t mode = row.find("FogMode.");
  if(mode == std::string_view::npos) return "<no FogMode column>";
  const std::size_t modeEnd = row.find_first_of(",)", mode);
  return std::string(row.substr(mode, modeEnd - mode));
 }
 return "<no such key>";
}
} // namespace
TEST(FogModeParity, FrameDataProducerEmitsGlConstants) {
 constexpr int kGlLinear = 9729;
 constexpr int kGlExp2 = 2049;
 client::render::core::FogUniforms linear;
 linear.enabled = true;
 linear.mode = 1;
 linear.shape = 0;
 EXPECT_EQ(frameForFog(linear).fogMode, kGlLinear);
 EXPECT_EQ(frameForFog(linear).fogShape, 0);
 client::render::core::FogUniforms cylindrical;
 cylindrical.enabled = true;
 cylindrical.mode = 1;
 cylindrical.shape = 1;
 EXPECT_EQ(frameForFog(cylindrical).fogShape, 1);
 client::render::core::FogUniforms exp2;
 exp2.enabled = true;
 exp2.mode = 3;
 EXPECT_EQ(frameForFog(exp2).fogMode, kGlExp2);
 client::render::core::FogUniforms exp;
 exp.enabled = true;
 exp.mode = 2;
 EXPECT_EQ(frameForFog(exp).fogMode, kGlExp2);
 client::render::core::FogUniforms off;
 EXPECT_EQ(frameForFog(off).fogMode, 0);
 EXPECT_EQ(frameForFog(off).fogShape, -1);
}
TEST(FogModeParity, RenderCoreProducerUsesSameMapping) {
 EXPECT_EQ(client::render::core::fogModeToGlConstant(1), 0x2601);
 EXPECT_EQ(client::render::core::fogModeToGlConstant(2), 0x0801);
 EXPECT_EQ(client::render::core::fogModeToGlConstant(3), 0x0801);
}
TEST(FogModeParity, CustomUniformsFogModeMatchesFrameValue) {
 std::vector<client::render::CustomUniformDecl> decls = {
     {"fogModeCopy", client::render::CustomUniformType::Int, true, "fogMode"}};
 client::render::CustomUniformRuntime runtime;
 std::string error;
 ASSERT_TRUE(runtime.compile(decls, error)) << error;
 client::render::PackUniformValues frame;
 frame.fogMode = 9729;
 runtime.evaluate(frame);
 EXPECT_EQ(runtime.values().at("fogModeCopy").asInt(), 9729);
 frame.fogMode = 2049;
 runtime.evaluate(frame);
 EXPECT_EQ(runtime.values().at("fogModeCopy").asInt(), 2049);
 frame.fogMode = 0;
 runtime.evaluate(frame);
 EXPECT_EQ(runtime.values().at("fogModeCopy").asInt(), 0);
}
TEST(FogModeParity, VanillaCommonGlslDecodesGlConstants) {
  const std::filesystem::path shaders =
      std::filesystem::path(MINECRAFT_TEST_SOURCE_DIR) / "src" / "net" / "minecraft" / "client" / "render" / "shaders" / "glsl" / "vanilla" / "shaders";
  const std::string source = read(shaders / "lib" / "common.glsl");
 EXPECT_NE(source.find("fogMode == 9729"), std::string::npos);
 EXPECT_NE(source.find("fogMode == 2049"), std::string::npos);
 // Substring-safe: "fogMode == 2" would also match "fogMode == 2049".
 EXPECT_EQ(source.find("fogMode == 1)"), std::string::npos);
 EXPECT_EQ(source.find("fogMode == 2)"), std::string::npos);
 EXPECT_EQ(source.find("fogMode == 3)"), std::string::npos);
 EXPECT_EQ(source.find("as Iris reports them"), std::string::npos);
}
// Beta's fog coordinate is fixed-function GL_FRAGMENT_DEPTH — the eye-plane distance.
// Radial made the wall 1.74x closer at the screen corners than at the centre, so every
// program has to take its fog coordinate from fogCoord(), never from length().
TEST(FogModeParity, VanillaProgramsUsePlanarFogCoordinate) {
 const std::filesystem::path shaders = std::filesystem::path(MINECRAFT_TEST_SOURCE_DIR) / "src" / "net" /
                                       "minecraft" / "client" / "render" / "shaders" / "glsl" / "vanilla" /
                                       "shaders";
 const std::string common = read(shaders / "lib" / "common.glsl");
 EXPECT_NE(common.find("float fogCoord(vec3 viewPosition)"), std::string::npos);
 EXPECT_NE(common.find("return abs(viewPosition.z);"), std::string::npos);
 int programsWithFog = 0;
 for(const auto& entry : std::filesystem::directory_iterator(shaders)) {
  if(entry.path().extension() != ".vsh") {
   continue;
  }
  const std::string source = read(entry.path());
  const std::size_t assignment = source.find("viewDistance = ");
  if(assignment == std::string::npos) {
   continue;
  }
  ++programsWithFog;
  EXPECT_EQ(source.compare(assignment, std::strlen("viewDistance = fogCoord("), "viewDistance = fogCoord("), 0)
      << entry.path().filename().string() << " must take its fog coordinate from fogCoord()";
 }
 EXPECT_EQ(programsWithFog, 9);
}
// P-FOGCLASS: Iris hangs the fog mode on the ShaderKey -- the kind of draw -- and never on
// the program the key resolves to, so SKY_BASIC stays FogMode.OFF even when
// gbuffers_skybasic falls back through gbuffers_basic. Keying this off the resolved
// program name instead handed forced fog to both sky programs and to the block-damage
// overlay. see third_party/iris/.../pipeline/programs/ShaderKey.java
TEST(FogModeParity, WorldProgramFogClassMatchesShaderKeyTable) {
 using client::render::fogEnabled;
 using client::render::WorldProgramId;
 // FogMode.OFF
 EXPECT_FALSE(fogEnabled(WorldProgramId::SkyBasic));     // SKY_BASIC, SKY_BASIC_COLOR
 EXPECT_FALSE(fogEnabled(WorldProgramId::SkyTextured));  // SKY_TEXTURED, SKY_TEXTURED_COLOR
 EXPECT_FALSE(fogEnabled(WorldProgramId::Textured));     // TEXTURED, TEXTURED_COLOR
 EXPECT_FALSE(fogEnabled(WorldProgramId::DamagedBlock)); // CRUMBLING
 EXPECT_FALSE(fogEnabled(WorldProgramId::Gui));          // no Iris key: the interface
 EXPECT_FALSE(fogEnabled(WorldProgramId::GuiTextured));  // never goes through gbuffers
 // FogMode.PER_VERTEX / PER_FRAGMENT
 EXPECT_TRUE(fogEnabled(WorldProgramId::TerrainSolid));         // TERRAIN_SOLID
 EXPECT_TRUE(fogEnabled(WorldProgramId::TerrainCutout));        // TERRAIN_CUTOUT
 EXPECT_TRUE(fogEnabled(WorldProgramId::TerrainTranslucent));   // TERRAIN_TRANSLUCENT
 EXPECT_TRUE(fogEnabled(WorldProgramId::Item));
 EXPECT_TRUE(fogEnabled(WorldProgramId::Text));                 // TEXT
 EXPECT_TRUE(fogEnabled(WorldProgramId::Basic));                // BASIC, LEASH
 EXPECT_TRUE(fogEnabled(WorldProgramId::Line));                 // LINES
 EXPECT_TRUE(fogEnabled(WorldProgramId::Entities));             // ENTITIES_*
 EXPECT_TRUE(fogEnabled(WorldProgramId::EntitiesTranslucent));  // ENTITIES_TRANSLUCENT
 EXPECT_TRUE(fogEnabled(WorldProgramId::Lightning));            // LIGHTNING
 EXPECT_TRUE(fogEnabled(WorldProgramId::Block));                // BLOCK_ENTITY
 EXPECT_TRUE(fogEnabled(WorldProgramId::BlockTranslucent));     // BE_TRANSLUCENT
 EXPECT_TRUE(fogEnabled(WorldProgramId::Particles));            // PARTICLES
 EXPECT_TRUE(fogEnabled(WorldProgramId::ParticlesTranslucent)); // PARTICLES_TRANS
 EXPECT_TRUE(fogEnabled(WorldProgramId::Clouds));               // CLOUDS
 EXPECT_TRUE(fogEnabled(WorldProgramId::Weather));              // WEATHER
 EXPECT_TRUE(fogEnabled(WorldProgramId::Hand));                 // HAND_*
}
// The table above is only worth anything while it still says what Iris says, so read the
// rows it was copied from. Both directions are checked: a source that stopped naming
// FogMode at all must not read as "everything is OFF".
TEST(FogModeParity, IrisShaderKeyRowsStillCarryTheseFogModes) {
 const std::string source =
     read(std::filesystem::path(MINECRAFT_TEST_SOURCE_DIR) / "third_party" / "iris" / "common" / "src" /
          "main" / "java" / "net" / "irisshaders" / "iris" / "pipeline" / "programs" / "ShaderKey.java");
 ASSERT_FALSE(source.empty()) << "the Iris reference checkout is the source of this table";
 for(const std::string_view key : {"SKY_BASIC", "SKY_BASIC_COLOR", "SKY_TEXTURED", "SKY_TEXTURED_COLOR",
                                   "TEXTURED", "TEXTURED_COLOR", "CRUMBLING", "SHADOW_BASIC", "SHADOW_TEX",
                                   "SHADOW_TERRAIN_CUTOUT"}) {
  EXPECT_EQ(irisFogMode(source, key), "FogMode.OFF") << key;
 }
 for(const std::string_view key : {"TERRAIN_SOLID", "TERRAIN_CUTOUT", "TERRAIN_TRANSLUCENT", "CLOUDS",
                                   "PARTICLES", "WEATHER", "LIGHTNING", "LINES", "TEXT", "BLOCK_ENTITY",
                                   "BE_TRANSLUCENT", "ENTITIES_TRANSLUCENT", "HAND_CUTOUT", "BASIC"}) {
  EXPECT_NE(irisFogMode(source, key), "FogMode.OFF") << key;
 }
}
} // namespace net::minecraft::test
