// The full model registration pipeline: load a JSON model asset, parse it,
// flatten its parent chain, bake it into textured quads, and cache the result
// by handle. Sections below follow that order.
#include "net/minecraft/mod/model/ModelRegistry.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <memory>
#include <mutex>
#include <set>
#include <unordered_map>
#include <utility>
#include <vector>
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include "net/minecraft/mod/model/BakedModel.hpp"
#include "net/minecraft/mod/model/JsonValue.hpp"
#include "net/minecraft/mod/runtime/ModHost.hpp"
#include "net/minecraft/mod/runtime/ModHostUtil.hpp"
namespace net::minecraft::mod::model {
namespace {
// --- Parsed JSON model representation -------------------------------------
struct JsonModelFaceSpec {
  bool present = false;
  bool hasUv = false;
  double uv[4] = {0.0, 0.0, 16.0, 16.0};
  std::string texture;
  int rotation = 0;
  int tintIndex = -1;
};
struct JsonModelRotationSpec {
  // 'x'/'y'/'z' for vanilla single-axis rotation; 0 for Blockbench free x/y/z
  char axis = 0;
  double angle = 0.0;
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
  double origin[3] = {8.0, 8.0, 8.0};
  bool rescale = false;
};
struct JsonModelElement {
  double from[3] = {0.0, 0.0, 0.0};
  double to[3] = {16.0, 16.0, 16.0};
  bool hasRotation = false;
  JsonModelRotationSpec rotation;
  bool shade = true;
  JsonModelFaceSpec faces[kModelFaceCount];
};
struct JsonModel {
  std::string parent;
  std::vector<std::pair<std::string, std::string>> textures;
  std::vector<JsonModelElement> elements;
  bool hasElements = false;
};
const std::string* findTexture(const JsonModel& model, const std::string& key) noexcept {
  for(const auto& [name, value] : model.textures) {
    if(name == key) {
      return &value;
    }
  }
  return nullptr;
}
// --- JSON parsing ----------------------------------------------------------
constexpr const char* kFaceNames[kModelFaceCount] = {"down", "up", "north", "south", "west", "east"};
bool readVec(const JsonValue& value, double* out, int count) {
  if(!value.isArray() || value.size() < static_cast<std::size_t>(count)) {
    return false;
  }
  for(int i = 0; i < count; ++i) {
    if(!value.at(static_cast<std::size_t>(i)).isNumber()) {
      return false;
    }
    out[i] = value.at(static_cast<std::size_t>(i)).asNumber();
  }
  return true;
}
bool parseFace(const JsonValue& value, JsonModelFaceSpec& out, std::string& error) {
  if(!value.isObject()) {
    error = "face must be an object";
    return false;
  }
  out.present = true;
  const JsonValue& uv = value["uv"];
  if(!uv.isNull()) {
    if(!readVec(uv, out.uv, 4)) {
      error = "face uv must be an array of 4 numbers";
      return false;
    }
    out.hasUv = true;
  }
  out.texture = value["texture"].asString();
  const int rotation = static_cast<int>(value["rotation"].asNumber(0.0));
  if(rotation % 90 != 0) {
    error = "face rotation must be a multiple of 90";
    return false;
  }
  out.rotation = ((rotation % 360) + 360) % 360;
  out.tintIndex = static_cast<int>(value["tintindex"].asNumber(-1.0));
  return true;
}
bool parseRotation(const JsonValue& value, JsonModelRotationSpec& out, std::string& error) {
  if(!value.isObject()) {
    error = "element rotation must be an object";
    return false;
  }
  readVec(value["origin"], out.origin, 3);
  const std::string axis = value["axis"].asString();
  if(!axis.empty()) {
    if(axis != "x" && axis != "y" && axis != "z") {
      error = "element rotation axis must be x, y, or z";
      return false;
    }
    out.axis = axis[0];
    out.angle = value["angle"].asNumber(0.0);
  } else {
    out.axis = 0;
    out.x = value["x"].asNumber(0.0);
    out.y = value["y"].asNumber(0.0);
    out.z = value["z"].asNumber(0.0);
  }
  out.rescale = value["rescale"].asBool(false);
  return true;
}
bool parseElement(const JsonValue& value, JsonModelElement& out, std::string& error) {
  if(!value.isObject()) {
    error = "element must be an object";
    return false;
  }
  if(!readVec(value["from"], out.from, 3) || !readVec(value["to"], out.to, 3)) {
    error = "element requires from/to arrays of 3 numbers";
    return false;
  }
  const JsonValue& rotation = value["rotation"];
  if(!rotation.isNull()) {
    if(!parseRotation(rotation, out.rotation, error)) {
      return false;
    }
    out.hasRotation = true;
  }
  out.shade = value["shade"].asBool(true);
  const JsonValue& faces = value["faces"];
  if(!faces.isObject()) {
    error = "element requires a faces object";
    return false;
  }
  for(const auto& [name, face] : faces.members()) {
    const auto* it = std::find(std::begin(kFaceNames), std::end(kFaceNames), name);
    if(it == std::end(kFaceNames)) {
      error = "unknown face name: " + name;
      return false;
    }
    if(!parseFace(face, out.faces[it - std::begin(kFaceNames)], error)) {
      return false;
    }
  }
  return true;
}
bool parseJsonModel(const JsonValue& root, JsonModel& out, std::string& error) {
  if(!root.isObject()) {
    error = "model root must be an object";
    return false;
  }
  out.parent = root["parent"].asString();
  const JsonValue& textures = root["textures"];
  if(textures.isObject()) {
    for(const auto& [key, value] : textures.members()) {
      if(value.isString()) {
        out.textures.emplace_back(key, value.asString());
      }
    }
  }
  const JsonValue& elements = root["elements"];
  if(elements.isArray()) {
    out.hasElements = true;
    out.elements.resize(elements.size());
    for(std::size_t i = 0; i < elements.size(); ++i) {
      if(!parseElement(elements.at(i), out.elements[i], error)) {
        return false;
      }
    }
  }
  return true;
}
// Vanilla flattening: child elements win outright; parent textures fill only
// keys the child leaves unbound.
void mergeParentModel(JsonModel& child, const JsonModel& parent) {
  if(!child.hasElements && parent.hasElements) {
    child.elements = parent.elements;
    child.hasElements = true;
  }
  for(const auto& [key, value] : parent.textures) {
    if(findTexture(child, key) == nullptr) {
      child.textures.emplace_back(key, value);
    }
  }
}
// --- Baking ----------------------------------------------------------------
constexpr double kPi = 3.14159265358979323846;
// Corner selectors per face: which of from(0)/to(1) supplies each axis, in
// vanilla EnumFaceDirection vertex order. Vertex i pairs with UV corner
// (i + rotation/90) % 4 of [(u1,v1),(u1,v2),(u2,v2),(u2,v1)].
constexpr int kFaceCorners[kModelFaceCount][4][3] = {
    {{0, 0, 1}, {0, 0, 0}, {1, 0, 0}, {1, 0, 1}}, // down
    {{0, 1, 0}, {0, 1, 1}, {1, 1, 1}, {1, 1, 0}}, // up
    {{1, 1, 0}, {1, 0, 0}, {0, 0, 0}, {0, 1, 0}}, // north
    {{0, 1, 1}, {0, 0, 1}, {1, 0, 1}, {1, 1, 1}}, // south
    {{0, 1, 0}, {0, 0, 0}, {0, 0, 1}, {0, 1, 1}}, // west
    {{1, 1, 1}, {1, 0, 1}, {1, 0, 0}, {1, 1, 0}}, // east
};
constexpr float kFaceShade[kModelFaceCount] = {0.5f, 1.0f, 0.8f, 0.8f, 0.6f, 0.6f};
// Vanilla default UVs derived from the element bounds when a face omits uv.
void autoUv(ModelFace face, const double* from, const double* to, double* uv) {
  switch(face) {
  case ModelFace::Down:
    uv[0] = from[0];
    uv[1] = 16.0 - to[2];
    uv[2] = to[0];
    uv[3] = 16.0 - from[2];
    break;
  case ModelFace::Up:
    uv[0] = from[0];
    uv[1] = from[2];
    uv[2] = to[0];
    uv[3] = to[2];
    break;
  case ModelFace::North:
    uv[0] = 16.0 - to[0];
    uv[1] = 16.0 - to[1];
    uv[2] = 16.0 - from[0];
    uv[3] = 16.0 - from[1];
    break;
  case ModelFace::South:
    uv[0] = from[0];
    uv[1] = 16.0 - to[1];
    uv[2] = to[0];
    uv[3] = 16.0 - from[1];
    break;
  case ModelFace::West:
    uv[0] = from[2];
    uv[1] = 16.0 - to[1];
    uv[2] = to[2];
    uv[3] = 16.0 - from[1];
    break;
  case ModelFace::East:
    uv[0] = 16.0 - to[2];
    uv[1] = 16.0 - to[1];
    uv[2] = 16.0 - from[2];
    uv[3] = 16.0 - from[1];
    break;
  }
}
void rotatePoint(double* point, const double* origin, char axis, double angleDeg) {
  if(angleDeg == 0.0) {
    return;
  }
  const double a = angleDeg * kPi / 180.0;
  const double c = std::cos(a);
  const double s = std::sin(a);
  const double x = point[0] - origin[0];
  const double y = point[1] - origin[1];
  const double z = point[2] - origin[2];
  double rx = x;
  double ry = y;
  double rz = z;
  switch(axis) {
  case 'x':
    ry = y * c - z * s;
    rz = y * s + z * c;
    break;
  case 'y':
    rx = x * c + z * s;
    rz = -x * s + z * c;
    break;
  case 'z':
    rx = x * c - y * s;
    ry = x * s + y * c;
    break;
  default:
    break;
  }
  point[0] = rx + origin[0];
  point[1] = ry + origin[1];
  point[2] = rz + origin[2];
}
void rescalePoint(double* point, const double* origin, char axis, double angleDeg) {
  const double c = std::cos(angleDeg * kPi / 180.0);
  if(c == 0.0) {
    return;
  }
  const double scale = 1.0 / std::abs(c);
  if(axis != 'x') {
    point[0] = origin[0] + (point[0] - origin[0]) * scale;
  }
  if(axis != 'y') {
    point[1] = origin[1] + (point[1] - origin[1]) * scale;
  }
  if(axis != 'z') {
    point[2] = origin[2] + (point[2] - origin[2]) * scale;
  }
}
// Resolves a face texture reference ("#key" chains, "ns:name", relative names)
// to a runtime texture path.
bool resolveTexture(const JsonModel& model, const std::string& reference, const std::string& basePath,
                    std::string& out) {
  std::string key = reference;
  std::set<std::string> seen;
  while(!key.empty() && key[0] == '#') {
    key.erase(0, 1);
    if(!seen.insert(key).second) {
      return false;
    }
    const std::string* value = findTexture(model, key);
    if(value == nullptr) {
      return false;
    }
    key = *value;
  }
  if(key.empty() || key == "missing") {
    return false;
  }
  const std::size_t colon = key.find(':');
  if(colon != std::string::npos) {
    key = "assets/" + key.substr(0, colon) + "/textures/" + key.substr(colon + 1);
  } else if(key.compare(0, 7, "assets/") != 0 && key.compare(0, 5, "mods/") != 0) {
    key = basePath + key;
  }
  if(key.size() < 4 || key.compare(key.size() - 4, 4, ".png") != 0) {
    key += ".png";
  }
  out = std::move(key);
  return true;
}
std::vector<BakedQuad>& batchFor(BakedModel& model, const std::string& texturePath) {
  for(BakedTextureBatch& batch : model.batches) {
    if(batch.texturePath == texturePath) {
      return batch.quads;
    }
  }
  BakedTextureBatch& batch = model.batches.emplace_back();
  batch.texturePath = texturePath;
  return batch.quads;
}
// Bakes a parent-flattened model into world-space quads (positions in block
// units, 0..1 for a full cube). basePath is the model file's directory and
// anchors relative texture paths.
bool bakeJsonModel(const JsonModel& model, const std::string& basePath, BakedModel& out, std::string& error) {
  out.batches.clear();
  for(const JsonModelElement& element : model.elements) {
    const bool zeroX = element.from[0] == element.to[0];
    const bool zeroY = element.from[1] == element.to[1];
    const bool zeroZ = element.from[2] == element.to[2];
    for(int faceIndex = 0; faceIndex < kModelFaceCount; ++faceIndex) {
      const JsonModelFaceSpec& face = element.faces[faceIndex];
      if(!face.present) {
        continue;
      }
      // A degenerate (zero-thickness) element box has opposing faces that bake
      // onto the exact same plane; emitting both makes them z-fight. Skip the
      // duplicate when its partner face is present (the partner covers it).
      const bool partnerPresent =
          (faceIndex == 1 && element.faces[0].present) ||
          (faceIndex == 3 && element.faces[2].present) ||
          (faceIndex == 5 && element.faces[4].present);
      if((faceIndex == 1 && zeroY && partnerPresent) ||
         (faceIndex == 3 && zeroZ && partnerPresent) ||
         (faceIndex == 5 && zeroX && partnerPresent)) {
        continue;
      }
      std::string texturePath;
      if(!resolveTexture(model, face.texture, basePath, texturePath)) {
        continue;
      }
      double uv[4];
      if(face.hasUv) {
        for(int i = 0; i < 4; ++i) {
          uv[i] = face.uv[i];
        }
      } else {
        autoUv(static_cast<ModelFace>(faceIndex), element.from, element.to, uv);
      }
      const double uvCorners[4][2] = {{uv[0], uv[1]}, {uv[0], uv[3]}, {uv[2], uv[3]}, {uv[2], uv[1]}};
      const int uvShift = face.rotation / 90;
      BakedQuad quad;
      quad.face = static_cast<ModelFace>(faceIndex);
      quad.shade = element.shade ? kFaceShade[faceIndex] : 1.0f;
      quad.tintIndex = face.tintIndex;
      for(int i = 0; i < 4; ++i) {
        double point[3];
        for(int axis = 0; axis < 3; ++axis) {
          point[axis] = kFaceCorners[faceIndex][i][axis] != 0 ? element.to[axis] : element.from[axis];
        }
        if(element.hasRotation) {
          const JsonModelRotationSpec& rotation = element.rotation;
          if(rotation.axis != 0) {
            rotatePoint(point, rotation.origin, rotation.axis, rotation.angle);
            if(rotation.rescale) {
              rescalePoint(point, rotation.origin, rotation.axis, rotation.angle);
            }
          } else {
            rotatePoint(point, rotation.origin, 'x', rotation.x);
            rotatePoint(point, rotation.origin, 'y', rotation.y);
            rotatePoint(point, rotation.origin, 'z', rotation.z);
          }
        }
        const double* corner = uvCorners[(i + uvShift) % 4];
        BakedVertex& vertex = quad.vertices[i];
        vertex.x = static_cast<float>(point[0] / 16.0);
        vertex.y = static_cast<float>(point[1] / 16.0);
        vertex.z = static_cast<float>(point[2] / 16.0);
        vertex.u = static_cast<float>(corner[0] / 16.0);
        vertex.v = static_cast<float>(corner[1] / 16.0);
      }
      batchFor(out, texturePath).push_back(quad);
    }
  }
  if(out.batches.empty()) {
    error = "model has no renderable faces";
    return false;
  }
  computeBakedBounds(out);
  return true;
}
// --- Handle cache ------------------------------------------------------------
struct ModelStore {
  std::mutex mutex;
  std::vector<std::unique_ptr<BakedModel>> models; // handle - 1 indexes this
  std::unordered_map<std::string, int> handlesByKey;
};
ModelStore& store() {
  static ModelStore instance;
  return instance;
}
std::string directoryOf(const std::string& path) {
  const std::size_t slash = path.find_last_of("/\\");
  return slash == std::string::npos ? std::string() : path.substr(0, slash + 1);
}
// Parent references resolve like textures: "ns:name" maps into that
// namespace's models folder, bare names are siblings of the child model.
std::string parentModelPath(const std::string& parent, const std::string& basePath) {
  std::string path = parent;
  const std::size_t colon = path.find(':');
  if(colon != std::string::npos) {
    path = "assets/" + path.substr(0, colon) + "/models/" + path.substr(colon + 1);
  } else if(path.compare(0, 7, "assets/") != 0 && path.compare(0, 5, "mods/") != 0) {
    path = basePath + path;
  }
  if(path.size() < 5 || path.compare(path.size() - 5, 5, ".json") != 0) {
    path += ".json";
  }
  return path;
}
bool loadModelFile(const std::string& modId, const std::string& path, JsonModel& out, std::string& error) {
  const std::filesystem::path file = runtime::host().assetPath(modId, path);
  if(file.empty() || !std::filesystem::is_regular_file(file)) {
    error = "model not found: " + path;
    return false;
  }
  const std::string text = runtime::readFileText(file);
  JsonValue root;
  if(!JsonValue::parse(text, root, error)) {
    error = path + ": " + error;
    return false;
  }
  if(!parseJsonModel(root, out, error)) {
    error = path + ": " + error;
    return false;
  }
  return true;
}
} // namespace
void computeBakedBounds(BakedModel& model) {
  BakedBounds bounds;
  for(const BakedTextureBatch& batch : model.batches) {
    for(const BakedQuad& quad : batch.quads) {
      for(const BakedVertex& vertex : quad.vertices) {
        const float point[3] = {vertex.x, vertex.y, vertex.z};
        if(bounds.empty) {
          for(int axis = 0; axis < 3; ++axis) {
            bounds.min[axis] = point[axis];
            bounds.max[axis] = point[axis];
          }
          bounds.empty = false;
        } else {
          for(int axis = 0; axis < 3; ++axis) {
            bounds.min[axis] = std::min(bounds.min[axis], point[axis]);
            bounds.max[axis] = std::max(bounds.max[axis], point[axis]);
          }
        }
      }
    }
  }
  model.bounds = bounds;
}
int bakedModelHandleForKey(const std::string& key) noexcept {
  ModelStore& models = store();
  const std::lock_guard<std::mutex> lock(models.mutex);
  const auto it = models.handlesByKey.find(key);
  return it != models.handlesByKey.end() ? it->second : 0;
}
int storeBakedModel(const std::string& key, std::unique_ptr<BakedModel> baked) {
  ModelStore& models = store();
  const std::lock_guard<std::mutex> lock(models.mutex);
  models.models.push_back(std::move(baked));
  const int handle = static_cast<int>(models.models.size());
  models.handlesByKey.emplace(key, handle);
  return handle;
}
int loadBakedModel(const std::string& modId, const std::string& path, std::string& error) {
  const std::string key = modId + "|" + path;
  if(const int cached = bakedModelHandleForKey(key)) {
    return cached;
  }
  JsonModel merged;
  if(!loadModelFile(modId, path, merged, error)) {
    return 0;
  }
  // Flatten the parent chain. Blockbench exports sometimes carry dangling or
  // self-referential parents; those are skipped rather than fatal.
  std::set<std::string> visited{path};
  std::string basePath = directoryOf(path);
  std::string parent = merged.parent;
  while(!parent.empty()) {
    const std::string parentPath = parentModelPath(parent, basePath);
    if(!visited.insert(parentPath).second) {
      break;
    }
    JsonModel parentModel;
    std::string parentError;
    if(!loadModelFile(modId, parentPath, parentModel, parentError)) {
      lua::runtimeLog(modId, "warn", "model " + path + " skipping parent " + parent + " (" + parentError + ")");
      break;
    }
    mergeParentModel(merged, parentModel);
    basePath = directoryOf(parentPath);
    parent = parentModel.parent;
  }
  auto baked = std::make_unique<BakedModel>();
  if(!bakeJsonModel(merged, directoryOf(path), *baked, error)) {
    error = path + ": " + error;
    return 0;
  }
  return storeBakedModel(key, std::move(baked));
}
const BakedModel* bakedModelForHandle(int handle) noexcept {
  ModelStore& models = store();
  const std::lock_guard<std::mutex> lock(models.mutex);
  if(handle < 1 || static_cast<std::size_t>(handle) > models.models.size()) {
    return nullptr;
  }
  return models.models[static_cast<std::size_t>(handle) - 1].get();
}
} // namespace net::minecraft::mod::model
