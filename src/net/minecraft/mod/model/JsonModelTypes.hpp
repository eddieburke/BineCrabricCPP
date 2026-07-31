#pragma once
#include <array>
#include <string>
#include <vector>
#include "net/minecraft/mod/model/ModModels.hpp"
namespace net::minecraft::mod::model::detail {
struct JsonModelFaceSpec {
 bool present = false;
 bool hasUv = false;
 std::array<double, 4> uv = {0.0, 0.0, 16.0, 16.0};
 std::string texture;
 int rotation = 0;
 int tintIndex = -1;
 int cullFace = -1;
};
struct JsonModelRotationSpec {
 char axis = 0;
 double angle = 0.0;
 double x = 0.0;
 double y = 0.0;
 double z = 0.0;
 std::array<double, 3> origin = {8.0, 8.0, 8.0};
 bool rescale = false;
};
struct JsonModelElement {
 std::array<double, 3> from = {0.0, 0.0, 0.0};
 std::array<double, 3> to = {16.0, 16.0, 16.0};
 std::string basePath;
 double uvWidth = 16.0;
 double uvHeight = 16.0;
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
 std::array<double, 2> textureSize = {16.0, 16.0};
 bool hasTextureSize = false;
};
} // namespace net::minecraft::mod::model::detail
