#pragma once
#include <string>
#include <vector>
#include "net/minecraft/mod/model/JsonModelTypes.hpp"
namespace net::minecraft::mod::model::detail {
bool bakeJsonModel(const JsonModel& model, const std::string& basePath, BakedModel& out, std::string& error);
std::vector<BakedQuad>& batchFor(BakedModel& model, const std::string& texturePath);
void rotatePoint(double* point, const double* origin, char axis, double angleDeg);
void rescalePoint(double* point, const double* origin, char axis, double angleDeg);
} // namespace net::minecraft::mod::model::detail
