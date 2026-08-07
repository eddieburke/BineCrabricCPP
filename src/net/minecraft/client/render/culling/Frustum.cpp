#include "net/minecraft/client/render/culling/Frustum.hpp"
#if defined(_MSC_VER)
#include <intrin.h>
#else
#include <x86intrin.h>
#endif
#include <array>
#include "net/minecraft/client/gl/GlConstants.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
namespace net::minecraft::client::render {
void Frustum::normalize(float plane[4]) {
#ifdef __SSE3__
 __m128 v = _mm_loadu_ps(plane);
 __m128 sq = _mm_mul_ps(v, v);
 const __m128i mask = _mm_setr_epi32(-1, -1, -1, 0);
 sq = _mm_and_ps(sq, _mm_castsi128_ps(mask));
 __m128 hsum = _mm_hadd_ps(sq, sq);
 hsum = _mm_hadd_ps(hsum, hsum);
 const float lenSq = _mm_cvtss_f32(hsum);
 if(lenSq == 0.0f) {
  return;
 }
 const __m128 invLen = _mm_set1_ps(1.0f / MathHelper::sqrt(lenSq));
 v = _mm_mul_ps(v, invLen);
 _mm_storeu_ps(plane, v);
#else
 const float length = MathHelper::sqrt(plane[0] * plane[0] + plane[1] * plane[1] + plane[2] * plane[2]);
 if(length == 0.0f) {
  return;
 }
 plane[0] /= length;
 plane[1] /= length;
 plane[2] /= length;
 plane[3] /= length;
#endif
}
void Frustum::compute(const net::minecraft::util::math::Matrix4f& projection,
                      const net::minecraft::util::math::Matrix4f& modelView) {
 const net::minecraft::util::math::Matrix4f clip = projection * modelView;
 struct Extraction {
  int row;
  float sign;
 };
 static constexpr Extraction kExtractions[6] = {
     {0, -1.0f},
     {0, +1.0f},
     {1, +1.0f},
     {1, -1.0f},
     {2, -1.0f},
     {2, +1.0f},
 };
 planes_.setCount(6);
 for(std::size_t i = 0; i < 6; ++i) {
  const int row = kExtractions[i].row;
  const float sign = kExtractions[i].sign;
  std::array<float, 4>& plane = planes_.plane(i);
  plane[0] = clip.m[3] + sign * clip.m[static_cast<std::size_t>(row)];
  plane[1] = clip.m[7] + sign * clip.m[static_cast<std::size_t>(4 + row)];
  plane[2] = clip.m[11] + sign * clip.m[static_cast<std::size_t>(8 + row)];
  plane[3] = clip.m[15] + sign * clip.m[static_cast<std::size_t>(12 + row)];
  normalize(plane.data());
 }
}
} // namespace net::minecraft::client::render
