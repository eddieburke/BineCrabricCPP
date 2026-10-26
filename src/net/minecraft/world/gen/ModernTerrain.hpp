#pragma once
#include <cstdint>
#include <vector>
#include "net/minecraft/util/math/Types.hpp"
#include "net/minecraft/util/math/noise/OctavePerlinNoiseSampler.hpp"
namespace net::minecraft::world::gen {
class ModernTerrain {
 public:
 static constexpr int sizeX = 5;
 static constexpr int sizeY = 17;
 static constexpr int sizeZ = 5;
 static constexpr double seaLevel = 64.0;
 struct Climate {
  double continentalness = 0.0;
  double erosion = 0.0;
  double weirdness = 0.0;
  double river = 1.0;
 };
 explicit ModernTerrain(std::uint64_t seed);
 void fill(std::vector<double>& density, int cellX, int cellZ);
 [[nodiscard]] static double baseHeight(double continentalness);
 [[nodiscard]] static double mountainAmplitude(double continentalness, double erosion);
 [[nodiscard]] static double peaksAndValleys(double weirdness);
 [[nodiscard]] static double surfaceHeight(const Climate& climate);
 [[nodiscard]] static double mountainness(double surfaceHeight);

 private:
 JavaRandom random_;
 OctavePerlinNoiseSampler continentalness_;
 OctavePerlinNoiseSampler erosion_;
 OctavePerlinNoiseSampler weirdness_;
 OctavePerlinNoiseSampler river_;
 OctavePerlinNoiseSampler detail_;
 std::vector<double> continentalnessBuffer_;
 std::vector<double> erosionBuffer_;
 std::vector<double> weirdnessBuffer_;
 std::vector<double> riverBuffer_;
 std::vector<double> detailBuffer_;
};
} // namespace net::minecraft::world::gen
