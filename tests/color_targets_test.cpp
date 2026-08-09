#include <gtest/gtest.h>
#include <algorithm>
#include <array>
#include <cctype>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>
#include "net/minecraft/client/gl/GlConstants.hpp"
#include "net/minecraft/client/render/GlState.hpp"
#include "net/minecraft/client/render/shaderpack/Loader.hpp"
#include "net/minecraft/client/render/targets/RenderTargets.hpp"
namespace net::minecraft::client::render {
namespace {
std::string lower(std::string value) {
 std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
  return static_cast<char>(std::tolower(ch));
 });
 return value;
}
bool load(const std::unordered_map<std::string, std::string>& sources,
          PackDefinition& pack,
          std::unordered_map<std::string, PackSourceOption>& options,
          std::string& error) {
 std::vector<std::string> paths;
 for(const auto& [path, ignored] : sources) paths.push_back(path);
 return PackLoader::load(paths, [&sources](std::string_view path) {
                          const auto found = sources.find(std::string(path));
                          return found == sources.end() ? std::string{} : found->second; },
                          pack, options, error);
}
// All 58 names of InternalTextureFormat.java (the RGBA alias resolves to GL_RGBA8).
// https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/gl/texture/InternalTextureFormat.java
constexpr std::array<const char*, 58> kIrisFormatNames = {
    "RGBA",     "R8",         "RG8",        "RGB8",       "RGBA8",
    "R8_SNORM", "RG8_SNORM",  "RGB8_SNORM", "RGBA8_SNORM",
    "R16",      "RG16",       "RGB16",      "RGBA16",
    "R16_SNORM", "RG16_SNORM", "RGB16_SNORM", "RGBA16_SNORM",
    "R16F",     "RG16F",      "RGB16F",     "RGBA16F",
    "R32F",     "RG32F",      "RGB32F",     "RGBA32F",
    "R8I",      "RG8I",       "RGB8I",      "RGBA8I",
    "R8UI",     "RG8UI",      "RGB8UI",     "RGBA8UI",
    "R16I",     "RG16I",      "RGB16I",     "RGBA16I",
    "R16UI",    "RG16UI",     "RGB16UI",    "RGBA16UI",
    "R32I",     "RG32I",      "RGB32I",     "RGBA32I",
    "R32UI",    "RG32UI",     "RGB32UI",    "RGBA32UI",
    "RGBA2",    "RGBA4",      "R3_G3_B2",   "RGB5_A1",
    "RGB565",   "RGB10_A2",   "RGB10_A2UI", "R11F_G11F_B10F", "RGB9_E5"};
} // namespace
TEST(ColorTargetsTest, ParsesEveryIrisInternalTextureFormatName) {
 for(const char* name : kIrisFormatNames) {
  EXPECT_NO_FATAL_FAILURE(parseFormat(name)) << name;
  // GL_RGBA8 is the canonical mapping for both "RGBA" and "RGBA8".
  EXPECT_EQ(parseFormat(name) == ColorFormat::Rgba8, name == std::string("RGBA") ||
                                                                 name == std::string("RGBA8"))
      << name;
 }
}
TEST(ColorTargetsTest, IrisFormatNamesMapToFiftySevenDistinctFormats) {
 std::set<ColorFormat> formats;
 for(const char* name : kIrisFormatNames) {
  formats.insert(parseFormat(name));
 }
 EXPECT_EQ(formats.size(), 57u);
}
TEST(ColorTargetsTest, RoundTripsFormatNames) {
 for(const char* name : kIrisFormatNames) {
  const std::string expected = name == std::string("RGBA") ? "rgba8" : lower(name);
  EXPECT_EQ(std::string(colorFormatName(parseFormat(name))), expected) << name;
 }
}
TEST(ColorTargetsTest, ClassifiesIntegerFormats) {
 EXPECT_EQ(client::gl::pixel::RgbaInteger, 0x8D99);
 constexpr std::array<const char*, 25> integerNames = {
     "R8UI",   "R16UI",  "R32UI",  "RG8UI",   "RG16UI",  "RG32UI",
     "RGB8UI", "RGB16UI", "RGB32UI", "RGBA8UI", "RGBA16UI", "RGBA32UI",
     "R8I",    "R16I",   "R32I",   "RG8I",    "RG16I",   "RG32I",
     "RGB8I",  "RGB16I", "RGB32I", "RGBA8I",  "RGBA16I", "RGBA32I",
     "RGB10_A2UI"};
 for(const char* name : integerNames) {
  const ColorFormat format = parseFormat(name);
  EXPECT_TRUE(isIntegerColorFormat(format)) << name;
  EXPECT_EQ(isSignedIntegerColorFormat(format),
            std::string(name).find('I') != std::string::npos && std::string(name).find("UI") == std::string::npos)
      << name;
 }
}
TEST(ColorTargetsTest, ClassifiesNormalizedAndPackedFormatsAsFloat) {
 for(const char* name : {"RGBA", "R8", "RGBA8", "R8_SNORM", "RGBA16_SNORM", "RGBA2", "RGBA4",
                         "R3_G3_B2", "RGB5_A1", "RGB565", "RGB10_A2", "R11F_G11F_B10F", "RGB9_E5"}) {
  const ColorFormat format = parseFormat(name);
  EXPECT_FALSE(isIntegerColorFormat(format)) << name;
  EXPECT_FALSE(isSignedIntegerColorFormat(format)) << name;
 }
}
TEST(ColorTargetsTest, LoaderParsesTargetFormatMipmapAndClearColorDirectives) {
 PackDefinition pack;
 std::unordered_map<std::string, PackSourceOption> options;
 std::string error;
 EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                   {"shaders/gbuffers_basic.fsh", "void main(){}"},
                   {"shaders/composite.vsh", "void main(){}"},
                   {"shaders/composite.fsh",
                    "/* RENDERTARGETS: 2 */\n"
                    "const int colortex2Format = R16_SNORM;\n"
                    "const int colortex3Format = RGBA;\n"
                    "const bool colortex2MipmapEnabled = true;\n"
                    "const vec4 colortex2ClearColor = vec4(0.25, 0.5, 0.75, 1.0);\n"
                    "void main(){}"},
                   {"shaders/shaders.properties", ""}},
                  pack,
                  options,
                  error));
 EXPECT_EQ(pack.targets["colortex2"].format, "R16_SNORM");
 EXPECT_EQ(pack.targets["colortex3"].format, "RGBA8");
 EXPECT_EQ(parseFormat(pack.targets["colortex2"].format), ColorFormat::R16Snorm);
 EXPECT_TRUE(pack.targets["colortex2"].customClearColor);
 EXPECT_FLOAT_EQ(pack.targets["colortex2"].clearColor[0], 0.25f);
 EXPECT_FLOAT_EQ(pack.targets["colortex2"].clearColor[1], 0.5f);
 EXPECT_FLOAT_EQ(pack.targets["colortex2"].clearColor[2], 0.75f);
 EXPECT_FLOAT_EQ(pack.targets["colortex2"].clearColor[3], 1.0f);
 const auto composite = std::find_if(pack.passes.begin(), pack.passes.end(), [](const PackPass& pass) {
  return pass.name == "composite";
 });
 ASSERT_NE(composite, pack.passes.end());
 ASSERT_EQ(composite->outputs.size(), 1u);
 EXPECT_EQ(composite->outputs.front(), "colortex2");
 EXPECT_EQ(composite->mipmapBuffers, std::vector<std::string>{"colortex2"});
}
TEST(ColorTargetsTest, LoaderParsesPreAndPassFlipDirectives) {
  PackDefinition pack;
  std::unordered_map<std::string, PackSourceOption> options;
  std::string error;
  EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                    {"shaders/gbuffers_basic.fsh", "void main(){}"},
                    {"shaders/shaders.properties",
                     "flip.deferred_pre.colortex0=true\n"
                     "flip.composite_pre.colortex2=false\n"
                     "flip.composite3.colortex4=true\n"
                     "flip.composite3.colortex5=false\n"}},
                  pack,
                  options,
                  error));
  EXPECT_EQ(pack.flips.at("deferred_pre.colortex0"), true);
  EXPECT_EQ(pack.flips.at("composite_pre.colortex2"), false);
  EXPECT_EQ(pack.flips.at("composite3.colortex4"), true);
  EXPECT_EQ(pack.flips.at("composite3.colortex5"), false);
}
TEST(ColorTargetsTest, ExplicitTrueFlipAppliesToBuffersThePassDoesNotWrite) {
  // Java: per-pass flips are the drawBuffers loop (explicit FALSE blocks the default
  // flip) plus an explicitFlips sweep that flips every TRUE buffer, written or not
  // (CompositeRenderer.java:165-187). Pre-flip keys must never match a pass name.
  PackDefinition pack;
  std::unordered_map<std::string, PackSourceOption> options;
  std::string error;
  EXPECT_TRUE(load({{"shaders/gbuffers_basic.vsh", "void main(){}"},
                    {"shaders/gbuffers_basic.fsh", "void main(){}"},
                    {"shaders/shaders.properties",
                     "flip.composite3.colortex4=true\n"
                     "flip.composite3.colortex5=false\n"
                     "flip.composite3.colortex6=true\n"
                     "flip.composite_pre.colortex6=true\n"
                     "flip.composite4.colortex7=true\n"}},
                  pack,
                  options,
                  error));
  // colortex5 is explicit FALSE: blocks the default flip of a written buffer.
  EXPECT_TRUE(ColorTargets::flipExplicitlyBlocked(pack, "composite3", "colortex5"));
  // Explicit TRUE does not block the default flip.
  EXPECT_FALSE(ColorTargets::flipExplicitlyBlocked(pack, "composite3", "colortex4"));
  // No directive -> default flip (not blocked).
  EXPECT_FALSE(ColorTargets::flipExplicitlyBlocked(pack, "composite3", "colortex7"));
  // The explicit TRUE sweep covers written (colortex4) and unwritten (colortex6)
  // buffers alike; FALSE entries and other passes are excluded.
  const std::vector<std::string> expected = {"colortex4", "colortex6"};
  EXPECT_EQ(ColorTargets::explicitTrueFlips(pack, "composite3"), expected);
  // "composite_pre" is a different pass: its keys never leak into "composite".
  EXPECT_TRUE(ColorTargets::explicitTrueFlips(pack, "composite").empty());
  EXPECT_EQ(ColorTargets::explicitTrueFlips(pack, "composite4"), std::vector<std::string>({"colortex7"}));
  // A pass with no directives at all sweeps nothing.
  EXPECT_TRUE(ColorTargets::explicitTrueFlips(pack, "composite0").empty());
}
TEST(ColorTargetsTest, RenderTargetSamplersStartAtColortex4ForNonFullscreenPasses) {
  // Java: startIndex = isFullscreenPass ? 0 : 4; colortex0-3 are only sampleable
  // from fullscreen passes (IrisSamplers.java:53-55).
  EXPECT_EQ(ColorTargets::renderTargetSamplerStartIndex(true), 0);
  EXPECT_EQ(ColorTargets::renderTargetSamplerStartIndex(false), 4);
}
} // namespace net::minecraft::client::render
