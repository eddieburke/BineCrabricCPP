#include "net/minecraft/client/render/shaders/ColorWheelMerge.hpp"
#include "net/minecraft/client/render/shaders/GlslSource.hpp"
#include "net/minecraft/client/render/shaders/GlslSnippets.hpp"
#include <array>
#include <string_view>

namespace net::minecraft::client::render {
namespace {

struct AttrDecl {
 std::string_view storage;
 std::string_view type;
 std::string_view name;
};

constexpr std::array kVertexAttrDecls = {
    AttrDecl{"in", "vec3", "vaPosition"},
    AttrDecl{"in", "vec2", "vaUV0"},
    AttrDecl{"in", "vec2", "vaUV2"},
    AttrDecl{"in", "vec4", "vaColor"},
    AttrDecl{"in", "vec3", "vaNormal"},
    AttrDecl{"uniform", "vec3", "chunkOffset"},
};

void ensureVertexAttrs(std::string& source) {
 std::string decls;
 for(const AttrDecl& d : kVertexAttrDecls) {
  if(!referencesToken(source, d.name)) continue;
  if(hasStorageDeclaration(source, d.storage, d.name)) continue;
  decls += std::string(d.storage) + " " + std::string(d.type) + " " + std::string(d.name) + ";\n";
 }
 if(!decls.empty()) source.insert(sourceDeclarationOffset(source), decls);
}

// https://djefrey.github.io/colorwheel/reference/miscellaneous/functions/
std::string fragmentBridge(bool translucent) {
 const std::string& discard = translucent
     ? GlslSnippets::get("colorwheel_fragment_discard_translucent")
     : GlslSnippets::get("colorwheel_fragment_discard_cutout");
 std::string bridge = GlslSnippets::get("colorwheel_fragment_bridge");
 replaceAllToken(bridge, "CLRWL_DISCARD_FUNCTIONS", discard);
 return bridge;
}

void insertOnce(std::string& source, std::string_view guard, const std::string& block) {
 if(source.find(guard) != std::string::npos) return;
 source.insert(sourceDeclarationOffset(source), block);
}

}

bool isColorWheelProgramName(std::string_view programName) {
 return programName.rfind("clrwl_", 0) == 0;
}

void appendColorWheelMacros(std::string& preamble) {
 // The prefix ends mid-line (right after COLORWHEEL_VERSION's space); editors
 // may append a trailing line ending, so strip it before the value is joined.
 std::string prefix = GlslSnippets::get("colorwheel_macros_prefix");
 while(!prefix.empty() && (prefix.back() == '\n' || prefix.back() == '\r')) prefix.pop_back();
 preamble += prefix;
 preamble += colorWheelVersionMacro();
 preamble += "\n";
 preamble += GlslSnippets::get("colorwheel_stage_defines");
}

std::string mergeColorWheelMaterial(const std::string& programName, ShaderStage stage,
                                    std::string source) {
 if(!isColorWheelProgramName(programName)) return source;
 switch(stage) {
  case ShaderStage::Vertex:
   // https://djefrey.github.io/colorwheel/reference/attributes/overview/
   insertOnce(source, "_CLRWL_MERGE_V", GlslSnippets::get("colorwheel_vertex_bridge"));
   if(source.find("flw_vertexPos = vec4(vaPosition") == std::string::npos)
    appendBeforeMainClose(source, GlslSnippets::get("colorwheel_vertex_main"));
   ensureVertexAttrs(source);
   return source;
  case ShaderStage::Fragment:
   insertOnce(source, "_CLRWL_MERGE_F",
              fragmentBridge(programName.find("translucent") != std::string::npos));
   return source;
  case ShaderStage::Geometry:
   if(referencesToken(source, "clrwl_setVertexOut"))
    insertOnce(source, "_CLRWL_MERGE_G", GlslSnippets::get("colorwheel_geometry_bridge"));
   return source;
  case ShaderStage::TessControl:
  case ShaderStage::TessEvaluation:
  case ShaderStage::Compute:
   return source;
 }
 return source;
}
}
