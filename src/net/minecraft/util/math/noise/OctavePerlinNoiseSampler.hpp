#pragma once
#include <algorithm>
#include <cstddef>
#include <vector>
#include "net/minecraft/util/math/noise/PerlinNoiseSampler.hpp"
namespace net::minecraft {
class OctavePerlinNoiseSampler {
public:
  OctavePerlinNoiseSampler(JavaRandom& random, int octaves) {
    samplers_.reserve(static_cast<std::size_t>(octaves));
    for(int i = 0; i < octaves; ++i) {
      samplers_.emplace_back(random);
    }
  }
  [[nodiscard]] double sample(double x, double z) const {
    double value = 0.0;
    double frequency = 1.0;
    for(const PerlinNoiseSampler& sampler : samplers_) {
      value += sampler.sample(x * frequency, z * frequency) / frequency;
      frequency /= 2.0;
    }
    return value;
  }
  std::vector<double>& create(std::vector<double>& map,
                              double x,
                              double y,
                              double z,
                              int width,
                              int height,
                              int depth,
                              double scaleX,
                              double scaleY,
                              double scaleZ) const {
    const std::size_t size = static_cast<std::size_t>(width * height * depth);
    if(map.size() < size) {
      map.assign(size, 0.0);
    } else {
      std::fill(map.begin(), map.end(), 0.0);
    }
    double frequency = 1.0;
    for(const PerlinNoiseSampler& sampler : samplers_) {
      sampler.create(map,
                     x,
                     y,
                     z,
                     width,
                     height,
                     depth,
                     scaleX * frequency,
                     scaleY * frequency,
                     scaleZ * frequency,
                     frequency);
      frequency /= 2.0;
    }
    return map;
  }
  std::vector<double>& create(std::vector<double>& map,
                              int x,
                              int z,
                              int width,
                              int depth,
                              double scaleX,
                              double scaleZ,
                              double scaleUnused) const {
    (void)scaleUnused;
    return create(map, x, 10.0, z, width, 1, depth, scaleX, 1.0, scaleZ);
  }

private:
  std::vector<PerlinNoiseSampler> samplers_;
};
} // namespace net::minecraft
