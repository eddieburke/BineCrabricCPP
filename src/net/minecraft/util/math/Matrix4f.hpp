#pragma once
#include <cmath>
#include <cstring>
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
  for(int col = 0; col < 4; ++col)
   for(int row = 0; row < 4; ++row) {
    float sum = 0.0f;
    for(int k = 0; k < 4; ++k)
     sum += m[k * 4 + row] * r.m[col * 4 + k];
    tmp[col * 4 + row] = sum;
   }
  std::memcpy(m, tmp, sizeof(m));
  return *this;
 }
 Matrix4f operator*(const Matrix4f& r) const {
  Matrix4f result = *this;
  result.multiply(r);
  return result;
 }
 void translate(float x, float y, float z) {
  Matrix4f t;
  t.m[12] = x;
  t.m[13] = y;
  t.m[14] = z;
  multiply(t);
 }
 void scale(float x, float y, float z) {
  Matrix4f s;
  s.m[0] = x;
  s.m[5] = y;
  s.m[10] = z;
  multiply(s);
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
 void perspective(float fovY, float aspect, float zNear, float zFar) {
  float f = 1.0f / std::tan(fovY * 3.14159265f / 360.0f);
  identity();
  m[0] = f / aspect;
  m[5] = f;
  m[10] = (zFar + zNear) / (zNear - zFar);
  m[11] = -1.0f;
  m[14] = (2.0f * zFar * zNear) / (zNear - zFar);
  m[15] = 0.0f;
 }
 void invert();
 void transpose() {
  for(int i = 0; i < 4; ++i)
   for(int j = i + 1; j < 4; ++j)
    std::swap(m[i * 4 + j], m[j * 4 + i]);
 }
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
};
} // namespace net::minecraft::util::math