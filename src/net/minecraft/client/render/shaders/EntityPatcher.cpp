#include "net/minecraft/client/render/shaders/EntityPatcher.hpp"
#include <algorithm>
#include <sstream>
#include <string_view>
#include "net/minecraft/client/render/shaders/GlslSource.hpp"
#include "net/minecraft/client/render/shaders/PreProcessor.hpp"

namespace net::minecraft::client::render {
namespace {
void eraseSimpleUniforms(std::string& source,
                         const std::vector<std::pair<std::string_view, std::string_view>>& uniforms) {
 std::istringstream input(source);
 std::string output;
 for(std::string line; std::getline(input, line);) {
  const std::string parsed = lineForDirectiveParse(line);
  std::istringstream tokens(parsed);
  std::string storage;
  std::string declaredType;
  std::string declaredName;
  std::string extra;
  tokens >> storage >> declaredType >> declaredName >> extra;
  const bool erase = storage == "uniform" && extra.empty() &&
                     std::any_of(uniforms.begin(), uniforms.end(), [&](const auto& uniform) {
                      return declaredType == uniform.first && declaredName == std::string(uniform.second) + ";";
                     });
  if(erase) {
   output += '\n';
  } else {
   output += line;
   output += '\n';
  }
 }
 source = std::move(output);
}

void appendDeclaration(std::string& declarations,
                       const std::string& source,
                       const CodeMask& mask,
                       std::string_view storage,
                       std::string_view name,
                       std::string_view declaration) {
 if(!hasStorageDeclaration(source, mask, storage, name)) declarations += declaration;
}
}

void patchEntityInputs(std::string& source, ShaderStage stage, const ShaderTransformContext& context) {
 if(!context.entityOverlay && !context.entityId) return;
 std::vector<std::pair<std::string_view, std::string_view>> erasedUniforms;
 if(context.entityOverlay) erasedUniforms.emplace_back("vec4", "entityColor");
 if(context.entityId) {
  erasedUniforms.emplace_back("int", "entityId");
  erasedUniforms.emplace_back("int", "blockEntityId");
  erasedUniforms.emplace_back("int", "currentRenderedItemId");
 }
 eraseSimpleUniforms(source, erasedUniforms);
 const CodeMask declarationsMask = codeMask(source);
 std::string declarations;
 std::string mainBody;
 if(context.entityOverlay) {
  if(stage == ShaderStage::Vertex) {
   appendDeclaration(declarations, source, declarationsMask, "uniform", "iris_overlay", "uniform sampler2D iris_overlay;\n");
   appendDeclaration(declarations, source, declarationsMask, "out", "entityColor", "out vec4 entityColor;\n");
   appendDeclaration(declarations, source, declarationsMask, "out", "iris_vertexColor", "out vec4 iris_vertexColor;\n");
   appendDeclaration(declarations, source, declarationsMask, "in", "iris_UV1", "in ivec2 iris_UV1;\n");
   mainBody += "vec4 iris_overlayColor = texelFetch(iris_overlay, iris_UV1, 0);\n";
   mainBody += "entityColor = vec4(iris_overlayColor.rgb, 1.0 - iris_overlayColor.a);\n";
   mainBody += "iris_vertexColor = vaColor;\n";
   mainBody += "entityColor.rgb *= float(entityColor.a != 0.0);\n";
  } else if(stage == ShaderStage::TessControl) {
   replaceAllToken(source, "entityColor", "entityColor[gl_InvocationID]");
   declarations += "in vec4 entityColor[];\npatch out vec4 entityColorTCS;\n";
   declarations += "in vec4 iris_vertexColor[];\nout vec4 iris_vertexColorTCS[];\n";
   mainBody += "entityColorTCS = entityColor[gl_InvocationID];\n";
   mainBody += "iris_vertexColorTCS[gl_InvocationID] = iris_vertexColor[gl_InvocationID];\n";
  } else if(stage == ShaderStage::TessEvaluation) {
   replaceAllToken(source, "entityColor", "entityColorTCS");
   declarations += "patch in vec4 entityColorTCS;\nout vec4 entityColorTES;\n";
   declarations += "in vec4 iris_vertexColorTCS[];\nout vec4 iris_vertexColorTES;\n";
   mainBody += "entityColorTES = entityColorTCS;\n";
   mainBody += "iris_vertexColorTES = iris_vertexColorTCS[0];\n";
  } else if(stage == ShaderStage::Geometry) {
   const std::string entityInput = context.hasTessellation ? "entityColorTES" : "entityColor";
   const std::string colorInput = context.hasTessellation ? "iris_vertexColorTES" : "iris_vertexColor";
   replaceAllToken(source, "entityColor", entityInput + "[0]");
   declarations += "in vec4 " + entityInput + "[];\nout vec4 entityColorGS;\n";
   declarations += "in vec4 " + colorInput + "[];\nout vec4 iris_vertexColorGS;\n";
   mainBody += "entityColorGS = " + entityInput + "[0];\n";
   mainBody += "iris_vertexColorGS = " + colorInput + "[0];\n";
  } else if(stage == ShaderStage::Fragment) {
   const std::string entityInput = context.hasGeometry ? "entityColorGS"
                                  : context.hasTessellation ? "entityColorTES"
                                                           : "entityColor";
   const std::string colorInput = context.hasGeometry ? "iris_vertexColorGS"
                                 : context.hasTessellation ? "iris_vertexColorTES"
                                                          : "iris_vertexColor";
   if(entityInput != "entityColor") replaceAllToken(source, "entityColor", entityInput);
   declarations += "in vec4 " + entityInput + ";\nin vec4 " + colorInput + ";\n";
   mainBody += "float iris_vertexColorAlpha = " + colorInput + ".a;\n";
  }
 }
 if(context.entityId) {
  std::string entityInfo;
  if(stage == ShaderStage::Vertex) {
   entityInfo = "iris_entityInfo";
   declarations += "in ivec3 iris_Entity;\nflat out ivec3 iris_entityInfo;\n";
   mainBody += "iris_entityInfo = iris_Entity;\n";
  } else if(stage == ShaderStage::TessControl) {
   entityInfo = "iris_entityInfo[gl_InvocationID]";
   declarations += "flat in ivec3 iris_entityInfo[];\nflat out ivec3 iris_entityInfoTCS[];\n";
   mainBody += "iris_entityInfoTCS[gl_InvocationID] = iris_entityInfo[gl_InvocationID];\n";
  } else if(stage == ShaderStage::TessEvaluation) {
   entityInfo = "iris_entityInfoTCS[0]";
   declarations += "flat in ivec3 iris_entityInfoTCS[];\nflat out ivec3 iris_entityInfoTES;\n";
   mainBody += "iris_entityInfoTES = iris_entityInfoTCS[0];\n";
  } else if(stage == ShaderStage::Geometry) {
   const std::string input = context.hasTessellation ? "iris_entityInfoTES" : "iris_entityInfo";
   entityInfo = input + "[0]";
   declarations += "flat in ivec3 " + input + "[];\nflat out ivec3 iris_entityInfoGS;\n";
   mainBody += "iris_entityInfoGS = " + input + "[0];\n";
  } else if(stage == ShaderStage::Fragment) {
   entityInfo = context.hasGeometry ? "iris_entityInfoGS"
                : context.hasTessellation ? "iris_entityInfoTES"
                                          : "iris_entityInfo";
   declarations += "flat in ivec3 " + entityInfo + ";\n";
  }
  if(!entityInfo.empty()) {
   replaceAllTokens(source, {{"entityId", entityInfo + ".x"},
                             {"blockEntityId", entityInfo + ".y"},
                             {"currentRenderedItemId", entityInfo + ".z"}});
  }
 }
 if(!declarations.empty()) source.insert(sourceDeclarationOffset(source), declarations);
 if(!mainBody.empty()) prependToMainBody(source, mainBody);
}
}
