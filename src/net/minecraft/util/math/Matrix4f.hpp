#pragma once
#include <cmath>
#include <cstring>
#if defined(__SSE2__) || defined(_MSC_VER)
#if defined(_MSC_VER)
#include <intrin.h>
#else
#include <x86intrin.h>
#endif
#endif
namespace net::minecraft::util::math {
struct Matrix4f {
 float m[16]{};
 Matrix4f() {
  identity();
 }
 void identity() {
  std::memset(m, 0, sizeof(m));
  m[0] = m[5] = m[10] = m[15] = 1.0f;
 }
 void set(const float* src) {
  std::memcpy(m, src, sizeof(m));
 }
 const float* data() const {
  return m;
 }
 float* data() {
  return m;
 }
 Matrix4f& multiply(const Matrix4f& r) {
  float tmp[16]{};
#if defined(__SSE2__) || defined(_MSC_VER)
  for(int col = 0; col < 4; ++col) {
   __m128 acc = _mm_setzero_ps();
   for(int k = 0; k < 4; ++k)
    acc = _mm_add_ps(acc, _mm_mul_ps(_mm_loadu_ps(&m[k * 4]), _mm_set1_ps(r.m[col * 4 + k])));
   _mm_storeu_ps(&tmp[col * 4], acc);
  }
#else
  for(int col = 0; col < 4; ++col)
   for(int row = 0; row < 4; ++row) {
    float sum = 0.0f;
    for(int k = 0; k < 4; ++k)
     sum += m[k * 4 + row] * r.m[col * 4 + k];
    tmp[col * 4 + row] = sum;
   }
#endif
  std::memcpy(m, tmp, sizeof(m));
  return *this;
 }
 Matrix4f operator*(const Matrix4f& r) const {
  Matrix4f result = *this;
  result.multiply(r);
  return result;
 }
 void translate(float x, float y, float z) {
  m[12] += x * m[0] + y * m[4] + z * m[8];
  m[13] += x * m[1] + y * m[5] + z * m[9];
  m[14] += x * m[2] + y * m[6] + z * m[10];
 }
 void scale(float x, float y, float z) {
  m[0] *= x;
  m[1] *= x;
  m[2] *= x;
  m[4] *= y;
  m[5] *= y;
  m[6] *= y;
  m[8] *= z;
  m[9] *= z;
  m[10] *= z;
 }
 void rotate(float deg, float ax, float ay, float az) {
  float rad = deg * 3.14159265f / 180.0f;
  float c = std::cos(rad), s = std::sin(rad);
  float len = std::sqrt(ax * ax + ay * ay + az * az);
  if(len < 1e-6f)
   return;
  float x = ax / len, y = ay / len, z = az / len;
  float ic = 1.0f - c;
  Matrix4f r;
  r.m[0] = x * x * ic + c;
  r.m[4] = x * y * ic - z * s;
  r.m[8] = x * z * ic + y * s;
  r.m[1] = y * x * ic + z * s;
  r.m[5] = y * y * ic + c;
  r.m[9] = y * z * ic - x * s;
  r.m[2] = z * x * ic - y * s;
  r.m[6] = z * y * ic + x * s;
  r.m[10] = z * z * ic + c;
  multiply(r);
 }
 void ortho(float l, float r, float b, float t, float n, float f) {
  identity();
  m[0] = 2.0f / (r - l);
  m[5] = 2.0f / (t - b);
  m[10] = -2.0f / (f - n);
  m[12] = -(r + l) / (r - l);
  m[13] = -(t + b) / (t - b);
  m[14] = -(f + n) / (f - n);
 }
 void invert();
 static Matrix4f identityMatrix() {
  Matrix4f mat;
  mat.identity();
  return mat;
 }
 void transformPoint(float x, float y, float z, float& outX, float& outY, float& outZ) const {
  outX = m[0] * x + m[4] * y + m[8] * z + m[12];
  outY = m[1] * x + m[5] * y + m[9] * z + m[13];
  outZ = m[2] * x + m[6] * y + m[10] * z + m[14];
 }
 void transformDirection(float x, float y, float z, float& outX, float& outY, float& outZ) const {
  outX = m[0] * x + m[4] * y + m[8] * z;
  outY = m[1] * x + m[5] * y + m[9] * z;
  outZ = m[2] * x + m[6] * y + m[10] * z;
 }
};
} // namespace net::minecraft::util::math
