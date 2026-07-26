// The full model registration pipeline: load a JSON model asset, parse it,
// flatten its parent chain, bake it into textured quads, and cache the result
// by handle. Sections below follow that order.
#include "net/minecraft/mod/model/ModModels.hpp"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>
#include <set>
#include <unordered_map>
#include <utility>
#include <vector>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/render/RenderSystem.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/block/BlockRenderManager.hpp"
#include "net/minecraft/client/render/item/ItemModelRenderer.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/mod/lua/LuaBlockRegistry.hpp"
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include "net/minecraft/mod/lua/LuaItemRegistry.hpp"
#include "net/minecraft/mod/model/JsonValue.hpp"
#include "net/minecraft/mod/runtime/LuaEventGlue.hpp"
#include "net/minecraft/mod/runtime/ModHost.hpp"
#include "net/minecraft/registry/TextureRegistry.hpp"
#include "net/minecraft/util/math/CoordinateHash.hpp"
#include "net/minecraft/world/BlockView.hpp"
#ifdef MINECRAFT_NATIVE_EXPORTS
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/render/FrameRenderCamera.hpp"
#include "net/minecraft/client/render/RenderType.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
#include "net/minecraft/mod/runtime/ModRenderScope.hpp"
#endif
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
 int cullFace = -1;
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
 std::string basePath;
 // Blockbench "texture_size" of the model file this element came from; face uv
 // values are expressed in this space, not in the texture's pixel size.
 double textureWidth = 16.0;
 double textureHeight = 16.0;
 bool hasRotation = false;
 JsonModelRotationSpec rotation;
 bool shade = true;
 JsonModelFaceSpec faces[kModelFaceCount];
};
struct JsonModelTexture {
 std::string name;
 std::string value;
 std::string basePath;
};
struct JsonModel {
 std::string parent;
 std::vector<JsonModelTexture> textures;
 std::vector<JsonModelElement> elements;
 bool hasElements = false;
 double textureSize[2] = {16.0, 16.0};
 bool hasTextureSize = false;
};
const JsonModelTexture* findTexture(const JsonModel& model, const std::string& key) noexcept {
 for(const JsonModelTexture& texture : model.textures) {
  if(texture.name == key) {
   return &texture;
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
 const std::string cullFace = value["cullface"].asString();
 if(!cullFace.empty()) {
  const auto* it =
      std::find(std::begin(kFaceNames), std::end(kFaceNames), cullFace == "bottom" ? "down" : cullFace);
  if(it == std::end(kFaceNames)) {
   error = "unknown cullface name: " + cullFace;
   return false;
  }
  out.cullFace = static_cast<int>(it - std::begin(kFaceNames));
 }
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
 // Blockbench writes the uv coordinate space here; it is independent of the
 // texture image's real pixel size (a 32x32 png with texture_size [16, 16]
 // still uses 0..16 uvs across the whole image).
 if(readVec(root["texture_size"], out.textureSize, 2) && out.textureSize[0] > 0.0 && out.textureSize[1] > 0.0) {
  out.hasTextureSize = true;
 } else {
  out.textureSize[0] = 16.0;
  out.textureSize[1] = 16.0;
 }
 const JsonValue& textures = root["textures"];
 if(textures.isObject()) {
  for(const auto& [key, value] : textures.members()) {
   if(value.isString()) {
    out.textures.push_back({key, value.asString(), {}});
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
 // texture_size is inherited too, and it re-scopes the uvs of the elements the
 // child itself declared. Parent elements already carry the parent's size.
 if(!child.hasTextureSize && parent.hasTextureSize) {
  for(JsonModelElement& element : child.elements) {
   element.textureWidth = parent.textureSize[0];
   element.textureHeight = parent.textureSize[1];
  }
  child.textureSize[0] = parent.textureSize[0];
  child.textureSize[1] = parent.textureSize[1];
  child.hasTextureSize = true;
 }
 if(!child.hasElements && parent.hasElements) {
  child.elements = parent.elements;
  child.hasElements = true;
 }
 for(const JsonModelTexture& texture : parent.textures) {
  if(findTexture(child, texture.name) == nullptr) {
   child.textures.push_back(texture);
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
    {{0, 1, 0}, {0, 0, 0}, {1, 0, 0}, {1, 1, 0}}, // north
    {{1, 1, 1}, {1, 0, 1}, {0, 0, 1}, {0, 1, 1}}, // south
    {{0, 1, 0}, {0, 0, 0}, {0, 0, 1}, {0, 1, 1}}, // west
    {{1, 1, 1}, {1, 0, 1}, {1, 0, 0}, {1, 1, 0}}, // east
};
constexpr float kFaceShade[kModelFaceCount] = {0.5f, 1.0f, 0.8f, 0.8f, 0.6f, 0.6f};
constexpr int kFaceAxis[3] = {1, 2, 0};
constexpr int kFaceOffsets[kModelFaceCount][3] = {
    {0, -1, 0},
    {0, 1, 0},
    {0, 0, -1},
    {0, 0, 1},
    {-1, 0, 0},
    {1, 0, 0},
};
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
bool resolveTexture(const JsonModel& model,
                    const std::string& reference,
                    const std::string& basePath,
                    std::string& out) {
 std::string key = reference;
 std::string textureBasePath = basePath;
 std::set<std::string> seen;
 while(!key.empty() && key[0] == '#') {
  key.erase(0, 1);
  if(!seen.insert(key).second) {
   return false;
  }
  const JsonModelTexture* texture = findTexture(model, key);
  if(texture == nullptr) {
   return false;
  }
  key = texture->value;
  textureBasePath = texture->basePath;
 }
 if(key.empty() || key == "missing") {
  return false;
 }
 const std::size_t colon = key.find(':');
 if(colon != std::string::npos) {
  key = "assets/" + key.substr(0, colon) + "/textures/" + key.substr(colon + 1);
 } else if(key.compare(0, 7, "assets/") != 0 && key.compare(0, 5, "mods/") != 0) {
  key = textureBasePath + key;
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
 batch.textureId = net::minecraft::registry::TextureRegistry::getOrRegisterTexture(texturePath);
 return batch.quads;
}
int boundaryCullFace(const JsonModelElement& element, int faceIndex) {
 if(element.hasRotation) {
  return -1;
 }
 const int axis = kFaceAxis[faceIndex / 2];
 const bool negative = (faceIndex % 2) == 0;
 const double coordinate = negative ? element.from[axis] : element.to[axis];
 return coordinate == (negative ? 0.0 : 16.0) ? faceIndex : -1;
}
// True when the element has thickness on every axis, i.e. it is a real box and
// not one of the flat planes Blockbench exports for cross/billboard shapes
// (whose two opposite faces are the same rectangle and must both survive).
bool isSolidElement(const JsonModelElement& element) noexcept {
 return element.from[0] < element.to[0] && element.from[1] < element.to[1] && element.from[2] < element.to[2];
}
// Two faces of different elements that sit on the same plane, cover the same
// rectangle and point at each other are sealed against one another: whatever
// the camera angle, each is hidden by the element behind the other. Blockbench
// models made of stacked boxes (repair_table's top and base) produce these in
// bulk and they z-fight when drawn, so they are dropped while baking.
bool facesSealAgainstEachOther(const JsonModelElement& a, int faceA, const JsonModelElement& b, int faceB) {
 if(faceA / 2 != faceB / 2 || faceA == faceB || a.hasRotation || b.hasRotation) {
  return false;
 }
 if(!isSolidElement(a) || !isSolidElement(b)) {
  return false;
 }
 const int axis = kFaceAxis[faceA / 2];
 const double planeA = (faceA % 2) == 0 ? a.from[axis] : a.to[axis];
 const double planeB = (faceB % 2) == 0 ? b.from[axis] : b.to[axis];
 if(planeA != planeB) {
  return false;
 }
 for(int other = 0; other < 3; ++other) {
  if(other != axis && (a.from[other] != b.from[other] || a.to[other] != b.to[other])) {
   return false;
  }
 }
 return true;
}
// Bakes a parent-flattened model into world-space quads (positions in block
// units, 0..1 for a full cube). basePath is the model file's directory and
// anchors relative texture paths.
bool bakeJsonModel(const JsonModel& model, const std::string& basePath, BakedModel& out, std::string& error) {
 out.batches.clear();
 const std::size_t elementCount = model.elements.size();
 std::vector<bool> sealed(elementCount * kModelFaceCount, false);
 for(std::size_t i = 0; i < elementCount; ++i) {
  for(std::size_t j = i + 1; j < elementCount; ++j) {
   for(int faceA = 0; faceA < kModelFaceCount; ++faceA) {
    if(!model.elements[i].faces[faceA].present) {
     continue;
    }
    const int faceB = (faceA % 2) == 0 ? faceA + 1 : faceA - 1;
    if(!model.elements[j].faces[faceB].present ||
       !facesSealAgainstEachOther(model.elements[i], faceA, model.elements[j], faceB)) {
     continue;
    }
    sealed[i * kModelFaceCount + static_cast<std::size_t>(faceA)] = true;
    sealed[j * kModelFaceCount + static_cast<std::size_t>(faceB)] = true;
   }
  }
 }
 for(std::size_t elementIndex = 0; elementIndex < elementCount; ++elementIndex) {
  const JsonModelElement& element = model.elements[elementIndex];
  for(int faceIndex = 0; faceIndex < kModelFaceCount; ++faceIndex) {
   const JsonModelFaceSpec& face = element.faces[faceIndex];
   if(!face.present || sealed[elementIndex * kModelFaceCount + static_cast<std::size_t>(faceIndex)]) {
    continue;
   }
   std::string texturePath;
   const std::string& textureBasePath = element.basePath.empty() ? basePath : element.basePath;
   if(!resolveTexture(model, face.texture, textureBasePath, texturePath)) {
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
   quad.cullFace = face.cullFace >= 0 ? face.cullFace : boundaryCullFace(element, faceIndex);
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
    vertex.u = static_cast<float>(corner[0] / element.textureWidth);
    vertex.v = static_cast<float>(corner[1] / element.textureHeight);
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
// Models are const from the moment they land here, and the unique_ptr keeps each
// one at a fixed address as the vector grows. That is what lets lookups hand out
// a bare pointer and drop the lock: the callers (chunk-mesh workers, the render
// thread, Lua) only ever read, and never race with a concurrent store.
struct ModelStore {
 std::mutex mutex;
 std::vector<std::unique_ptr<const BakedModel>> models; // handle - 1 indexes this
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
std::string normalizeModelPath(std::string path) {
 std::replace(path.begin(), path.end(), '\\', '/');
 path = std::filesystem::path(path).lexically_normal().generic_string();
 while(path.starts_with("./")) {
  path.erase(0, 2);
 }
 return path == "." ? std::string() : path;
}
std::string parentModelPath(const std::string& parent, const std::string& basePath) {
 const bool explicitRelative = parent.starts_with("./") || parent.starts_with(".\\") || parent.starts_with("../") ||
                               parent.starts_with("..\\");
 std::string path = normalizeModelPath(parent);
 const std::size_t colon = path.find(':');
 if(colon != std::string::npos) {
  path = "assets/" + path.substr(0, colon) + "/models/" + path.substr(colon + 1);
 } else if(!path.starts_with("assets/") && !path.starts_with("mods/") && !path.starts_with("models/")) {
  if(explicitRelative) {
   path = basePath + path;
  } else if(path.find('/') != std::string::npos) {
   path = "assets/minecraft/models/" + path;
  } else {
   path = basePath + path;
  }
 }
 if(path.size() < 5 || path.compare(path.size() - 5, 5, ".json") != 0) {
  path += ".json";
 }
 return normalizeModelPath(path);
}
bool loadModelFile(const std::string& modId, const std::string& path, JsonModel& out, std::string& error) {
 const std::filesystem::path file = runtime::host().assetPath(modId, path);
 if(file.empty() || !std::filesystem::is_regular_file(file)) {
  error = "model not found: " + path;
  return false;
 }
 const std::string text = lua::readFileText(file);
 JsonValue root;
 if(!JsonValue::parse(text, root, error)) {
  error = path + ": " + error;
  return false;
 }
 if(!parseJsonModel(root, out, error)) {
  error = path + ": " + error;
  return false;
 }
 const std::string basePath = directoryOf(path);
 for(JsonModelTexture& texture : out.textures) {
  texture.basePath = basePath;
 }
 for(JsonModelElement& element : out.elements) {
  element.basePath = basePath;
  element.textureWidth = out.textureSize[0];
  element.textureHeight = out.textureSize[1];
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
 const std::string normalizedPath = normalizeModelPath(path);
 const std::string key = modId + "|" + normalizedPath;
 if(const int cached = bakedModelHandleForKey(key)) {
  return cached;
 }
 JsonModel merged;
 if(!loadModelFile(modId, normalizedPath, merged, error)) {
  return 0;
 }
 // Flatten the parent chain. Blockbench exports sometimes carry dangling or
 // self-referential parents; those are skipped rather than fatal.
 std::set<std::string> visited{normalizedPath};
 std::string basePath = directoryOf(normalizedPath);
 std::string parent = merged.parent;
 while(!parent.empty()) {
  const std::string parentPath = parentModelPath(parent, basePath);
  if(!visited.insert(parentPath).second) {
   break;
  }
  JsonModel parentModel;
  std::string parentError;
  if(!loadModelFile(modId, parentPath, parentModel, parentError)) {
   lua::runtimeLog(
       modId, "warn", "model " + normalizedPath + " skipping parent " + parent + " (" + parentError + ")");
   break;
  }
  mergeParentModel(merged, parentModel);
  basePath = directoryOf(parentPath);
  parent = parentModel.parent;
 }
 auto baked = std::make_unique<BakedModel>();
 if(!bakeJsonModel(merged, directoryOf(normalizedPath), *baked, error)) {
  error = normalizedPath + ": " + error;
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
namespace {
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
InstanceStore& instanceStore() {
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
int placeModelInstance(const std::string& modId, int handle, const ModelTransform& transform, const std::string& tag) {
 const WorldBox box = worldBoxFor(handle, transform);
 if(!box.valid) {
  return 0;
 }
 InstanceStore& instances = instanceStore();
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
 InstanceStore& instances = instanceStore();
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
 InstanceStore& instances = instanceStore();
 const std::lock_guard<std::mutex> lock(instances.mutex);
 const auto it = std::find_if(instances.instances.begin(),
                              instances.instances.end(),
                              [instanceId](const ModelInstance& i) { return i.id == instanceId; });
 if(it == instances.instances.end()) {
  return false;
 }
 instances.instances.erase(it);
 return true;
}
void clearModelInstances(const std::string& modId) {
 InstanceStore& instances = instanceStore();
 const std::lock_guard<std::mutex> lock(instances.mutex);
 instances.instances.erase(std::remove_if(instances.instances.begin(),
                                          instances.instances.end(),
                                          [&modId](const ModelInstance& i) { return i.modId == modId; }),
                           instances.instances.end());
}
bool raycastModelInstances(
    double ox, double oy, double oz, double dx, double dy, double dz, double maxDistance, ModelRaycastHit& hit) {
 const double length = std::sqrt(dx * dx + dy * dy + dz * dz);
 if(length < 1.0e-9) {
  return false;
 }
 const double origin[3] = {ox, oy, oz};
 const double dir[3] = {dx / length, dy / length, dz / length};
 InstanceStore& instances = instanceStore();
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
using namespace net::minecraft::mod::lua;
using namespace net::minecraft::client::render::item;
namespace {
using net::minecraft::BlockView;
using net::minecraft::block::Block;
using net::minecraft::client::render::Tessellator;
using net::minecraft::client::render::block::BlockRenderManager;
#ifdef MINECRAFT_NATIVE_EXPORTS
using net::minecraft::client::render::RenderSystem;
class ModBlockInventoryScope {
 public:
 ModBlockInventoryScope() : saved_(RenderSystem::getShadow()) {
  RenderSystem::depthTestWrite(true);
  RenderSystem::disableLighting();
  RenderSystem::enableTexture();
  RenderSystem::color4f(1.0f, 1.0f, 1.0f, 1.0f);
 }
 ~ModBlockInventoryScope() {
  RenderSystem::setShadow(saved_);
 }

 private:
 RenderSystem::StateShadow saved_;
};
using runtime::ModLuaDrawScope;
using MatrixScope = RenderSystem::MatrixScope;
// Camera-relative placement shared by drawBakedModelWorld and drawItemStackWorld.
// Leaves the caller to apply whatever model-space recentring its geometry needs.
void applyWorldDrawTransform(const WorldModelDraw& options) {
 const client::render::FrameRenderCamera& camera = client::render::RenderCameraState::instance().frame();
 RenderSystem::translate(static_cast<float>(options.x - camera.x),
                         static_cast<float>(options.y - camera.y),
                         static_cast<float>(options.z - camera.z));
 if(options.yaw != 0.0f) {
  RenderSystem::rotate(options.yaw, 0.0f, 1.0f, 0.0f);
 }
 if(options.pitch != 0.0f) {
  RenderSystem::rotate(options.pitch, 1.0f, 0.0f, 0.0f);
 }
 if(options.roll != 0.0f) {
  RenderSystem::rotate(options.roll, 0.0f, 0.0f, 1.0f);
 }
 if(options.scale != 1.0f) {
  RenderSystem::scale(options.scale, options.scale, options.scale);
 }
}
float worldBrightness(const WorldModelDraw& options) {
 if(options.brightness >= 0.0f) {
  return options.brightness;
 }
 if(client::Minecraft::INSTANCE != nullptr && client::Minecraft::INSTANCE->world != nullptr) {
  return client::Minecraft::INSTANCE->world->getLightBrightness(static_cast<int>(std::floor(options.x)),
                                                                static_cast<int>(std::floor(options.y)),
                                                                static_cast<int>(std::floor(options.z)));
 }
 return 1.0f;
}
#endif
// Where a block's baked model is being drawn. Passed explicitly down the draw
// path; it used to be a thread-local that every emit helper reached for, which
// meant a missing scope guard produced no geometry and no diagnostic.
struct BlockModelDraw {
 BlockRenderManager* manager = nullptr;
 Block* block = nullptr;
 int x = 0;
 int y = 0;
 int z = 0;
 bool inventory = false;
 float brightness = 1.0f;
};
// Item draws bind their sprite up front, but a baked model can reference other
// textures; rebind per quad so multi-texture item models look like their block
// counterparts.
void bindItemModelTexture(int textureId) {
#ifdef MINECRAFT_NATIVE_EXPORTS
 if(!net::minecraft::registry::TextureRegistry::isCustomTexture(textureId) || client::Minecraft::INSTANCE == nullptr) {
  return;
 }
 client::texture::TextureManager& textures = client::Minecraft::INSTANCE->textureManager;
 const int glId = net::minecraft::registry::TextureRegistry::resolveGlId(textureId, textures);
 if(glId >= 0) {
  textures.bindTexture(glId);
 }
#else
 (void)textureId;
#endif
}
// ---------------------------------------------------------------------------
// Baked-model quad emission.
//
// One loop turns a BakedModel into quads for every destination — world blocks,
// inventory icons, item models, minecraft.model.draw. A caller supplies where
// each texture batch goes and what to do with a finished quad; the cullface
// test, the per-quad transform, the shade/tint math, the face normal and the uv
// mapping live here and nowhere else.
// A baked vertex after the per-quad transform, in double precision so a world
// draw can carry the block's grid offset without losing model detail.
struct TransformedVertex {
 double x = 0.0;
 double y = 0.0;
 double z = 0.0;
 double u = 0.0;
 double v = 0.0;
};
struct EmittedQuad {
 const TransformedVertex* vertices = nullptr;
 float nx = 0.0f;
 float ny = 1.0f;
 float nz = 0.0f;
 float red = 1.0f;
 float green = 1.0f;
 float blue = 1.0f;
 float alpha = 1.0f;
};
// Face normal from the first three vertices, in the winding the vanilla face
// renderers hand to Tessellator::normal.
void assignQuadNormal(EmittedQuad& quad) {
 client::render::block::quadNormal(quad.vertices[0].x,
                                   quad.vertices[0].y,
                                   quad.vertices[0].z,
                                   quad.vertices[1].x,
                                   quad.vertices[1].y,
                                   quad.vertices[1].z,
                                   quad.vertices[2].x,
                                   quad.vertices[2].y,
                                   quad.vertices[2].z,
                                   quad.nx,
                                   quad.ny,
                                   quad.nz);
}
// The one place a baked vertex reaches a Tessellator. Same shape as the vanilla
// face renderers: normal, colour, then four uv-mapped vertices.
void writeQuad(Tessellator& t,
               const EmittedQuad& quad,
               double baseX,
               double baseY,
               double baseZ,
               float light,
               float alphaScale) {
 t.color(quad.red * light, quad.green * light, quad.blue * light, quad.alpha * alphaScale);
 for(int i = 0; i < 4; ++i) {
  // uv is already normalized 0..1 across the batch's own image, which is what
  // the bound texture is: mod content never lives in the vanilla atlas, so
  // there is no tile to fold these into.
  client::render::block::emitBlockVertex(t,
                                         quad.nx,
                                         quad.ny,
                                         quad.nz,
                                         baseX + quad.vertices[i].x,
                                         baseY + quad.vertices[i].y,
                                         baseZ + quad.vertices[i].z,
                                         quad.vertices[i].u,
                                         quad.vertices[i].v);
 }
}
// beginBatch(batch) -> bool  : false skips the batch (texture unresolvable).
// takeQuad(batch, quad)      : consume one transformed, culled quad.
template <typename BeginBatch, typename TakeQuad>
bool forEachBakedQuad(const BakedModel& baked,
                      const BakedQuadTransform& transform,
                      const BlockModelDraw* cullDraw,
                      const BlockView* cullView,
                      BeginBatch beginBatch,
                      TakeQuad takeQuad) {
 // The overwhelmingly common case: a block or icon drawn at its baked pose.
 // Nothing then needs the per-vertex rotate/scale below, and cullface (which
 // only means anything against the block grid) stays meaningful.
 const bool placedAsBaked = transform.scale == 1.0f && transform.offsetX == 0.0f && transform.offsetY == 0.0f &&
                            transform.offsetZ == 0.0f && transform.yaw == 0.0f && transform.pitch == 0.0f &&
                            transform.roll == 0.0f;
 const bool cullFaces = placedAsBaked && cullDraw != nullptr && cullView != nullptr;
 static constexpr double zeroOrigin[3] = {0.0, 0.0, 0.0};
 bool emitted = false;
 for(const BakedTextureBatch& batch : baked.batches) {
  if(!beginBatch(batch)) {
   continue;
  }
  for(const BakedQuad& quad : batch.quads) {
   if(cullFaces && quad.cullFace >= 0) {
    const int* offset = kFaceOffsets[quad.cullFace];
    if(!cullDraw->block->isSideVisible(
           cullView, cullDraw->x + offset[0], cullDraw->y + offset[1], cullDraw->z + offset[2], quad.cullFace)) {
     continue;
    }
   }
   TransformedVertex vertices[4];
   for(int i = 0; i < 4; ++i) {
    if(placedAsBaked) {
     vertices[i].x = quad.vertices[i].x;
     vertices[i].y = quad.vertices[i].y;
     vertices[i].z = quad.vertices[i].z;
    } else {
     // Rotation and scale act around the model's centre, so shift there first.
     double point[3] = {quad.vertices[i].x - 0.5, quad.vertices[i].y - 0.5, quad.vertices[i].z - 0.5};
     if(transform.pitch != 0.0f) {
      rotatePoint(point, zeroOrigin, 'x', transform.pitch);
     }
     if(transform.yaw != 0.0f) {
      rotatePoint(point, zeroOrigin, 'y', transform.yaw);
     }
     if(transform.roll != 0.0f) {
      rotatePoint(point, zeroOrigin, 'z', transform.roll);
     }
     vertices[i].x = point[0] * transform.scale + 0.5 + transform.offsetX;
     vertices[i].y = point[1] * transform.scale + 0.5 + transform.offsetY;
     vertices[i].z = point[2] * transform.scale + 0.5 + transform.offsetZ;
    }
    vertices[i].u = quad.vertices[i].u;
    vertices[i].v = quad.vertices[i].v;
   }
   EmittedQuad out;
   out.vertices = vertices;
   out.red = quad.red * quad.shade * transform.colorR;
   out.green = quad.green * quad.shade * transform.colorG;
   out.blue = quad.blue * quad.shade * transform.colorB;
   out.alpha = quad.alpha;
   assignQuadNormal(out);
   takeQuad(batch, out);
   emitted = true;
  }
 }
 return emitted;
}
#ifdef MINECRAFT_NATIVE_EXPORTS
// Render-thread-only scratch tessellator for the world draw paths. Shared
// rather than local because constructing a Tessellator reserves a
// multi-megabyte vertex buffer; every user brackets its own startQuads/draw.
Tessellator& worldDrawTessellator() {
 static Tessellator tess;
 return tess;
}
#endif
// Resolves a block quad's final texture and tessellator and writes it.
bool writeBlockQuad(const BlockModelDraw& draw, const EmittedQuad& quad, int textureId) {
 if(draw.manager == nullptr || draw.block == nullptr) {
  return false;
 }
 if(textureId < 0) {
  textureId = draw.block->textureId;
 }
 BlockRenderManager& manager = *draw.manager;
 if(!draw.inventory && manager.ctx.textureOverride >= 0) {
  textureId = manager.ctx.textureOverride;
 }
 manager.ctx.bindTextureFor(textureId);
 Tessellator& t = draw.inventory ? *manager.ctx.tess : manager.ctx.activeTess(textureId);
 // Capture mode owns the batch lifecycle (the chunk builder started it and will
 // draw it); an immediate draw has to bracket every quad itself.
 const bool capturing = !draw.inventory && manager.ctx.modMeshes != nullptr;
 // World quads sit at the block's grid position; inventory quads stay in 0..1.
 const double baseX = draw.inventory ? 0.0 : static_cast<double>(draw.x);
 const double baseY = draw.inventory ? 0.0 : static_cast<double>(draw.y);
 const double baseZ = draw.inventory ? 0.0 : static_cast<double>(draw.z);
 if(!capturing) {
  t.startQuads();
 }
 writeQuad(t, quad, baseX, baseY, baseZ, draw.brightness, 1.0f);
 if(!capturing) {
  t.draw();
 }
 return true;
}
bool drawBakedBlockModel(const BlockModelDraw& draw, int handle, const BakedQuadTransform& transform) {
 const BakedModel* baked = bakedModelForHandle(handle);
 if(baked == nullptr) {
  return false;
 }
 // Cullface only means anything against the block grid, so an inventory icon
 // keeps every quad.
 const bool grid = !draw.inventory && draw.block != nullptr && draw.manager != nullptr;
 return forEachBakedQuad(*baked,
                         transform,
                         grid ? &draw : nullptr,
                         grid ? draw.manager->ctx.blockView : nullptr,
                         [](const BakedTextureBatch&) { return true; },
                         [&](const BakedTextureBatch& batch, const EmittedQuad& quad) {
                          writeBlockQuad(draw, quad, batch.textureId);
                         });
}
bool drawBakedItemModel(Tessellator& tess, float brightness, int handle) {
 const BakedModel* baked = bakedModelForHandle(handle);
 if(baked == nullptr) {
  return false;
 }
 return forEachBakedQuad(*baked,
                         BakedQuadTransform{},
                         nullptr,
                         nullptr,
                         [](const BakedTextureBatch&) { return true; },
                         [&](const BakedTextureBatch& batch, const EmittedQuad& quad) {
                          bindItemModelTexture(batch.textureId);
                          tess.startQuads();
                          writeQuad(tess, quad, 0.0, 0.0, 0.0, brightness, 1.0f);
                          tess.draw();
                         });
}
// Baked-model equivalent of LuaModBlock::getRenderBounds/getColorMultiplier:
// register_block's coordinate_bounds/coordinate_color have to be applied here
// too, since a block with a model never takes the vanilla cube path.
BakedQuadTransform coordinateQuadTransform(const BlockRegistrationSpec& spec, int x, int y, int z) {
 BakedQuadTransform transform;
 if(spec.coordinateBounds) {
  const lua::CoordinateVariedTransform varied = lua::coordinateVariedTransform(spec, x, y, z);
  transform.scale = varied.scale;
  transform.offsetX = varied.offsetX;
  transform.offsetY = varied.offsetY;
  transform.offsetZ = varied.offsetZ;
 }
 if(spec.coordinateColor) {
  const int color = net::minecraft::util::math::coordinateColor(x, y, z);
  transform.colorR = static_cast<float>((color >> 16) & 0xFF) / 255.0f;
  transform.colorG = static_cast<float>((color >> 8) & 0xFF) / 255.0f;
  transform.colorB = static_cast<float>(color & 0xFF) / 255.0f;
 }
 return transform;
}
} // namespace
bool drawLuaBlockWorld(BlockRenderManager& manager, Block& block, int x, int y, int z) {
 const BlockRegistrationSpec* spec = blockRegistrationSpecForId(block.id);
 if(spec == nullptr || spec->bakedModel == 0) {
  return false;
 }
 const BlockModelDraw draw{
     &manager, &block, x, y, z, false, block.getLuminance(manager.ctx.blockView, x, y, z)};
 RenderSystem::alphaTest(0.1f);
 if(spec->coordinateBounds || spec->coordinateColor) {
  return drawBakedBlockModel(draw, spec->bakedModel, coordinateQuadTransform(*spec, x, y, z));
 }
 return drawBakedBlockModel(draw, spec->bakedModel, BakedQuadTransform{});
}
void drawLuaBlockInventory(BlockRenderManager& manager, Block& block, int /*metadata*/, float brightness) {
 const BlockRegistrationSpec* spec = blockRegistrationSpecForId(block.id);
 if(spec == nullptr || spec->bakedModel == 0) {
  return;
 }
 const ModBlockInventoryScope inventoryDraw;
 const MatrixScope matrix;
 RenderSystem::translate(-0.5f, -0.5f, -0.5f);
 const BlockModelDraw draw{&manager, &block, 0, 0, 0, true, brightness};
 drawBakedBlockModel(draw, spec->bakedModel, BakedQuadTransform{});
}
bool drawLuaItemModel(Tessellator& tessellator, const ItemStack& stack, float brightness) {
 const ItemRegistrationSpec* spec = itemRegistrationSpecForId(stack.itemId);
 if(spec == nullptr || spec->bakedModel == 0) {
  return false;
 }
 return drawBakedItemModel(tessellator, brightness, spec->bakedModel);
}
#ifdef MINECRAFT_NATIVE_EXPORTS
bool drawBakedModelWorld(int handle, const WorldModelDraw& options) {
 const BakedModel* baked = bakedModelForHandle(handle);
 if(baked == nullptr || !runtime::ModWorldDrawContext::active() || client::Minecraft::INSTANCE == nullptr) {
  return false;
 }
 const float brightness = worldBrightness(options);
 const bool textured = !baked->batches.empty() && !baked->batches.front().texturePath.empty();
 const ModLuaDrawScope modCaps(textured, options.blend, options.cull, options.depthTest, options.depthWrite);
 const MatrixScope matrix;
 applyWorldDrawTransform(options);
 RenderSystem::translate(-0.5f, -options.pivotY, -0.5f);
 client::texture::TextureManager& textures = client::Minecraft::INSTANCE->textureManager;
 Tessellator& tess = worldDrawTessellator();
 bool open = false;
 // A world draw places the model with the matrix above, so the quads go out at
 // their baked pose: no per-quad transform, and no grid to cull against. Each
 // batch binds its own texture by path, so one open batch spans its quads.
 forEachBakedQuad(
     *baked,
     BakedQuadTransform{},
     nullptr,
     nullptr,
     [&](const BakedTextureBatch& batch) {
      if(batch.texturePath.empty()) {
       return false;
      }
      if(open) {
       tess.draw();
      }
      const int glId = textures.getTextureId(batch.texturePath);
      if(glId >= 0) {
       textures.bindTexture(glId);
      }
      tess.startQuads();
      open = true;
      return true;
     },
     [&](const BakedTextureBatch& /*batch*/, const EmittedQuad& quad) {
      writeQuad(tess, quad, 0.0, 0.0, 0.0, brightness, options.alpha);
     });
 if(open) {
  tess.draw();
 }
 return true;
}
bool drawItemStackWorld(const ItemStack& stack, const WorldModelDraw& options) {
 if(!runtime::ModWorldDrawContext::active() || client::Minecraft::INSTANCE == nullptr) {
  return false;
 }
 const bool custom = ItemModelRenderer::hasCustomModel(stack);
 const bool blockModel = !custom && ItemModelRenderer::rendersAsBlockModel(stack);
 if(!custom && !blockModel) {
  return false;
 }
 Block* block = blockModel ? ItemModelRenderer::blockOf(stack) : nullptr;
 if(blockModel && block == nullptr) {
  return false;
 }
 const float brightness = worldBrightness(options);
 const ModLuaDrawScope modCaps(true, options.blend, options.cull, options.depthTest, options.depthWrite);
 const MatrixScope matrix;
 applyWorldDrawTransform(options);
 client::texture::TextureManager& textures = client::Minecraft::INSTANCE->textureManager;
 if(custom) {
  // Custom item models are baked in 0..1 model space; recentre onto the pivot.
  RenderSystem::translate(-0.5f, -options.pivotY, -0.5f);
  textures.bindTextureOrAtlas(stack.getTextureId(), ItemModelRenderer::spriteAtlasPath(stack));
  // Not Tessellator::INSTANCE: a world item draw can happen while an outer
  // renderer has a batch open on INSTANCE, and draw()ing it here would flush
  // that half-built geometry with this item's matrix and texture.
  return drawLuaItemModel(worldDrawTessellator(), stack, brightness);
 }
 // The inventory block renderers (vanilla and drawLuaBlockInventory) emit
 // geometry already centred on the origin, so only the pivot's deviation
 // from centre needs compensating; a second -0.5 shift would put the cube's
 // visual centre half a block off the rotation pivot (lopsided tumbling).
 if(options.pivotY != 0.5f) {
  RenderSystem::translate(0.0f, 0.5f - options.pivotY, 0.0f);
 }
 textures.bindTextureOrAtlas(block->textureId, "/terrain.png");
 static BlockRenderManager itemDropBlockManager;
 auto* previousTextureManager = itemDropBlockManager.ctx.textureManager;
 const bool previousUseAo = itemDropBlockManager.ctx.faceState.useAo;
 itemDropBlockManager.ctx.textureManager = &textures;
 itemDropBlockManager.ctx.faceState.useAo = false;
 itemDropBlockManager.render(*block, stack.getDamage(), brightness);
 itemDropBlockManager.ctx.textureManager = previousTextureManager;
 itemDropBlockManager.ctx.faceState.useAo = previousUseAo;
 return true;
}
#else
bool drawBakedModelWorld(int /*handle*/, const WorldModelDraw& /*options*/) {
 return false;
}
bool drawItemStackWorld(const ItemStack& /*stack*/, const WorldModelDraw& /*options*/) {
 return false;
}
#endif
bool itemStackBounds(const ItemStack& stack, BakedBounds& outBounds) {
 if(ItemModelRenderer::hasCustomModel(stack)) {
  const ItemRegistrationSpec* spec = itemRegistrationSpecForId(stack.itemId);
  if(spec == nullptr || spec->bakedModel == 0) {
   return false;
  }
  const BakedModel* baked = bakedModelForHandle(spec->bakedModel);
  if(baked == nullptr || baked->bounds.empty) {
   return false;
  }
  outBounds = baked->bounds;
  return true;
 }
 if(ItemModelRenderer::rendersAsBlockModel(stack)) {
  Block* block = ItemModelRenderer::blockOf(stack);
  if(block == nullptr) {
   return false;
  }
  const BlockRegistrationSpec* spec = blockRegistrationSpecForId(block->id);
  if(spec != nullptr && spec->bakedModel != 0) {
   const BakedModel* baked = bakedModelForHandle(spec->bakedModel);
   if(baked != nullptr && !baked->bounds.empty) {
    outBounds = baked->bounds;
    return true;
   }
  }
  // Vanilla (and shape-less custom) blocks: approximate with the full unit
  // cube. Non-cube shapes (stairs, fences, ...) are subsets of this box, so
  // it is a safe (if slightly loose) tumble hitbox rather than an exact one.
  outBounds.min[0] = outBounds.min[1] = outBounds.min[2] = 0.0f;
  outBounds.max[0] = outBounds.max[1] = outBounds.max[2] = 1.0f;
  outBounds.empty = false;
  return true;
 }
 return false;
}
} // namespace net::minecraft::mod::model
namespace net::minecraft::mod::runtime {
using namespace net::minecraft::mod::lua;
namespace {
int luaModelLoad(lua_State* state) {
 LuaApi& api = luaApi();
 ModHost::LoadedLuaMod* mod = currentLuaMod(state);
 const std::string path = luaString(state, 1, "");
 if(mod == nullptr || path.empty()) {
  api.pushnil(state);
  api.pushstring(state, "model.load expects an asset path");
  return 2;
 }
 std::string error;
 const int handle = model::loadBakedModel(mod->modId, path, error);
 if(handle == 0) {
  api.pushnil(state);
  api.pushstring(state, error.c_str());
  return 2;
 }
 api.pushinteger(state, handle);
 return 1;
}
// Reads a placement transform from the options table at optsIndex.
model::ModelTransform readTransform(lua_State* state, int optsIndex) {
 model::ModelTransform t;
 if(optsIndex != 0) {
  t.x = luaDoubleField(state, optsIndex, "x", 0.0);
  t.y = luaDoubleField(state, optsIndex, "y", 0.0);
  t.z = luaDoubleField(state, optsIndex, "z", 0.0);
  t.yaw = luaFloatField(state, optsIndex, "yaw", 0.0f);
  t.pitch = luaFloatField(state, optsIndex, "pitch", 0.0f);
  t.roll = luaFloatField(state, optsIndex, "roll", 0.0f);
  t.scale = luaFloatField(state, optsIndex, "scale", 1.0f);
  t.pivotY = luaFloatField(state, optsIndex, "pivot_y", 0.0f);
 }
 return t;
}
// model.place(handle, opts) -> instance id. Registers a hitbox the engine's
// raycast honors; the box tracks the transform's scale automatically.
int luaModelPlace(lua_State* state) {
 LuaApi& api = luaApi();
 ModHost::LoadedLuaMod* mod = currentLuaMod(state);
 const int handle = luaIntArg(state, 1, 0);
 if(mod == nullptr || handle <= 0) {
  api.pushnil(state);
  api.pushstring(state, "model.place expects (handle, opts)");
  return 2;
 }
 const int optsIndex = api.type(state, 2) == kLuaTTable ? 2 : 0;
 const model::ModelTransform transform = readTransform(state, optsIndex);
 const std::string tag = optsIndex != 0 ? luaStringField(state, optsIndex, "tag", "") : std::string();
 const int instanceId = model::placeModelInstance(mod->modId, handle, transform, tag);
 if(instanceId == 0) {
  api.pushnil(state);
  api.pushstring(state, "model.place failed: unknown or empty model");
  return 2;
 }
 api.pushinteger(state, instanceId);
 return 1;
}
int luaModelUpdate(lua_State* state) {
 LuaApi& api = luaApi();
 const int instanceId = luaIntArg(state, 1, 0);
 const int optsIndex = api.type(state, 2) == kLuaTTable ? 2 : 0;
 const model::ModelTransform transform = readTransform(state, optsIndex);
 api.pushboolean(state, model::updateModelInstance(instanceId, transform) ? 1 : 0);
 return 1;
}
int luaModelRemove(lua_State* state) {
 LuaApi& api = luaApi();
 api.pushboolean(state, model::removeModelInstance(luaIntArg(state, 1, 0)) ? 1 : 0);
 return 1;
}
int luaModelClear(lua_State* state) {
 ModHost::LoadedLuaMod* mod = currentLuaMod(state);
 if(mod != nullptr) {
  model::clearModelInstances(mod->modId);
 }
 return 0;
}
int luaModelBounds(lua_State* state) {
 LuaApi& api = luaApi();
 const model::BakedModel* baked = model::bakedModelForHandle(luaIntArg(state, 1, 0));
 if(baked == nullptr || baked->bounds.empty) {
  api.pushnil(state);
  return 1;
 }
 const model::BakedBounds& b = baked->bounds;
 api.createtable(state, 0, 6);
 setFields(state,
           "min_x",
           static_cast<double>(b.min[0]),
           "min_y",
           static_cast<double>(b.min[1]),
           "min_z",
           static_cast<double>(b.min[2]),
           "max_x",
           static_cast<double>(b.max[0]),
           "max_y",
           static_cast<double>(b.max[1]),
           "max_z",
           static_cast<double>(b.max[2]));
 return 1;
}
// Shared option parsing for minecraft.model.draw and minecraft.model.draw_item.
model::WorldModelDraw readWorldModelDraw(lua_State* state, int optsIndex) {
 model::WorldModelDraw options;
 if(optsIndex != 0) {
  options.x = luaDoubleField(state, optsIndex, "x", 0.0);
  options.y = luaDoubleField(state, optsIndex, "y", 0.0);
  options.z = luaDoubleField(state, optsIndex, "z", 0.0);
  options.yaw = luaFloatField(state, optsIndex, "yaw", 0.0f);
  options.pitch = luaFloatField(state, optsIndex, "pitch", 0.0f);
  options.roll = luaFloatField(state, optsIndex, "roll", 0.0f);
  options.pivotY = luaFloatField(state, optsIndex, "pivot_y", 0.0f);
  options.scale = luaFloatField(state, optsIndex, "scale", 1.0f);
  options.brightness = luaFloatField(state, optsIndex, "brightness", -1.0f);
  if(options.brightness >= 0.0f) {
   options.brightness = std::clamp(options.brightness, 0.0f, 1.0f);
  }
  options.alpha = std::clamp(luaFloatField(state, optsIndex, "a", 1.0f), 0.0f, 1.0f);
  options.blend = luaBoolField(state, optsIndex, "blend", true);
  options.cull = luaBoolField(state, optsIndex, "cull", false);
  options.depthTest = luaBoolField(state, optsIndex, "depth_test", true);
  options.depthWrite = luaBoolField(state, optsIndex, "depth_write", true);
 }
 return options;
}
// World-space draw for a baked model; option parsing here, GL work in
// model::drawBakedModelWorld (a no-op returning false without the client
// renderer).
int luaModelDraw(lua_State* state) {
 LuaApi& api = luaApi();
 const int handle = luaIntArg(state, 1, 0);
 const int optsIndex = api.gettop(state) >= 2 && api.type(state, 2) == kLuaTTable ? 2 : 0;
 const model::WorldModelDraw options = readWorldModelDraw(state, optsIndex);
 api.pushboolean(state, model::drawBakedModelWorld(handle, options) ? 1 : 0);
 return 1;
}
// minecraft.model.draw_item(item_id, damage, opts) -> drew a real 3D model
// (custom Lua item/block model, or the vanilla/mod block-cube renderer).
// false for plain sprite items with no 3D shape; callers should fall back to
// their own flat-icon representation in that case.
int luaModelDrawItem(lua_State* state) {
 LuaApi& api = luaApi();
 const int itemId = luaIntArg(state, 1, 0);
 const int damage = luaIntArg(state, 2, 0);
 const int optsIndex = api.gettop(state) >= 3 && api.type(state, 3) == kLuaTTable ? 3 : 0;
 const model::WorldModelDraw options = readWorldModelDraw(state, optsIndex);
 const ItemStack stack(itemId, 1, damage);
 api.pushboolean(state, model::drawItemStackWorld(stack, options) ? 1 : 0);
 return 1;
}
// minecraft.model.item_bounds(item_id, damage) -> model-space bounds table for
// the same items draw_item draws a real model for, or nil otherwise.
int luaModelItemBounds(lua_State* state) {
 LuaApi& api = luaApi();
 const int itemId = luaIntArg(state, 1, 0);
 const int damage = luaIntArg(state, 2, 0);
 const ItemStack stack(itemId, 1, damage);
 model::BakedBounds bounds;
 if(!model::itemStackBounds(stack, bounds)) {
  api.pushnil(state);
  return 1;
 }
 api.createtable(state, 0, 6);
 setFields(state,
           "min_x",
           static_cast<double>(bounds.min[0]),
           "min_y",
           static_cast<double>(bounds.min[1]),
           "min_z",
           static_cast<double>(bounds.min[2]),
           "max_x",
           static_cast<double>(bounds.max[0]),
           "max_y",
           static_cast<double>(bounds.max[1]),
           "max_z",
           static_cast<double>(bounds.max[2]));
 return 1;
}
// Reads one {x,y,z,u,v} vertex from the table at vtxIndex into vertex.
void readBuildVertex(lua_State* state, int vtxIndex, model::BakedVertex& vertex) {
 vertex.x = luaFloatField(state, vtxIndex, "x", 0.0f);
 vertex.y = luaFloatField(state, vtxIndex, "y", 0.0f);
 vertex.z = luaFloatField(state, vtxIndex, "z", 0.0f);
 vertex.u = luaFloatField(state, vtxIndex, "u", 0.0f);
 vertex.v = luaFloatField(state, vtxIndex, "v", 0.0f);
}
// model.build{quads = {{texture?, r,g,b,a?, shade?, vertices = {v1,v2,v3,v4}}, ...},
// key?} -> handle. Generic model builder: assembles arbitrary colored/textured
// quads into a baked model. All voxel geometry (sprite sampling, interior-face
// culling, cube generation) is built on top of this in Lua.
int luaModelBuild(lua_State* state) {
 LuaApi& api = luaApi();
 if(api.type(state, 1) != kLuaTTable) {
  api.pushnil(state);
  api.pushstring(state, "model.build expects an options table");
  return 2;
 }
 const std::string key = luaStringField(state, 1, "key", "");
 if(!key.empty()) {
  if(const int cached = model::bakedModelHandleForKey(key)) {
   api.pushinteger(state, cached);
   return 1;
  }
 }
 api.getfield(state, 1, "quads");
 if(api.type(state, -1) != kLuaTTable) {
  api.settop(state, 1);
  api.pushnil(state);
  api.pushstring(state, "model.build requires a quads array");
  return 2;
 }
 const int quadsIndex = api.gettop(state);
 const std::size_t quadCount = api.rawlen(state, quadsIndex);
 auto baked = std::make_unique<model::BakedModel>();
 const auto batchFor = [&](const std::string& texture) -> std::vector<model::BakedQuad>& {
  for(model::BakedTextureBatch& batch : baked->batches) {
   if(batch.texturePath == texture) {
    return batch.quads;
   }
  }
  model::BakedTextureBatch& batch = baked->batches.emplace_back();
  batch.texturePath = texture;
  // Same rule as the JSON baker: a named texture becomes a registry id so the
  // block/item draw paths can bind and uv-map it; untextured batches keep -1.
  batch.textureId =
      texture.empty() ? -1 : net::minecraft::registry::TextureRegistry::getOrRegisterTexture(texture);
  return batch.quads;
 };
 for(std::size_t qi = 1; qi <= quadCount; ++qi) {
  api.rawgeti(state, quadsIndex, static_cast<long long>(qi));
  const int quadIndex = api.gettop(state);
  if(api.type(state, quadIndex) == kLuaTTable) {
   const std::string texture = luaStringField(state, quadIndex, "texture", "");
   model::BakedQuad quad;
   quad.shade = std::clamp(luaFloatField(state, quadIndex, "shade", 1.0f), 0.0f, 1.0f);
   quad.red = std::clamp(luaFloatField(state, quadIndex, "r", 1.0f), 0.0f, 1.0f);
   quad.green = std::clamp(luaFloatField(state, quadIndex, "g", 1.0f), 0.0f, 1.0f);
   quad.blue = std::clamp(luaFloatField(state, quadIndex, "b", 1.0f), 0.0f, 1.0f);
   quad.alpha = std::clamp(luaFloatField(state, quadIndex, "a", 1.0f), 0.0f, 1.0f);
   api.getfield(state, quadIndex, "vertices");
   bool ok = false;
   if(api.type(state, -1) == kLuaTTable) {
    const int verticesIndex = api.gettop(state);
    if(api.rawlen(state, verticesIndex) >= 4) {
     ok = true;
     for(int vi = 0; vi < 4; ++vi) {
      api.rawgeti(state, verticesIndex, vi + 1);
      const int vtxIndex = api.gettop(state);
      if(api.type(state, vtxIndex) == kLuaTTable) {
       readBuildVertex(state, vtxIndex, quad.vertices[vi]);
      } else {
       ok = false;
      }
      api.settop(state, verticesIndex);
     }
    }
   }
   api.settop(state, quadIndex);
   if(ok) {
    batchFor(texture).push_back(quad);
   }
  }
  api.settop(state, quadsIndex);
 }
 api.settop(state, 1);
 if(baked->batches.empty()) {
  api.pushnil(state);
  api.pushstring(state, "model.build requires at least one quad");
  return 2;
 }
 model::computeBakedBounds(*baked);
 // Keyless builds still need a unique registry key so their handles never alias.
 static std::atomic<std::uint64_t> anonCounter{0};
 const std::string storeKey = key.empty() ? ("\x01build#" + std::to_string(anonCounter.fetch_add(1))) : key;
 api.pushinteger(state, model::storeBakedModel(storeKey, std::move(baked)));
 return 1;
}
} // namespace
void installModelApi(lua_State* state, ModHost::LoadedLuaMod& mod) {
 LuaApi& api = luaApi();
 api.createtable(state, 0, 10);
 bindModFunction(state, &mod, "load", luaModelLoad);
 bindModFunction(state, &mod, "place", luaModelPlace);
 bindModFunction(state, &mod, "clear", luaModelClear);
 bindFunctions(state,
               {
                   {"draw", luaModelDraw},
                   {"draw_item", luaModelDrawItem},
                   {"item_bounds", luaModelItemBounds},
                   {"build", luaModelBuild},
                   {"update", luaModelUpdate},
                   {"remove", luaModelRemove},
                   {"bounds", luaModelBounds},
               });
 api.setfield(state, -2, "model");
}
} // namespace net::minecraft::mod::runtime
