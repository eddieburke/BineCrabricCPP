#pragma once
#include "net/minecraft/util/math/Types.hpp"
#include "net/minecraft/world/gen/feature/Feature.hpp"
#include "net/minecraft/world/gen/feature/TreeFeatureHelpers.hpp"
namespace net::minecraft {
class World;
class BirchTreeFeature : public Feature {
 public:
 bool generate(World* world, JavaRandom& random, int x, int y, int z) override {
  const int height = random.nextInt(3) + 5;
  return tree_feature::generateRoundedTree(world, random, x, y, z, height, 2, 2);
 }
};
} // namespace net::minecraft
