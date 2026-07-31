#include "net/minecraft/mod/model/JsonModelParser.hpp"
#include <algorithm>
#include <filesystem>
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include "net/minecraft/mod/runtime/ModHost.hpp"
namespace net::minecraft::mod::model::detail {
namespace {
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
  if(!readVec(uv, out.uv.data(), 4)) {
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
 readVec(value["origin"], out.origin.data(), 3);
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
 if(!readVec(value["from"], out.from.data(), 3) || !readVec(value["to"], out.to.data(), 3)) {
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
} // namespace
const JsonModelTexture* findTexture(const JsonModel& model, const std::string& key) noexcept {
 for(const JsonModelTexture& texture : model.textures) {
  if(texture.name == key) {
   return &texture;
  }
 }
 return nullptr;
}
bool parseJsonModel(const JsonValue& root, JsonModel& out, std::string& error) {
 if(!root.isObject()) {
  error = "model root must be an object";
  return false;
 }
 out.parent = root["parent"].asString();
 if(readVec(root["texture_size"], out.textureSize.data(), 2) && out.textureSize[0] > 0.0 && out.textureSize[1] > 0.0) {
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
void mergeParentModel(JsonModel& child, const JsonModel& parent) {
 if(!child.hasTextureSize && parent.hasTextureSize) {
  for(JsonModelElement& element : child.elements) {
   element.uvWidth = parent.textureSize[0];
   element.uvHeight = parent.textureSize[1];
  }
  child.textureSize = parent.textureSize;
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
  element.uvWidth = out.textureSize[0];
  element.uvHeight = out.textureSize[1];
 }
 return true;
}
} // namespace net::minecraft::mod::model::detail
