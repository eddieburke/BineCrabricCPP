#pragma once
#include <algorithm>
#include <cstddef>
#include <memory>
#include <vector>
#include "net/minecraft/util/math/noise/SimplexNoiseSampler.hpp"
namespace net::minecraft {
class OctaveSimplexNoiseSampler {
 public:
 OctaveSimplexNoiseSampler(JavaRandom& random, int octaves) {
  auto samplers = std::make_shared<std::vector<SimplexNoiseSampler>>();
  samplers->reserve(static_cast<std::size_t>(octaves));
  for(int i = 0; i < octaves; ++i) {
   samplers->emplace_back(random);
  }
  samplers_ = std::move(samplers);
 }
 std::vector<double>& sample(std::vector<double>& map,
                             double x,
                             double z,
                             int width,
                             int depth,
                             double frequencyX,
                             double frequencyZ,
                             double persistence,
                             double lacunarity = 0.5) const {
  frequencyX /= 1.5;
  frequencyZ /= 1.5;
  const std::size_t size = static_cast<std::size_t>(width * depth);
  if(map.size() < size) {
   map.assign(size, 0.0);
  } else {
   std::fill(map.begin(), map.end(), 0.0);
  }
  double amplitude = 1.0;
  double frequency = 1.0;
  for(const SimplexNoiseSampler& sampler : *samplers_) {
   sampler.create(map, x, z, width, depth, frequencyX * frequency, frequencyZ * frequency, 0.55 / amplitude);
   frequency *= persistence;
   amplitude *= lacunarity;
  }
  return map;
 }

 private:
 std::shared_ptr<const std::vector<SimplexNoiseSampler>> samplers_;
};
} // namespace net::minecraft
