#pragma once

namespace net::minecraft::client::render {
enum class ShaderStage { Vertex, Fragment, Geometry, TessControl, TessEvaluation, Compute };

struct ShaderTransformContext {
 bool entityOverlay = false;
 bool entityId = false;
 bool hasGeometry = false;
 bool hasTessellation = false;
};
}
