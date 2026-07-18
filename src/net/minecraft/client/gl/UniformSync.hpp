#pragma once
// UniformSync — CPU-side change tracking for the engine ubershader uniforms so
// bindAndUploadUniforms only re-uploads groups that actually changed since the
// previous draw. Pure CPU logic (no GL) so it is unit/bench testable.
#include <cstring>
#include "net/minecraft/client/gl/EnginePipeline.hpp"
#include "net/minecraft/client/gl/PipelineState.hpp"
namespace net::minecraft::client::gl {
struct UniformSnapshot {
  EngineLighting lighting{};
  float modelView[16]{};
  float projection[16]{};
  bool valid = false;
};
struct UniformDelta {
  bool pipeline = false;
  bool lighting = false;
  bool modelView = false;
  bool projection = false;
};
// Compares current state against the snapshot, updates the snapshot to match, and
// returns which uniform groups need re-upload. The pipeline group is driven by the
// PipelineState::dirty flag (maintained by every GlDraw shim write); lighting and
// matrices are compared by value since they have no dirty flag.
inline UniformDelta diffAndUpdate(UniformSnapshot& snap,
                                  const PipelineState& pipeline,
                                  const EngineLighting& lighting,
                                  const float* modelView,
                                  const float* projection) {
  UniformDelta delta;
  if(!snap.valid) {
    delta.pipeline = delta.lighting = delta.modelView = delta.projection = true;
  } else {
    delta.pipeline = pipeline.dirty;
    delta.lighting = std::memcmp(&snap.lighting, &lighting, sizeof(EngineLighting)) != 0;
    delta.modelView = std::memcmp(snap.modelView, modelView, sizeof(snap.modelView)) != 0;
    delta.projection = std::memcmp(snap.projection, projection, sizeof(snap.projection)) != 0;
  }
  if(delta.lighting) {
    std::memcpy(&snap.lighting, &lighting, sizeof(EngineLighting));
  }
  if(delta.modelView) {
    std::memcpy(snap.modelView, modelView, sizeof(snap.modelView));
  }
  if(delta.projection) {
    std::memcpy(snap.projection, projection, sizeof(snap.projection));
  }
  snap.valid = true;
  return delta;
}
} // namespace net::minecraft::client::gl
