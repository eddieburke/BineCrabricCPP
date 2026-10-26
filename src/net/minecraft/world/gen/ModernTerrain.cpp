#include "net/minecraft/world/gen/ModernTerrain.hpp"
#include <algorithm>
#include <array>
#include <cmath>
namespace net::minecraft::world::gen {
namespace {
constexpr int continentalnessOctaves = 6;
constexpr int erosionOctaves = 5;
constexpr int weirdnessOctaves = 5;
constexpr int riverOctaves = 4;
constexpr int detailOctaves = 4;
constexpr double continentalnessScale = 0.08;
constexpr double erosionScale = 0.1;
constexpr double weirdnessScale = 0.16;
constexpr double riverScale = 0.067;
constexpr double detailScaleXZ = 0.8;
constexpr double detailScaleY = 1.6;
constexpr double climateGain = 2.8;
constexpr double continentalnessGain = 3.6;
constexpr double continentalnessBias = 0.18;
constexpr double detailGain = 2.0;
constexpr double riverHalfWidth = 0.05;
constexpr double riverBedHeight = 59.0;
constexpr double densityPerBlock = 0.1;
constexpr double topFadeStart = 13.0;
double smoothstep(double edge0, double edge1, double value) {
 const double t = std::clamp((value - edge0) / (edge1 - edge0), 0.0, 1.0);
 return t * t * (3.0 - 2.0 * t);
}
double lerp(double a, double b, double t) {
 return a + (b - a) * t;
}
double normalize(double value, int octaves, double gain) {
 const double total = static_cast<double>((1 << octaves) - 1);
 return std::clamp(value / total * gain, -1.0, 1.0);
}
struct HeightPoint {
 double continentalness;
 double height;
};
constexpr std::array<HeightPoint, 10> baseCurve{{
    {-1.00, 26.0},
    {-0.60, 34.0},
    {-0.40, 46.0},
    {-0.25, 57.0},
    {-0.12, 62.0},
    {-0.05, 65.5},
    {0.05, 67.0},
    {0.25, 72.0},
    {0.50, 80.0},
    {1.00, 90.0},
}};
} // namespace
ModernTerrain::ModernTerrain(std::uint64_t seed)
    : random_(seed ^ 0x4D6F6465726EULL),
      continentalness_(random_, continentalnessOctaves),
      erosion_(random_, erosionOctaves),
      weirdness_(random_, weirdnessOctaves),
      river_(random_, riverOctaves),
      detail_(random_, detailOctaves) {
}
double ModernTerrain::baseHeight(double continentalness) {
 const double c = std::clamp(continentalness, -1.0, 1.0);
 for(std::size_t i = 1; i < baseCurve.size(); ++i) {
  if(c <= baseCurve[i].continentalness) {
   const HeightPoint& lo = baseCurve[i - 1];
   const HeightPoint& hi = baseCurve[i];
   return lerp(lo.height, hi.height, smoothstep(lo.continentalness, hi.continentalness, c));
  }
 }
 return baseCurve.back().height;
}
double ModernTerrain::mountainAmplitude(double continentalness, double erosion) {
 const double inland = smoothstep(0.0, 0.45, continentalness);
 return lerp(34.0, 3.0, smoothstep(-0.5, 0.7, erosion)) * inland;
}
double ModernTerrain::peaksAndValleys(double weirdness) {
 return 1.0 - std::abs(3.0 * std::abs(weirdness) - 2.0);
}
double ModernTerrain::mountainness(double surfaceHeight) {
 return smoothstep(72.0, 104.0, surfaceHeight);
}
double ModernTerrain::surfaceHeight(const Climate& climate) {
 const double base = baseHeight(climate.continentalness);
 const double amplitude = mountainAmplitude(climate.continentalness, climate.erosion);
 const double shape = peaksAndValleys(climate.weirdness);
 const double ridge = shape > 0.0 ? std::pow(shape, 1.5) : 0.0;
 const double valley = shape < 0.0 ? shape : 0.0;
 double height = base + amplitude * (ridge + valley * 0.35);
 if(height > 100.0) {
  height = 100.0 + (height - 100.0) * 0.6;
 }
 const double land = smoothstep(-0.12, -0.02, climate.continentalness);
 const double riverCut = (1.0 - smoothstep(0.0, riverHalfWidth, std::abs(climate.river))) * land;
 if(riverCut > 0.0 && height > riverBedHeight) {
  height = lerp(height, riverBedHeight, riverCut);
 }
 return height;
}
void ModernTerrain::fill(std::vector<double>& density, int cellX, int cellZ) {
 const std::size_t columns = static_cast<std::size_t>(sizeX * sizeZ);
 const std::size_t total = columns * static_cast<std::size_t>(sizeY);
 if(density.size() < total) {
  density.assign(total, 0.0);
 }
 continentalnessBuffer_ =
     continentalness_.create(continentalnessBuffer_, cellX, cellZ, sizeX, sizeZ, continentalnessScale, continentalnessScale, 0.5);
 erosionBuffer_ = erosion_.create(erosionBuffer_, cellX, cellZ, sizeX, sizeZ, erosionScale, erosionScale, 0.5);
 weirdnessBuffer_ =
     weirdness_.create(weirdnessBuffer_, cellX, cellZ, sizeX, sizeZ, weirdnessScale, weirdnessScale, 0.5);
 riverBuffer_ = river_.create(riverBuffer_, cellX, cellZ, sizeX, sizeZ, riverScale, riverScale, 0.5);
 detailBuffer_ = detail_.create(
     detailBuffer_, cellX, 0, cellZ, sizeX, sizeY, sizeZ, detailScaleXZ, detailScaleY, detailScaleXZ);
 for(std::size_t column = 0; column < columns; ++column) {
  Climate climate;
  climate.continentalness = std::clamp(normalize(continentalnessBuffer_[column], continentalnessOctaves, continentalnessGain) + continentalnessBias, -1.0, 1.0);
  climate.erosion = normalize(erosionBuffer_[column], erosionOctaves, climateGain);
  climate.weirdness = normalize(weirdnessBuffer_[column], weirdnessOctaves, climateGain);
  climate.river = normalize(riverBuffer_[column], riverOctaves, climateGain);
  const double height = surfaceHeight(climate);
  const double detailAmplitude = 0.6 + 1.6 * mountainness(height);
  for(int iy = 0; iy < sizeY; ++iy) {
   const std::size_t index = column * static_cast<std::size_t>(sizeY) + static_cast<std::size_t>(iy);
   const double detail = normalize(detailBuffer_[index], detailOctaves, detailGain);
   double value = (height - static_cast<double>(iy) * 8.0) * densityPerBlock + detail * detailAmplitude;
   if(static_cast<double>(iy) > topFadeStart) {
    const double fade = (static_cast<double>(iy) - topFadeStart) / 3.0;
    value = value * (1.0 - fade) - 10.0 * fade;
   }
   density[index] = value;
  }
 }
}
} // namespace net::minecraft::world::gen
