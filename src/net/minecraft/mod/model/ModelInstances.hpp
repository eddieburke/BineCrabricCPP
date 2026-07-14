#pragma once
#include <string>
namespace net::minecraft::mod::model {
// Placement transform for a model instance. Matches minecraft.model.draw: the
// anchor (x, y, z) is where model-space (0.5, pivotY, 0.5) lands, and is also
// the rotation pivot. scale applies uniformly about that anchor.
struct ModelTransform {
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
  float yaw = 0.0f;
  float pitch = 0.0f;
  float roll = 0.0f;
  float scale = 1.0f;
  float pivotY = 0.0f;
};
// Registers a placed instance of a baked model and returns its instance id
// (>= 1), or 0 if the handle is unknown. The world-space AABB is derived from
// the model's baked bounds and the transform (scale included), so hitboxes
// track global/per-model scaling automatically.
int placeModelInstance(const std::string& modId, int handle, const ModelTransform& transform,
                       const std::string& tag);
bool updateModelInstance(int instanceId, const ModelTransform& transform);
bool removeModelInstance(int instanceId);
void clearModelInstances(const std::string& modId);
struct ModelRaycastHit {
  int instanceId = 0;
  std::string tag;
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
  double distance = 0.0;
};
// Tests all placed instances against the ray and reports the nearest hit within
// maxDistance. Returns false if nothing is hit.
bool raycastModelInstances(double ox, double oy, double oz, double dx, double dy, double dz, double maxDistance,
                           ModelRaycastHit& hit);
} // namespace net::minecraft::mod::model
