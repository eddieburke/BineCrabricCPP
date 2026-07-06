#pragma once
#include "net/minecraft/client/render/culling/FrustumData.hpp"
namespace net::minecraft::client::render {
class Frustum : public FrustumData {
public:
  static Frustum& getInstance();
  void compute();

private:
  static void normalize(float plane[4]);
};
} // namespace net::minecraft::client::render
