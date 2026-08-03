#include "net/minecraft/mod/model/JsonModelBaker.hpp"
#include <array>
#include <cmath>
#include <set>
#include <utility>
#include "net/minecraft/mod/model/JsonModelParser.hpp"
#include "net/minecraft/registry/TextureRegistry.hpp"
namespace net::minecraft::mod::model::detail {
namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr int kFaceCorners[kModelFaceCount][4][3] = {
    {{0, 0, 1}, {0, 0, 0}, {1, 0, 0}, {1, 0, 1}},
    {{0, 1, 0}, {0, 1, 1}, {1, 1, 1}, {1, 1, 0}},
    {{1, 1, 0}, {1, 0, 0}, {0, 0, 0}, {0, 1, 0}},
    {{0, 1, 1}, {0, 0, 1}, {1, 0, 1}, {1, 1, 1}},
    {{0, 1, 0}, {0, 0, 0}, {0, 0, 1}, {0, 1, 1}},
    {{1, 1, 1}, {1, 0, 1}, {1, 0, 0}, {1, 1, 0}},
};
constexpr float kFaceShade[kModelFaceCount] = {0.5f, 1.0f, 0.8f, 0.8f, 0.6f, 0.6f};
constexpr int kFaceAxis[3] = {1, 2, 0};
struct UvRect {
 double uMin = 0.0;
 double vMin = 0.0;
 double uMax = 16.0;
 double vMax = 16.0;
};
struct UvPoint {
 double u = 0.0;
 double v = 0.0;
};
UvRect defaultUvFor(ModelFace face, const std::array<double, 3>& from, const std::array<double, 3>& to) {
 UvRect uv;
 switch(face) {
 case ModelFace::Down:
  uv = {from[0], 16.0 - to[2], to[0], 16.0 - from[2]};
  break;
 case ModelFace::Up:
  uv = {from[0], from[2], to[0], to[2]};
  break;
 case ModelFace::North:
  uv = {16.0 - to[0], 16.0 - to[1], 16.0 - from[0], 16.0 - from[1]};
  break;
 case ModelFace::South:
  uv = {from[0], 16.0 - to[1], to[0], 16.0 - from[1]};
  break;
 case ModelFace::West:
  uv = {from[2], 16.0 - to[1], to[2], 16.0 - from[1]};
  break;
 case ModelFace::East:
  uv = {16.0 - to[2], 16.0 - to[1], 16.0 - from[2], 16.0 - from[1]};
  break;
 }
 return uv;
}
UvRect faceUv(const JsonModelFaceSpec& face, ModelFace modelFace, const std::array<double, 3>& from,
              const std::array<double, 3>& to) {
 if(face.hasUv) {
  return {face.uv[0], face.uv[1], face.uv[2], face.uv[3]};
 }
 return defaultUvFor(modelFace, from, to);
}
std::array<UvPoint, 4> rotatedUvCorners(const UvRect& uv, int rotation) {
 const std::array<UvPoint, 4> corners = {
     UvPoint{uv.uMin, uv.vMin}, UvPoint{uv.uMin, uv.vMax}, UvPoint{uv.uMax, uv.vMax}, UvPoint{uv.uMax, uv.vMin}};
 std::array<UvPoint, 4> rotated;
 const int shift = rotation / 90;
 for(int index = 0; index < 4; ++index) {
  rotated[index] = corners[(index + shift) % 4];
 }
 return rotated;
}
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
int boundaryCullFace(const JsonModelElement& element, int faceIndex) {
 if(element.hasRotation) {
  return -1;
 }
 const int axis = kFaceAxis[faceIndex / 2];
 const bool negative = (faceIndex % 2) == 0;
 const double coordinate = negative ? element.from[axis] : element.to[axis];
 return coordinate == (negative ? 0.0 : 16.0) ? faceIndex : -1;
}
bool isSolidElement(const JsonModelElement& element) noexcept {
 return element.from[0] < element.to[0] && element.from[1] < element.to[1] && element.from[2] < element.to[2];
}
bool isCoplanarBackFace(const JsonModelElement& element, int faceIndex) {
 if((faceIndex % 2) == 0) {
  return false;
 }
 const int axis = kFaceAxis[faceIndex / 2];
 if(element.from[axis] != element.to[axis]) {
  return false;
 }
 const JsonModelFaceSpec& opposite = element.faces[faceIndex - 1];
 return opposite.present && opposite.texture == element.faces[faceIndex].texture;
}
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
} // namespace
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
std::vector<BakedQuad>& batchFor(BakedModel& model, const std::string& texturePath) {
 for(BakedTextureBatch& batch : model.batches) {
  if(batch.texturePath == texturePath) {
   return batch.quads;
  }
 }
 BakedTextureBatch& batch = model.batches.emplace_back();
 batch.texturePath = texturePath;
 batch.textureId = texturePath.empty() ? -1 : net::minecraft::registry::TextureRegistry::getOrRegisterTexture(texturePath);
 return batch.quads;
}
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
   if(isCoplanarBackFace(element, faceIndex)) {
    continue;
   }
   const ModelFace modelFace = static_cast<ModelFace>(faceIndex);
   const std::array<UvPoint, 4> uvCorners =
       rotatedUvCorners(faceUv(face, modelFace, element.from, element.to), face.rotation);
   BakedQuad quad;
   quad.face = modelFace;
   quad.cullFace = face.cullFace >= 0 ? face.cullFace : boundaryCullFace(element, faceIndex);
   quad.shade = element.shade ? kFaceShade[faceIndex] : 1.0f;
   quad.coplanarBackFace = false;
   quad.tintIndex = face.tintIndex;
   for(int i = 0; i < 4; ++i) {
    double point[3];
    for(int axis = 0; axis < 3; ++axis) {
     point[axis] = kFaceCorners[faceIndex][i][axis] != 0 ? element.to[axis] : element.from[axis];
    }
    if(element.hasRotation) {
     const JsonModelRotationSpec& rotation = element.rotation;
     if(rotation.axis != 0) {
      rotatePoint(point, rotation.origin.data(), rotation.axis, rotation.angle);
      if(rotation.rescale) {
       rescalePoint(point, rotation.origin.data(), rotation.axis, rotation.angle);
      }
     } else {
      rotatePoint(point, rotation.origin.data(), 'x', rotation.x);
      rotatePoint(point, rotation.origin.data(), 'y', rotation.y);
      rotatePoint(point, rotation.origin.data(), 'z', rotation.z);
     }
    }
    const UvPoint& corner = uvCorners[i];
    BakedVertex& vertex = quad.vertices[i];
    vertex.x = static_cast<float>(point[0] / 16.0);
    vertex.y = static_cast<float>(point[1] / 16.0);
    vertex.z = static_cast<float>(point[2] / 16.0);
    vertex.u = static_cast<float>(corner.u / element.uvWidth);
    vertex.v = static_cast<float>(corner.v / element.uvHeight);
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
} // namespace net::minecraft::mod::model::detail
