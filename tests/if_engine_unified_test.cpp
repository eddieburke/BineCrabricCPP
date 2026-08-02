#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include <vector>
#include "net/minecraft/client/render/shaders/ConditionalState.hpp"
#include "net/minecraft/client/render/shaders/SourceProcessor.hpp"
#include "net/minecraft/client/render/shaderpack/Loader.hpp"
#include "net/minecraft/client/render/shaderpack/Pack.hpp"
namespace net::minecraft::client::render {
std::string preprocessProperties(const std::string& source,
                                 int mcVersion,
                                 const std::unordered_map<std::string, PackSourceOption>& options,
                                 const std::vector<PackProfile>& profiles);
namespace {
std::vector<std::string> contentLines(const std::string& output) {
 std::vector<std::string> lines;
 std::istringstream stream(output);
 for(std::string line; std::getline(stream, line);) {
  const std::size_t first = line.find_first_not_of(" \t\r\n");
  if(first == std::string::npos) continue;
  const std::string trimmed = line.substr(first);
  if(trimmed.empty() || trimmed.front() == '#') continue;
  lines.push_back(trimmed);
 }
 return lines;
}
std::vector<std::string> glslContent(const std::string& text) {
 return contentLines(normalizePackSource(text, ""));
}
std::vector<std::string> propertiesContent(const std::string& text) {
 return contentLines(preprocessProperties(text, 10703, {}, {}));
}
void expectIdentical(const std::string& text) {
 EXPECT_EQ(glslContent(text), propertiesContent(text)) << text;
}
TEST(IfEngineUnified, DefineIfdefIfndefElifElseBattery) {
 expectIdentical("#define FOO\n"
                 "#define BAR 2\n"
                 "#undef BAZ\n"
                 "#if defined(FOO)\n"
                 "foo_defined\n"
                 "#endif\n"
                 "#ifdef FOO\n"
                 "ifdef_foo\n"
                 "#endif\n"
                 "#ifndef MISSING\n"
                 "ifndef_missing\n"
                 "#endif\n"
                 "#if BAR == 2\n"
                 "bar_is_two\n"
                 "#endif\n"
                 "#if defined(BAZ)\n"
                 "baz_defined\n"
                 "#else\n"
                 "baz_not_defined\n"
                 "#endif\n");
 EXPECT_EQ(glslContent("#define FOO\n#if defined(FOO)\nfoo_defined\n#endif\n"),
           (std::vector<std::string>{"foo_defined"}));
 EXPECT_EQ(propertiesContent("#if 0\nhidden\n#else\nshown\n#endif\n"),
           (std::vector<std::string>{"shown"}));
}
TEST(IfEngineUnified, ElifChainTakesFirstTrueBranch) {
 expectIdentical("#define X\n"
                 "#if defined(NOPE)\n"
                 "nope\n"
                 "#elif defined(X)\n"
                 "x_defined\n"
                 "#else\n"
                 "else_branch\n"
                 "#endif\n");
 EXPECT_EQ(propertiesContent("#if 0\na\n#elif 1\nb\n#elif 1\nc\n#endif\n"),
           (std::vector<std::string>{"b"}));
 EXPECT_EQ(glslContent("#if 0\na\n#elif 0\nb\n#else\nc\n#endif\n"),
           (std::vector<std::string>{"c"}));
}
TEST(IfEngineUnified, NestedConditionalsMatch) {
 expectIdentical("#define A\n"
                 "#if defined(A)\n"
                 "outer\n"
                 "#if 1\n"
                 "inner_taken\n"
                 "#else\n"
                 "inner_else\n"
                 "#endif\n"
                 "#elif 1\n"
                 "elif_branch\n"
                 "#else\n"
                 "else_branch\n"
                 "#endif\n");
 expectIdentical("#if 1\n"
                 "#if 0\n"
                 "nested_dead\n"
                 "#elif 1\n"
                 "nested_elif_alive\n"
                 "#else\n"
                 "nested_else\n"
                 "#endif\n"
                 "#endif\n");
 EXPECT_EQ(glslContent("#if 0\n#if 1\nx\n#else\ny\n#endif\n#endif\n"), (std::vector<std::string>{}));
 EXPECT_EQ(propertiesContent("#if 0\n#if 1\nx\n#else\ny\n#endif\n#endif\n"), (std::vector<std::string>{}));
}
TEST(IfEngineUnified, DeadRegionsExcluded) {
 expectIdentical("#if 0\n"
                 "dead_zero\n"
                 "#elif 0\n"
                 "dead_elif\n"
                 "#else\n"
                 "else_alive\n"
                 "#endif\n"
                 "#if 1\n"
                 "alive_one\n"
                 "#endif\n");
 EXPECT_EQ(propertiesContent("#if 0\ndead\n#endif\n"), (std::vector<std::string>{}));
}
TEST(IfEngineUnified, DefineAndUndefOnlyTakeEffectWhenActive) {
 expectIdentical("#if 0\n"
                 "#define DEAD\n"
                 "#endif\n"
                 "#ifdef DEAD\n"
                 "dead_should_not_exist\n"
                 "#endif\n");
 expectIdentical("#if 0\n"
                 "#elif 1\n"
                 "#define ACTIVE_ELIF\n"
                 "#endif\n"
                 "#ifdef ACTIVE_ELIF\n"
                 "elif_define_visible\n"
                 "#endif\n");
 expectIdentical("#define TEMP\n"
                 "#undef TEMP\n"
                 "#if defined(TEMP)\n"
                 "temp_should_not_exist\n"
                 "#endif\n");
 EXPECT_TRUE(glslContent("#define TEMP\n#undef TEMP\n#if defined(TEMP)\ntemp\n#endif\n").empty());
}
TEST(ConditionalState, GlslFlavorKeepsInactiveParentInactive) {
 ConditionalState s(ConditionalState::Flavor::Glsl);
 s.push(false);
 s.push(true);
 s.else_();
 EXPECT_FALSE(s.active());
 s.endif();
 s.endif();
 EXPECT_TRUE(s.active());
}
TEST(ConditionalState, PropertiesFlavorMasquesInactiveParentThroughAllOf) {
 ConditionalState s(ConditionalState::Flavor::Properties);
 s.push(false);
 s.push(true);
 s.else_();
 EXPECT_FALSE(s.active());
 s.endif();
 s.endif();
 EXPECT_TRUE(s.active());
}
TEST(ConditionalState, GlslFlavorIgnoresUnmatchedTopLevelElse) {
 ConditionalState s(ConditionalState::Flavor::Glsl);
 s.else_();
 EXPECT_TRUE(s.active());
 s.endif();
 EXPECT_TRUE(s.active());
}
TEST(ConditionalState, PropertiesFlavorTopLevelElseDisablesRestOfFile) {
 ConditionalState s(ConditionalState::Flavor::Properties);
 s.else_();
 EXPECT_FALSE(s.active());
 s.endif();
 EXPECT_FALSE(s.active());
}
TEST(ConditionalState, PropertiesFlavorTopLevelElifDisablesRestOfFile) {
 ConditionalState s(ConditionalState::Flavor::Properties);
 s.elif(true);
 EXPECT_FALSE(s.active());
}
TEST(ConditionalState, PropertiesSentinelIsNeverPopped) {
 ConditionalState s(ConditionalState::Flavor::Properties);
 s.push(true);
 s.endif();
 EXPECT_TRUE(s.active());
 s.push(false);
 s.endif();
 EXPECT_TRUE(s.active());
}
TEST(IfEngineUnified, PreservesTopLevelElseDivergenceBetweenEngines) {
 // Well-formed input produces identical results (the battery above); malformed
 // top-level #else is the one construct the two engines historically disagreed
 // on: the GLSL engine ignores it, the .properties engine disables the rest of
 // the file. The unified machine must keep each engine's current output.
 const std::string text = "#else\nline_after_top_else\n";
 EXPECT_EQ(glslContent(text), (std::vector<std::string>{"line_after_top_else"}));
 EXPECT_TRUE(propertiesContent(text).empty());
}
} // namespace
} // namespace net::minecraft::client::render
