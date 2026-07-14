#include "net/minecraft/mod/model/ModelInstances.hpp"
#include <algorithm>
#include <cmath>
#include <mutex>
#include <vector>
#include "net/minecraft/mod/model/BakedModel.hpp"
#include "net/minecraft/mod/model/ModelRegistry.hpp"
namespace net::minecraft::mod::model {
namespace {
constexpr double kPi = 3.14159265358979323846;
struct WorldBox {
  double min[3] = {0.0, 0.0, 0.0};
  double max[3] = {0.0, 0.0, 0.0};
  bool valid = false;
};
struct ModelInstance {
  int id = 0;
  int handle = 0;
  std::string modId;
  std::string tag;
  WorldBox box;
};
struct InstanceStore {
  std::mutex mutex;
  std::vector<ModelInstance> instances;
  int nextId = 1;
};
InstanceStore& store() {
  static InstanceStore instance;
  return instance;
}
// Applies the draw transform (yaw*pitch*roll, scale, pivot) to a model-space
// point and returns its world position.
void transformPoint(const ModelTransform& t, double px, double py, double pz, double* out) {
  double x = (px - 0.5) * t.scale;
  double y = (py - t.pivotY) * t.scale;
  double z = (pz - 0.5) * t.scale;
  if(t.roll != 0.0f) {
    const double a = t.roll * kPi / 180.0;
    const double c = std::cos(a);
    const double s = std::sin(a);
    const double nx = x * c - y * s;
    const double ny = x * s + y * c;
    x = nx;
    y = ny;
  }
  if(t.pitch != 0.0f) {
    const double a = t.pitch * kPi / 180.0;
    const double c = std::cos(a);
    const double s = std::sin(a);
    const double ny = y * c - z * s;
    const double nz = y * s + z * c;
    y = ny;
    z = nz;
  }
  if(t.yaw != 0.0f) {
    const double a = t.yaw * kPi / 180.0;
    const double c = std::cos(a);
    const double s = std::sin(a);
    const double nx = x * c + z * s;
    const double nz = -x * s + z * c;
    x = nx;
    z = nz;
  }
  out[0] = t.x + x;
  out[1] = t.y + y;
  out[2] = t.z + z;
}
WorldBox worldBoxFor(int handle, const ModelTransform& transform) {
  WorldBox box;
  const BakedModel* baked = bakedModelForHandle(handle);
  if(baked == nullptr || baked->bounds.empty) {
    return box;
  }
  const BakedBounds& b = baked->bounds;
  for(int corner = 0; corner < 8; ++corner) {
    const double px = (corner & 1) ? b.max[0] : b.min[0];
    const double py = (corner & 2) ? b.max[1] : b.min[1];
    const double pz = (corner & 4) ? b.max[2] : b.min[2];
    double world[3];
    transformPoint(transform, px, py, pz, world);
    if(!box.valid) {
      for(int axis = 0; axis < 3; ++axis) {
        box.min[axis] = world[axis];
        box.max[axis] = world[axis];
      }
      box.valid = true;
    } else {
      for(int axis = 0; axis < 3; ++axis) {
        box.min[axis] = std::min(box.min[axis], world[axis]);
        box.max[axis] = std::max(box.max[axis], world[axis]);
      }
    }
  }
  return box;
}
// Slab test; returns the entry distance along the (unit-length) ray, or a
// negative value when the box is missed.
double boxRayEntry(const WorldBox& box, const double* origin, const double* dir, double maxDistance) {
  double tMin = 0.0;
  double tMax = maxDistance;
  for(int axis = 0; axis < 3; ++axis) {
    if(std::abs(dir[axis]) < 1.0e-9) {
      if(origin[axis] < box.min[axis] || origin[axis] > box.max[axis]) {
        return -1.0;
      }
      continue;
    }
    double t1 = (box.min[axis] - origin[axis]) / dir[axis];
    double t2 = (box.max[axis] - origin[axis]) / dir[axis];
    if(t1 > t2) {
      std::swap(t1, t2);
    }
    tMin = std::max(tMin, t1);
    tMax = std::min(tMax, t2);
    if(tMin > tMax) {
      return -1.0;
    }
  }
  return tMin;
}
} // namespace
int placeModelInstance(const std::string& modId, int handle, const ModelTransform& transform,
                       const std::string& tag) {
  const WorldBox box = worldBoxFor(handle, transform);
  if(!box.valid) {
    return 0;
  }
  InstanceStore& instances = store();
  const std::lock_guard<std::mutex> lock(instances.mutex);
  ModelInstance instance;
  instance.id = instances.nextId++;
  instance.handle = handle;
  instance.modId = modId;
  instance.tag = tag;
  instance.box = box;
  instances.instances.push_back(std::move(instance));
  return instances.instances.back().id;
}
bool updateModelInstance(int instanceId, const ModelTransform& transform) {
  InstanceStore& instances = store();
  const std::lock_guard<std::mutex> lock(instances.mutex);
  for(ModelInstance& instance : instances.instances) {
    if(instance.id != instanceId) {
      continue;
    }
    const WorldBox box = worldBoxFor(instance.handle, transform);
    if(!box.valid) {
      return false;
    }
    instance.box = box;
    return true;
  }
  return false;
}
bool removeModelInstance(int instanceId) {
  InstanceStore& instances = store();
  const std::lock_guard<std::mutex> lock(instances.mutex);
  const auto it = std::find_if(instances.instances.begin(), instances.instances.end(),
                               [instanceId](const ModelInstance& i) { return i.id == instanceId; });
  if(it == instances.instances.end()) {
    return false;
  }
  instances.instances.erase(it);
  return true;
}
void clearModelInstances(const std::string& modId) {
  InstanceStore& instances = store();
  const std::lock_guard<std::mutex> lock(instances.mutex);
  instances.instances.erase(std::remove_if(instances.instances.begin(), instances.instances.end(),
                                           [&modId](const ModelInstance& i) { return i.modId == modId; }),
                            instances.instances.end());
}
bool raycastModelInstances(double ox, double oy, double oz, double dx, double dy, double dz, double maxDistance,
                           ModelRaycastHit& hit) {
  const double length = std::sqrt(dx * dx + dy * dy + dz * dz);
  if(length < 1.0e-9) {
    return false;
  }
  const double origin[3] = {ox, oy, oz};
  const double dir[3] = {dx / length, dy / length, dz / length};
  InstanceStore& instances = store();
  const std::lock_guard<std::mutex> lock(instances.mutex);
  double bestDistance = maxDistance;
  const ModelInstance* best = nullptr;
  for(const ModelInstance& instance : instances.instances) {
    const double entry = boxRayEntry(instance.box, origin, dir, maxDistance);
    if(entry < 0.0 || entry >= bestDistance) {
      continue;
    }
    bestDistance = entry;
    best = &instance;
  }
  if(best == nullptr) {
    return false;
  }
  hit.instanceId = best->id;
  hit.tag = best->tag;
  hit.distance = bestDistance;
  hit.x = origin[0] + dir[0] * bestDistance;
  hit.y = origin[1] + dir[1] * bestDistance;
  hit.z = origin[2] + dir[2] * bestDistance;
  return true;
}
} // namespace net::minecraft::mod::model
