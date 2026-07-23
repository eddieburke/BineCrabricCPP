#pragma once
#include <array>
#include <cstddef>
#include <vector>
#include "net/minecraft/util/math/Types.hpp"
namespace net::minecraft {
class SimplexNoiseSampler {
 public:
 explicit SimplexNoiseSampler(JavaRandom& random) {
  offsetX_ = random.nextDouble() * 256.0;
  offsetY_ = random.nextDouble() * 256.0;
  offsetZ_ = random.nextDouble() * 256.0;
  for(int i = 0; i < 256; ++i) {
   perm_[i] = i;
  }
  for(int i = 0; i < 256; ++i) {
   const int swapIndex = random.nextInt(256 - i) + i;
   std::swap(perm_[i], perm_[swapIndex]);
   perm_[i + 256] = perm_[i];
  }
 }
 void create(std::vector<double>& map,
             double x,
             double z,
             int width,
             int depth,
             double frequencyX,
             double frequencyZ,
             double amplitude) const {
  int index = 0;
  for(int ix = 0; ix < width; ++ix) {
   const double sampleX = (x + static_cast<double>(ix)) * frequencyX + offsetX_;
   for(int iz = 0; iz < depth; ++iz) {
    const double sampleZ = (z + static_cast<double>(iz)) * frequencyZ + offsetY_;
    const double skew = (sampleX + sampleZ) * f2_;
    const int cellX = fastFloor(sampleX + skew);
    const int cellZ = fastFloor(sampleZ + skew);
    const double unskew = static_cast<double>(cellX + cellZ) * g2_;
    const double originX = static_cast<double>(cellX) - unskew;
    const double originZ = static_cast<double>(cellZ) - unskew;
    const double dx0 = sampleX - originX;
    const double dz0 = sampleZ - originZ;
    const int stepX = dx0 > dz0 ? 1 : 0;
    const int stepZ = dx0 > dz0 ? 0 : 1;
    const double dx1 = dx0 - static_cast<double>(stepX) + g2_;
    const double dz1 = dz0 - static_cast<double>(stepZ) + g2_;
    const double dx2 = dx0 - 1.0 + 2.0 * g2_;
    const double dz2 = dz0 - 1.0 + 2.0 * g2_;
    const int px = cellX & 0xFF;
    const int pz = cellZ & 0xFF;
    const int gi0 = perm_[px + perm_[pz]] % 12;
    const int gi1 = perm_[px + stepX + perm_[pz + stepZ]] % 12;
    const int gi2 = perm_[px + 1 + perm_[pz + 1]] % 12;
    map[static_cast<std::size_t>(index++)] +=
        70.0 * (corner(gi0, dx0, dz0) + corner(gi1, dx1, dz1) + corner(gi2, dx2, dz2)) * amplitude;
   }
  }
 }

 private:
 static int fastFloor(double value) {
  return value > 0.0 ? static_cast<int>(value) : static_cast<int>(value) - 1;
 }
 static double corner(int gradIndex, double x, double z) {
  static constexpr std::array<std::array<int, 3>, 12> grads{{{{1, 1, 0}},
                                                             {{-1, 1, 0}},
                                                             {{1, -1, 0}},
                                                             {{-1, -1, 0}},
                                                             {{1, 0, 1}},
                                                             {{-1, 0, 1}},
                                                             {{1, 0, -1}},
                                                             {{-1, 0, -1}},
                                                             {{0, 1, 1}},
                                                             {{0, -1, 1}},
                                                             {{0, 1, -1}},
                                                             {{0, -1, -1}}}};
  double t = 0.5 - x * x - z * z;
  if(t < 0.0) {
   return 0.0;
  }
  t *= t;
  return t * t *
         (static_cast<double>(grads[static_cast<std::size_t>(gradIndex)][0]) * x +
          static_cast<double>(grads[static_cast<std::size_t>(gradIndex)][1]) * z);
 }
 static constexpr double f2_ = 0.5 * 0.73205080756887729353;
 static constexpr double g2_ = (3.0 - 1.73205080756887729353) / 6.0;
 std::array<int, 512> perm_{};
 double offsetX_ = 0.0;
 double offsetY_ = 0.0;
 double offsetZ_ = 0.0;
};
} // namespace net::minecraft
