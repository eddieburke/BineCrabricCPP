#pragma once
#include <algorithm>
#include <cmath>
namespace net::minecraft::util::math {
// Slab-method AABB ray intersection. Returns the entry distance t along
// origin + t*dir (clamped to [0, maxDist]) or -1.0 on miss.
// eps is the near-zero threshold for dir components.
[[nodiscard]] inline double raySlabIntersect(const double boxMin[3], const double boxMax[3],
                                             const double origin[3], const double dir[3],
                                             double maxDist, double eps = 1.0e-7) {
 double tMin = 0.0;
 double tMax = maxDist;
 for(int axis = 0; axis < 3; ++axis) {
  if(std::abs(dir[axis]) < eps) {
   if(origin[axis] < boxMin[axis] || origin[axis] > boxMax[axis]) {
    return -1.0;
   }
   continue;
  }
  double t1 = (boxMin[axis] - origin[axis]) / dir[axis];
  double t2 = (boxMax[axis] - origin[axis]) / dir[axis];
  if(t1 > t2) {
   std::swap(t1, t2);
  }
  tMin = std::max(tMin, t1);
  tMax = std::min(tMax, t2);
  if(tMin > tMax) {
   return -1.0;
  }
 }
 return tMin;
}
} // namespace net::minecraft::util::math
