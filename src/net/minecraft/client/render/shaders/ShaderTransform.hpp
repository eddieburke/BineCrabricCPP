#pragma once

namespace net::minecraft::client::render {
// see shaders/CoreGlslTransformer.cpp, shaders/SourceProcessor.cpp (keys shaders/Compiler.cpp transformationSalt)
inline constexpr int kShaderTransformerVersion = 9;
enum class ShaderStage { Vertex, Fragment, Geometry, TessControl, TessEvaluation, Compute };

struct ShaderTransformContext {
 bool entityOverlay = false;
 bool entityId = false;
 bool hasGeometry = false;
 bool hasTessellation = false;
};
}
