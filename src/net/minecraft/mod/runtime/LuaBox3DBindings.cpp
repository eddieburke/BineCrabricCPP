#include "net/minecraft/mod/runtime/LuaBox3DBindings.hpp"
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
namespace net::minecraft::mod::runtime {
using namespace net::minecraft::mod::lua;
namespace {
constexpr double kEps = 1.0e-9;
constexpr double kInvSqrt3 = 0.5773502691896258;
struct Vec3 {
 double x = 0.0;
 double y = 0.0;
 double z = 0.0;
};
struct Quat {
 double x = 0.0;
 double y = 0.0;
 double z = 0.0;
 double w = 1.0;
};
struct Mat3 {
 double xx = 0.0;
 double yy = 0.0;
 double zz = 0.0;
 double xy = 0.0;
 double xz = 0.0;
 double yz = 0.0;
};
struct Aabb {
 double min_x = 0.0;
 double min_y = 0.0;
 double min_z = 0.0;
 double max_x = 0.0;
 double max_y = 0.0;
 double max_z = 0.0;
};
struct Obb {
 Vec3 center;
 Vec3 half;
 Vec3 axes[3];
};
struct Softness {
 double bias_rate;
 double mass_scale;
 double impulse_scale;
};
struct ContactPoint {
 Vec3 point;
 int id = 0;
 double separation = 0.0;
 double normal_impulse = 0.0;
 double tangent_impulse1 = 0.0;
 double tangent_impulse2 = 0.0;
};
struct Manifold {
 Vec3 normal;
 double separation = 0.0;
 int feature = 0;
 double friction = 0.0;
 double restitution = 0.0;
};
struct CachedManifold {
 Manifold base;
 std::vector<ContactPoint> contacts;
};
Vec3 v3Add(const Vec3& a, const Vec3& b) {
 return {a.x + b.x, a.y + b.y, a.z + b.z};
}
Vec3 v3Sub(const Vec3& a, const Vec3& b) {
 return {a.x - b.x, a.y - b.y, a.z - b.z};
}
Vec3 v3Scale(const Vec3& a, double s) {
 return {a.x * s, a.y * s, a.z * s};
}
Vec3 v3Neg(const Vec3& a) {
 return {-a.x, -a.y, -a.z};
}
double v3Dot(const Vec3& a, const Vec3& b) {
 return a.x * b.x + a.y * b.y + a.z * b.z;
}
Vec3 v3Cross(const Vec3& a, const Vec3& b) {
 return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
double v3LenSqr(const Vec3& a) {
 return v3Dot(a, a);
}
double v3Len(const Vec3& a) {
 return std::sqrt(v3LenSqr(a));
}
Vec3 v3Normalize(const Vec3& a) {
 const double ls = v3LenSqr(a);
 if(ls <= kEps * kEps) {
  return {0.0, 0.0, 0.0};
 }
 return v3Scale(a, 1.0 / std::sqrt(ls));
}
Vec3 v3Lerp(const Vec3& a, const Vec3& b, double t) {
 return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t};
}
Quat quatNormalize(const Quat& q) {
 const double ls = q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;
 if(ls <= kEps * kEps) {
  return {0.0, 0.0, 0.0, 1.0};
 }
 const double inv = 1.0 / std::sqrt(ls);
 return {q.x * inv, q.y * inv, q.z * inv, q.w * inv};
}
Quat quatMul(const Quat& a, const Quat& b) {
 return {
  a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
  a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
  a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
  a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z,
 };
}
Vec3 quatRotate(const Quat& q, const Vec3& p) {
 const double tx = 2.0 * (q.y * p.z - q.z * p.y);
 const double ty = 2.0 * (q.z * p.x - q.x * p.z);
 const double tz = 2.0 * (q.x * p.y - q.y * p.x);
 return {
  p.x + q.w * tx + q.y * tz - q.z * ty,
  p.y + q.w * ty + q.z * tx - q.x * tz,
  p.z + q.w * tz + q.x * ty - q.y * tx,
 };
}
Vec3 quatInvRotate(const Quat& q, const Vec3& p) {
 return quatRotate({-q.x, -q.y, -q.z, q.w}, p);
}
Quat quatIntegrate(const Quat& q, const Vec3& w, double h) {
 const double s = 0.5 * h;
 return quatNormalize({
  q.x + s * (w.x * q.w + w.y * q.z - w.z * q.y),
  q.y + s * (-w.x * q.z + w.y * q.w + w.z * q.x),
  q.z + s * (w.x * q.y - w.y * q.x + w.z * q.w),
  q.w - s * (w.x * q.x + w.y * q.y + w.z * q.z),
 });
}
Quat quatSlerp(const Quat& aIn, const Quat& bIn, double t) {
 double d = aIn.x * bIn.x + aIn.y * bIn.y + aIn.z * bIn.z + aIn.w * bIn.w;
 Quat b = bIn;
 if(d < 0) {
  b = {-bIn.x, -bIn.y, -bIn.z, -bIn.w};
  d = -d;
 }
 if(d > 0.9995) {
  return quatNormalize({
   aIn.x + (b.x - aIn.x) * t,
   aIn.y + (b.y - aIn.y) * t,
   aIn.z + (b.z - aIn.z) * t,
   aIn.w + (b.w - aIn.w) * t,
  });
 }
 d = std::max(-1.0, std::min(1.0, d));
 const double a0 = std::acos(d);
 const double sa0 = std::sin(a0);
 if(std::abs(sa0) <= kEps) {
  return aIn;
 }
 const double a1 = a0 * t;
 const double s0 = std::sin(a0 - a1) / sa0;
 const double s1 = std::sin(a1) / sa0;
 return {
  aIn.x * s0 + b.x * s1,
  aIn.y * s0 + b.y * s1,
  aIn.z * s0 + b.z * s1,
  aIn.w * s0 + b.w * s1,
 };
}
Softness makeSoft(double hertz, double damping_ratio, double h) {
 if(hertz <= 0.0 || h <= 0.0) {
  return {0.0, 1.0, 0.0};
 }
 const double omega = 2.0 * 3.14159265358979323846 * hertz;
 const double a1 = 2.0 * damping_ratio + h * omega;
 const double a2 = h * omega * a1;
 const double a3 = 1.0 / (1.0 + a2);
 return {omega / a1, a2 * a3, a3};
}
Mat3 invertSymmetric(const Mat3& m) {
 const double det = m.xx * (m.yy * m.zz - m.yz * m.yz) - m.xy * (m.xy * m.zz - m.yz * m.xz) +
                    m.xz * (m.xy * m.yz - m.yy * m.xz);
 if(std::abs(det) <= kEps) {
  return {};
 }
 const double inv = 1.0 / det;
 return {
  (m.yy * m.zz - m.yz * m.yz) * inv,
  (m.xx * m.zz - m.xz * m.xz) * inv,
  (m.xx * m.yy - m.xy * m.xy) * inv,
  (m.xz * m.yz - m.xy * m.zz) * inv,
  (m.xy * m.yz - m.yy * m.xz) * inv,
  (m.xy * m.xz - m.xx * m.yz) * inv,
 };
}
double quatToPitch(const Quat& q) {
 return std::asin(std::max(-1.0, std::min(1.0, 2.0 * (q.w * q.x - q.y * q.z))));
}
double quatToYaw(const Quat& q) {
 const double xx = q.x * q.x;
 const double yy = q.y * q.y;
 return std::atan2(2.0 * (q.x * q.z + q.w * q.y), 1.0 - 2.0 * (xx + yy));
}
double quatToRoll(const Quat& q) {
 const double xx = q.x * q.x;
 const double zz = q.z * q.z;
 return std::atan2(2.0 * (q.x * q.y + q.w * q.z), 1.0 - 2.0 * (xx + zz));
}
Vec3 readLuaV3(lua_State* state, int index) {
 LuaApi& api = luaApi();
 return {
  luaDoubleField(state, index, "x", 0.0),
  luaDoubleField(state, index, "y", 0.0),
  luaDoubleField(state, index, "z", 0.0),
 };
}
Quat readLuaQuat(lua_State* state, int index) {
 return {
  luaDoubleField(state, index, "x", 0.0),
  luaDoubleField(state, index, "y", 0.0),
  luaDoubleField(state, index, "z", 0.0),
  luaDoubleField(state, index, "w", 1.0),
 };
}
Mat3 readLuaMat3(lua_State* state, int index) {
 return {
  luaDoubleField(state, index, "xx", 0.0),
  luaDoubleField(state, index, "yy", 0.0),
  luaDoubleField(state, index, "zz", 0.0),
  luaDoubleField(state, index, "xy", 0.0),
  luaDoubleField(state, index, "xz", 0.0),
  luaDoubleField(state, index, "yz", 0.0),
 };
}
Aabb readLuaAabb(lua_State* state, int index) {
 return {
  luaDoubleField(state, index, "min_x", 0.0),
  luaDoubleField(state, index, "min_y", 0.0),
  luaDoubleField(state, index, "min_z", 0.0),
  luaDoubleField(state, index, "max_x", 0.0),
  luaDoubleField(state, index, "max_y", 0.0),
  luaDoubleField(state, index, "max_z", 0.0),
 };
}
void pushLuaV3(lua_State* state, const Vec3& v) {
 LuaApi& api = luaApi();
 api.createtable(state, 0, 3);
 setField(state, "x", v.x);
 setField(state, "y", v.y);
 setField(state, "z", v.z);
}
void pushLuaQuat(lua_State* state, const Quat& q) {
 LuaApi& api = luaApi();
 api.createtable(state, 0, 4);
 setField(state, "x", q.x);
 setField(state, "y", q.y);
 setField(state, "z", q.z);
 setField(state, "w", q.w);
}
void pushLuaAabb(lua_State* state, const Aabb& a) {
 LuaApi& api = luaApi();
 api.createtable(state, 0, 6);
 setField(state, "min_x", a.min_x);
 setField(state, "max_x", a.max_x);
 setField(state, "min_y", a.min_y);
 setField(state, "max_y", a.max_y);
 setField(state, "min_z", a.min_z);
 setField(state, "max_z", a.max_z);
}
double radiusOnAxis(const Obb& obb, const Vec3& axis) {
 return obb.half.x * std::abs(v3Dot(obb.axes[0], axis)) +
        obb.half.y * std::abs(v3Dot(obb.axes[1], axis)) +
        obb.half.z * std::abs(v3Dot(obb.axes[2], axis));
}
struct SatResult {
 Vec3 normal;
 double separation = 0.0;
 int kind = 0;
 int index = 0;
};
bool testSatAxis(const Obb& a, const Obb& b, const Vec3& delta,
                 const Vec3& raw_axis, int kind, int index, double margin,
                 SatResult* best) {
 const double ls = v3LenSqr(raw_axis);
 if(ls <= 1.0e-12) {
  return true;
 }
 const Vec3 axis = v3Scale(raw_axis, 1.0 / std::sqrt(ls));
 const double distance = v3Dot(delta, axis);
 const double separation = std::abs(distance) - radiusOnAxis(a, axis) - radiusOnAxis(b, axis);
 if(separation > margin) {
  return false;
 }
 Vec3 normal = distance < 0.0 ? v3Neg(axis) : axis;
 if(best == nullptr || separation > best->separation + 1.0e-5 ||
    (std::abs(separation - best->separation) <= 1.0e-5 && best->kind == 3 && kind != 3)) {
  if(best != nullptr) {
   best->normal = normal;
   best->separation = separation;
   best->kind = kind;
   best->index = index;
  }
 }
 return true;
}
const SatResult* satObb(const Obb& a, const Obb& b, double margin, SatResult& out) {
 const Vec3 delta = v3Sub(b.center, a.center);
 SatResult* best = nullptr;
 for(int i = 0; i < 3; ++i) {
  if(!testSatAxis(a, b, delta, a.axes[i], 1, i + 1, margin, best)) {
   return nullptr;
  }
  if(best == nullptr) {
   best = &out;
  }
 }
 for(int i = 0; i < 3; ++i) {
  if(!testSatAxis(a, b, delta, b.axes[i], 2, i + 1, margin, best)) {
   return nullptr;
  }
 }
 for(int i = 0; i < 3; ++i) {
  for(int j = 0; j < 3; ++j) {
   if(!testSatAxis(a, b, delta, v3Cross(a.axes[i], b.axes[j]), 3, i * 3 + j + 1, margin, best)) {
    return nullptr;
   }
  }
 }
 return best;
}
void tangentBasis(const Vec3& n, Vec3& t1, Vec3& t2) {
 Vec3 reference;
 if(std::abs(n.x) < kInvSqrt3) {
  reference = {1.0, 0.0, 0.0};
 } else if(std::abs(n.y) < kInvSqrt3) {
  reference = {0.0, 1.0, 0.0};
 } else {
  reference = {0.0, 0.0, 1.0};
 }
 t1 = v3Normalize(v3Cross(n, reference));
 t2 = v3Cross(n, t1);
}
Vec3 support(const Obb& obb, const Vec3& direction) {
 Vec3 p = obb.center;
 const double h[3] = {obb.half.x, obb.half.y, obb.half.z};
 for(int i = 0; i < 3; ++i) {
  const double sign = v3Dot(obb.axes[i], direction) >= 0.0 ? h[i] : -h[i];
  p.x += obb.axes[i].x * sign;
  p.y += obb.axes[i].y * sign;
  p.z += obb.axes[i].z * sign;
 }
 return p;
}
bool pointInsideObb(const Vec3& p, const Obb& obb, double tolerance) {
 const Vec3 d = v3Sub(p, obb.center);
 for(int i = 0; i < 3; ++i) {
  const double half = i == 0 ? obb.half.x : (i == 1 ? obb.half.y : obb.half.z);
  if(std::abs(v3Dot(d, obb.axes[i])) > half + tolerance) {
   return false;
  }
 }
 return true;
}
Obb makeBodyObb(const Vec3& position, const Vec3 bodyHalf, const Vec3 axes[3],
                const Vec3& com_offset) {
 const Vec3& ax = axes[0];
 const Vec3& ay = axes[1];
 const Vec3& az = axes[2];
 const double ox = com_offset.x;
 const double oy = com_offset.y;
 const double oz = com_offset.z;
 const double wx = ax.x * ox + ay.x * oy + az.x * oz;
 const double wy = ax.y * ox + ay.y * oy + az.y * oz;
 const double wz = ax.z * ox + ay.z * oy + az.z * oz;
 Obb obb;
 obb.center = {position.x - wx, position.y - wy, position.z - wz};
 obb.half = bodyHalf;
 obb.axes[0] = axes[0];
 obb.axes[1] = axes[1];
 obb.axes[2] = axes[2];
 return obb;
}
Obb makeAabbObb(const Aabb& aabb) {
 Obb obb;
 obb.center.x = 0.5 * (aabb.min_x + aabb.max_x);
 obb.center.y = 0.5 * (aabb.min_y + aabb.max_y);
 obb.center.z = 0.5 * (aabb.min_z + aabb.max_z);
 obb.half.x = 0.5 * (aabb.max_x - aabb.min_x);
 obb.half.y = 0.5 * (aabb.max_y - aabb.min_y);
 obb.half.z = 0.5 * (aabb.max_z - aabb.min_z);
 obb.axes[0] = {1.0, 0.0, 0.0};
 obb.axes[1] = {0.0, 1.0, 0.0};
 obb.axes[2] = {0.0, 0.0, 1.0};
 return obb;
}
CachedManifold buildCachedManifold(const Obb& a, const Obb& b, const SatResult& sat,
                                   double margin, bool single_contact) {
 CachedManifold result;
 result.base.normal = sat.normal;
 result.base.separation = sat.separation;
 const int feature_base = sat.kind == 1 ? 0 : (sat.kind == 2 ? 8 : 16);
 result.base.feature = feature_base + sat.index;
 const Vec3 n = sat.normal;
 if(single_contact) {
  const Vec3 pa = support(a, n);
  const Vec3 pb = support(b, v3Neg(n));
  ContactPoint cp;
  cp.point = v3Scale(v3Add(pa, pb), 0.5);
  cp.id = 24 + sat.index;
  cp.separation = sat.separation;
  result.contacts.push_back(cp);
  return result;
 }
 std::vector<Vec3> va_points;
 std::vector<int> va_ids;
 for(int i = 0; i < 8; ++i) {
  const double sx = (i % 2) == 0 ? -1.0 : 1.0;
  const double sy = ((i / 2) % 2) == 0 ? -1.0 : 1.0;
  const double sz = ((i / 4) % 2) == 0 ? -1.0 : 1.0;
  Vec3 p = a.center;
  p = v3Add(p, v3Scale(a.axes[0], sx * a.half.x));
  p = v3Add(p, v3Scale(a.axes[1], sy * a.half.y));
  p = v3Add(p, v3Scale(a.axes[2], sz * a.half.z));
  va_points.push_back(p);
  va_ids.push_back(i);
 }
 std::vector<Vec3> vb_points;
 std::vector<int> vb_ids;
 for(int i = 0; i < 8; ++i) {
  const double sx = (i % 2) == 0 ? -1.0 : 1.0;
  const double sy = ((i / 2) % 2) == 0 ? -1.0 : 1.0;
  const double sz = ((i / 4) % 2) == 0 ? -1.0 : 1.0;
  Vec3 p = b.center;
  p = v3Add(p, v3Scale(b.axes[0], sx * b.half.x));
  p = v3Add(p, v3Scale(b.axes[1], sy * b.half.y));
  p = v3Add(p, v3Scale(b.axes[2], sz * b.half.z));
  vb_points.push_back(p);
  vb_ids.push_back(i);
 }
 const double support_a = v3Dot(support(a, n), n);
 const double support_b = v3Dot(support(b, v3Neg(n)), n);
 const double band = std::max(0.0025, margin) + std::max(0.0, -sat.separation) * 0.25;
 const double tolerance = std::max(0.001, margin);
 std::vector<ContactPoint> candidates;
 for(std::size_t i = 0; i < va_points.size(); ++i) {
  const Vec3& vertex = va_points[i];
  if(v3Dot(vertex, n) >= support_a - band && pointInsideObb(vertex, b, tolerance)) {
   ContactPoint cp;
   cp.point = v3Add(vertex, v3Scale(n, 0.5 * sat.separation));
   cp.id = va_ids[i];
   cp.separation = sat.separation;
   candidates.push_back(cp);
  }
 }
 for(std::size_t i = 0; i < vb_points.size(); ++i) {
  const Vec3& vertex = vb_points[i];
  if(v3Dot(vertex, n) <= support_b + band && pointInsideObb(vertex, a, tolerance)) {
   ContactPoint cp;
   cp.point = v3Sub(vertex, v3Scale(n, 0.5 * sat.separation));
   cp.id = 8 + vb_ids[i];
   cp.separation = sat.separation;
   candidates.push_back(cp);
  }
 }
 if(candidates.empty()) {
  const Vec3 pa = support(a, n);
  const Vec3 pb = support(b, v3Neg(n));
  ContactPoint cp;
  cp.point = v3Scale(v3Add(pa, pb), 0.5);
  cp.id = 24 + sat.index;
  cp.separation = sat.separation;
  candidates.push_back(cp);
 }
 std::vector<ContactPoint> unique;
 for(std::size_t i = 0; i < candidates.size(); ++i) {
  bool duplicate = false;
  for(std::size_t j = 0; j < unique.size(); ++j) {
   if(v3LenSqr(v3Sub(candidates[i].point, unique[j].point)) < 1.0e-8) {
    duplicate = true;
    break;
   }
  }
  if(!duplicate) {
   unique.push_back(candidates[i]);
  }
 }
 if(unique.size() <= 4) {
  result.contacts = unique;
  return result;
 }
 Vec3 t1, t2;
 tangentBasis(n, t1, t2);
 std::vector<ContactPoint> selected;
 bool used[64] = {};
 for(int combo = 0; combo < 4; ++combo) {
  const double sign1 = (combo == 0 || combo == 3) ? 1.0 : -1.0;
  const double sign2 = (combo <= 1) ? 1.0 : -1.0;
  int best_i = -1;
  double best_value = 0.0;
  bool found = false;
  for(std::size_t i = 0; i < unique.size() && i < 64; ++i) {
   if(!used[i]) {
    const double value = sign1 * v3Dot(unique[i].point, t1) + sign2 * v3Dot(unique[i].point, t2);
    if(!found || value > best_value) {
     best_i = static_cast<int>(i);
     best_value = value;
     found = true;
    }
   }
  }
  if(found && best_i >= 0) {
   used[best_i] = true;
   selected.push_back(unique[static_cast<std::size_t>(best_i)]);
  }
 }
 result.contacts = selected;
 return result;
}
void readBodyCache(lua_State* state, int bodyIndex, Vec3& half, Vec3& orientationVec,
                   Vec3& angVelocity, double& inv_mass, Mat3& inv_inertia_local,
                   Vec3 axes[3], Mat3& inv_world) {
 LuaApi& api = luaApi();
 inv_mass = luaDoubleField(state, bodyIndex, "inv_mass", 0.0);
 api.getfield(state, bodyIndex, "half");
 half = readLuaV3(state, -1);
 api.settop(state, -2);
 api.getfield(state, bodyIndex, "orientation");
 Quat ori = readLuaQuat(state, -1);
  orientationVec = {ori.x, ori.y, ori.z};
 api.settop(state, -2);
 api.getfield(state, bodyIndex, "angular_velocity");
 angVelocity = readLuaV3(state, -1);
 api.settop(state, -2);
 const double x = ori.x, y = ori.y, z = ori.z, w = ori.w;
 const double xx = x * x, yy = y * y, zz = z * z;
 const double xy = x * y, xz = x * z, yz = y * z;
 const double wx = w * x, wy = w * y, wz = w * z;
 axes[0] = {1.0 - 2.0 * (yy + zz), 2.0 * (xy + wz), 2.0 * (xz - wy)};
 axes[1] = {2.0 * (xy - wz), 1.0 - 2.0 * (xx + zz), 2.0 * (yz + wx)};
 axes[2] = {2.0 * (xz + wy), 2.0 * (yz - wx), 1.0 - 2.0 * (xx + yy)};
 api.getfield(state, bodyIndex, "inv_inertia_local");
 inv_inertia_local = readLuaMat3(state, -1);
 api.settop(state, -2);
 const double ixx = inv_inertia_local.xx, iyy = inv_inertia_local.yy, izz = inv_inertia_local.zz;
 const double ixy = inv_inertia_local.xy, ixz = inv_inertia_local.xz, iyz = inv_inertia_local.yz;
 const Vec3& ax = axes[0];
 const Vec3& ay = axes[1];
 const Vec3& az = axes[2];
 inv_world.xx = ixx * ax.x * ax.x + iyy * ay.x * ay.x + izz * az.x * az.x +
                2.0 * (ixy * ax.x * ay.x + ixz * ax.x * az.x + iyz * ay.x * az.x);
 inv_world.yy = ixx * ax.y * ax.y + iyy * ay.y * ay.y + izz * az.y * az.y +
                2.0 * (ixy * ax.y * ay.y + ixz * ax.y * az.y + iyz * ay.y * az.y);
 inv_world.zz = ixx * ax.z * ax.z + iyy * ay.z * ay.z + izz * az.z * az.z +
                2.0 * (ixy * ax.z * ay.z + ixz * ax.z * az.z + iyz * ay.z * az.z);
 inv_world.xy = ixx * ax.x * ax.y + iyy * ay.x * ay.y + izz * az.x * az.y +
                ixy * (ax.x * ay.y + ay.x * ax.y) + ixz * (ax.x * az.y + az.x * ax.y) +
                iyz * (ay.x * az.y + az.x * ay.y);
 inv_world.xz = ixx * ax.x * ax.z + iyy * ay.x * ay.z + izz * az.x * az.z +
                ixy * (ax.x * ay.z + ay.x * ax.z) + ixz * (ax.x * az.z + az.x * ax.z) +
                iyz * (ay.x * az.z + az.x * ay.z);
 inv_world.yz = ixx * ax.y * ax.z + iyy * ay.y * ay.z + izz * az.y * az.z +
                ixy * (ax.y * ay.z + ay.y * ax.z) + ixz * (ax.y * az.z + az.y * ax.z) +
                iyz * (ay.y * az.z + az.y * ay.z);
}
Vec3 applyInvInertiaImpl(const Mat3& inv_world, const Vec3& world_vector) {
 return {
  inv_world.xx * world_vector.x + inv_world.xy * world_vector.y + inv_world.xz * world_vector.z,
  inv_world.xy * world_vector.x + inv_world.yy * world_vector.y + inv_world.yz * world_vector.z,
  inv_world.xz * world_vector.x + inv_world.yz * world_vector.y + inv_world.zz * world_vector.z,
 };
}
double effectiveMass(const Mat3* inv_world_a, double inv_mass_a, const Vec3& r_a,
                     const Mat3* inv_world_b, double inv_mass_b, const Vec3& r_b,
                     const Vec3& axis) {
 double k = inv_mass_a + inv_mass_b;
 if(inv_mass_a > 0.0 && inv_world_a != nullptr) {
  const Vec3 ra = v3Cross(r_a, axis);
  k += v3Dot(ra, applyInvInertiaImpl(*inv_world_a, ra));
 }
 if(inv_mass_b > 0.0 && inv_world_b != nullptr) {
  const Vec3 rb = v3Cross(r_b, axis);
  k += v3Dot(rb, applyInvInertiaImpl(*inv_world_b, rb));
 }
 return k > kEps ? 1.0 / k : 0.0;
}
struct SolverPoint {
 Manifold* manifold = nullptr;
 int contact_index = 0;
 Vec3 n;
 Vec3 t1;
 Vec3 t2;
 Vec3 r_a;
 Vec3 r_b;
 double normal_mass = 0.0;
 double tangent_mass1 = 0.0;
 double tangent_mass2 = 0.0;
 double normal_impulse = 0.0;
 double tangent_impulse1 = 0.0;
 double tangent_impulse2 = 0.0;
 double bias = 0.0;
 double restitution_target = 0.0;
 double friction = 0.0;
 Softness softness;
};
Vec3 pointVelocity(const Vec3& linear_velocity, const Vec3& angular_velocity, const Vec3& r) {
 return v3Add(linear_velocity, v3Cross(angular_velocity, r));
}
void applyImpulse(const Mat3& inv_world, double inv_mass, Vec3& linear_velocity,
                  Vec3& angular_velocity, const Vec3& r, const Vec3& impulse, double sign) {
 if(inv_mass <= 0.0) {
  return;
 }
 linear_velocity = v3Add(linear_velocity, v3Scale(impulse, sign * inv_mass));
 angular_velocity = v3Add(angular_velocity, applyInvInertiaImpl(inv_world, v3Scale(v3Cross(r, impulse), sign)));
}
std::vector<SolverPoint> preparePoints(const Mat3& inv_world_a, double inv_mass_a,
                                       Vec3& linear_velocity_a, Vec3& angular_velocity_a,
                                       const Mat3* inv_world_b, double inv_mass_b,
                                       Vec3& linear_velocity_b, Vec3& angular_velocity_b,
                                       const Vec3& position_a, const Vec3& position_b,
                                       std::vector<CachedManifold>& manifolds,
                                       double h, const Softness& softness, double slop,
                                       double restitution_threshold, double max_push,
                                       double default_friction, double default_restitution) {
 std::vector<SolverPoint> points;
 for(auto& cached : manifolds) {
  Manifold& manifold = cached.base;
  const Vec3 n = manifold.normal;
  Vec3 t1, t2;
  tangentBasis(n, t1, t2);
  const double friction = manifold.friction > 0.0 ? manifold.friction : default_friction;
  const double restitution = manifold.restitution > 0.0 ? manifold.restitution : default_restitution;
  for(std::size_t ci = 0; ci < cached.contacts.size(); ++ci) {
   ContactPoint& contact = cached.contacts[ci];
   SolverPoint sp;
   sp.manifold = &manifold;
   sp.contact_index = static_cast<int>(ci);
   sp.n = n;
   sp.t1 = t1;
   sp.t2 = t2;
   sp.r_a = v3Sub(contact.point, position_a);
   sp.r_b = inv_world_b ? v3Sub(contact.point, position_b) : Vec3{};
   const Vec3 va = pointVelocity(linear_velocity_a, angular_velocity_a, sp.r_a);
   const Vec3 vb = inv_world_b ? pointVelocity(linear_velocity_b, angular_velocity_b, sp.r_b) : Vec3{};
   const double initial_vn = v3Dot(v3Sub(vb, va), n);
   double separation = contact.separation;
   if(separation > 0.0) {
    sp.bias = separation / h;
   } else {
    sp.bias = std::max(-max_push, softness.bias_rate * std::min(separation + slop, 0.0));
   }
   sp.normal_mass = effectiveMass(&inv_world_a, inv_mass_a, sp.r_a,
                                  inv_world_b, inv_mass_b, sp.r_b, n);
   sp.tangent_mass1 = effectiveMass(&inv_world_a, inv_mass_a, sp.r_a,
                                    inv_world_b, inv_mass_b, sp.r_b, t1);
   sp.tangent_mass2 = effectiveMass(&inv_world_a, inv_mass_a, sp.r_a,
                                    inv_world_b, inv_mass_b, sp.r_b, t2);
   sp.normal_impulse = contact.normal_impulse;
   sp.tangent_impulse1 = contact.tangent_impulse1;
   sp.tangent_impulse2 = contact.tangent_impulse2;
   sp.restitution_target = initial_vn < -restitution_threshold ? (-restitution * initial_vn) : 0.0;
   sp.friction = friction;
   sp.softness = softness;
   points.push_back(sp);
  }
 }
 return points;
}
void warmStartImpl(const Mat3& inv_world_a, double inv_mass_a,
                   Vec3& linear_velocity_a, Vec3& angular_velocity_a,
                   const Mat3* inv_world_b, double inv_mass_b,
                   Vec3& linear_velocity_b, Vec3& angular_velocity_b,
                   std::vector<SolverPoint>& points) {
 for(auto& p : points) {
  const Vec3 impulse = v3Add(v3Scale(p.n, p.normal_impulse),
                             v3Add(v3Scale(p.t1, p.tangent_impulse1),
                                   v3Scale(p.t2, p.tangent_impulse2)));
  applyImpulse(inv_world_a, inv_mass_a, linear_velocity_a, angular_velocity_a, p.r_a, impulse, -1.0);
  if(inv_world_b) {
   applyImpulse(*inv_world_b, inv_mass_b, linear_velocity_b, angular_velocity_b, p.r_b, impulse, 1.0);
  }
 }
}
void solveVelocityImpl(const Mat3& inv_world_a, double inv_mass_a,
                       Vec3& linear_velocity_a, Vec3& angular_velocity_a,
                       const Mat3* inv_world_b, double inv_mass_b,
                       Vec3& linear_velocity_b, Vec3& angular_velocity_b,
                       std::vector<SolverPoint>& points, int iterations) {
 for(int iter = 0; iter < iterations; ++iter) {
  for(auto& p : points) {
   const Vec3 va = pointVelocity(linear_velocity_a, angular_velocity_a, p.r_a);
   const Vec3 vb = inv_world_b ? pointVelocity(linear_velocity_b, angular_velocity_b, p.r_b) : Vec3{};
   const double vn = v3Dot(v3Sub(vb, va), p.n);
   const Softness& soft = p.softness;
   double delta = -p.normal_mass * (soft.mass_scale * vn + p.bias) - soft.impulse_scale * p.normal_impulse;
   const double next_impulse = std::max(0.0, p.normal_impulse + delta);
   delta = next_impulse - p.normal_impulse;
   p.normal_impulse = next_impulse;
   const Vec3 impulse = v3Scale(p.n, delta);
   applyImpulse(inv_world_a, inv_mass_a, linear_velocity_a, angular_velocity_a, p.r_a, impulse, -1.0);
   if(inv_world_b) {
    applyImpulse(*inv_world_b, inv_mass_b, linear_velocity_b, angular_velocity_b, p.r_b, impulse, 1.0);
   }
  }
 }
 for(auto& p : points) {
  {
   const Vec3 va = pointVelocity(linear_velocity_a, angular_velocity_a, p.r_a);
   const Vec3 vb = inv_world_b ? pointVelocity(linear_velocity_b, angular_velocity_b, p.r_b) : Vec3{};
   const double vn = v3Dot(v3Sub(vb, va), p.n);
   double delta = -p.normal_mass * vn;
   const double next_impulse = std::max(0.0, p.normal_impulse + delta);
   delta = next_impulse - p.normal_impulse;
   p.normal_impulse = next_impulse;
   const Vec3 impulse = v3Scale(p.n, delta);
   applyImpulse(inv_world_a, inv_mass_a, linear_velocity_a, angular_velocity_a, p.r_a, impulse, -1.0);
   if(inv_world_b) {
    applyImpulse(*inv_world_b, inv_mass_b, linear_velocity_b, angular_velocity_b, p.r_b, impulse, 1.0);
   }
  }
  const Vec3 va = pointVelocity(linear_velocity_a, angular_velocity_a, p.r_a);
  const Vec3 vb = inv_world_b ? pointVelocity(linear_velocity_b, angular_velocity_b, p.r_b) : Vec3{};
  const Vec3 rv = v3Sub(vb, va);
  const double old1 = p.tangent_impulse1;
  const double old2 = p.tangent_impulse2;
  double next1 = old1 - p.tangent_mass1 * v3Dot(rv, p.t1);
  double next2 = old2 - p.tangent_mass2 * v3Dot(rv, p.t2);
  const double limit = p.friction * p.normal_impulse;
  const double mag2 = next1 * next1 + next2 * next2;
  if(mag2 > limit * limit && mag2 > kEps) {
   const double f = limit / std::sqrt(mag2);
   next1 *= f;
   next2 *= f;
  }
  p.tangent_impulse1 = next1;
  p.tangent_impulse2 = next2;
  const Vec3 friction_impulse = v3Add(v3Scale(p.t1, next1 - old1), v3Scale(p.t2, next2 - old2));
  applyImpulse(inv_world_a, inv_mass_a, linear_velocity_a, angular_velocity_a, p.r_a, friction_impulse, -1.0);
  if(inv_world_b) {
   applyImpulse(*inv_world_b, inv_mass_b, linear_velocity_b, angular_velocity_b, p.r_b, friction_impulse, 1.0);
  }
 }
 for(auto& p : points) {
  if(p.restitution_target > 0.0 && p.normal_impulse > 0.0) {
   const Vec3 va = pointVelocity(linear_velocity_a, angular_velocity_a, p.r_a);
   const Vec3 vb = inv_world_b ? pointVelocity(linear_velocity_b, angular_velocity_b, p.r_b) : Vec3{};
   const double vn = v3Dot(v3Sub(vb, va), p.n);
   double delta = p.normal_mass * (p.restitution_target - vn);
   const double next_impulse = std::max(0.0, p.normal_impulse + delta);
   delta = next_impulse - p.normal_impulse;
   p.normal_impulse = next_impulse;
   const Vec3 impulse = v3Scale(p.n, delta);
   applyImpulse(inv_world_a, inv_mass_a, linear_velocity_a, angular_velocity_a, p.r_a, impulse, -1.0);
   if(inv_world_b) {
    applyImpulse(*inv_world_b, inv_mass_b, linear_velocity_b, angular_velocity_b, p.r_b, impulse, 1.0);
   }
  }
 }
}
int luaBox3dV3(lua_State* state) {
 LuaApi& api = luaApi();
 const double x = luaDoubleArg(state, 1, 0.0);
 const double y = luaDoubleArg(state, 2, 0.0);
 const double z = luaDoubleArg(state, 3, 0.0);
 pushLuaV3(state, {x, y, z});
 return 1;
}
int luaBox3dV3Add(lua_State* state) {
 const Vec3 a = readLuaV3(state, 1);
 const Vec3 b = readLuaV3(state, 2);
 pushLuaV3(state, v3Add(a, b));
 return 1;
}
int luaBox3dV3Sub(lua_State* state) {
 const Vec3 a = readLuaV3(state, 1);
 const Vec3 b = readLuaV3(state, 2);
 pushLuaV3(state, v3Sub(a, b));
 return 1;
}
int luaBox3dV3Scale(lua_State* state) {
 const Vec3 a = readLuaV3(state, 1);
 const double s = luaDoubleArg(state, 2, 0.0);
 pushLuaV3(state, v3Scale(a, s));
 return 1;
}
int luaBox3dV3Dot(lua_State* state) {
 const Vec3 a = readLuaV3(state, 1);
 const Vec3 b = readLuaV3(state, 2);
 luaApi().pushnumber(state, v3Dot(a, b));
 return 1;
}
int luaBox3dV3Cross(lua_State* state) {
 const Vec3 a = readLuaV3(state, 1);
 const Vec3 b = readLuaV3(state, 2);
 pushLuaV3(state, v3Cross(a, b));
 return 1;
}
int luaBox3dV3Len(lua_State* state) {
 const Vec3 a = readLuaV3(state, 1);
 luaApi().pushnumber(state, v3Len(a));
 return 1;
}
int luaBox3dV3LenSqr(lua_State* state) {
 const Vec3 a = readLuaV3(state, 1);
 luaApi().pushnumber(state, v3LenSqr(a));
 return 1;
}
int luaBox3dV3Normalize(lua_State* state) {
 const Vec3 a = readLuaV3(state, 1);
 pushLuaV3(state, v3Normalize(a));
 return 1;
}
int luaBox3dV3Lerp(lua_State* state) {
 const Vec3 a = readLuaV3(state, 1);
 const Vec3 b = readLuaV3(state, 2);
 const double t = luaDoubleArg(state, 3, 0.0);
 pushLuaV3(state, v3Lerp(a, b, t));
 return 1;
}
int luaBox3dQuatIdentity(lua_State* state) {
 pushLuaQuat(state, {0.0, 0.0, 0.0, 1.0});
 return 1;
}
int luaBox3dQuatNormalize(lua_State* state) {
 const Quat q = readLuaQuat(state, 1);
 pushLuaQuat(state, quatNormalize(q));
 return 1;
}
int luaBox3dQuatMul(lua_State* state) {
 const Quat a = readLuaQuat(state, 1);
 const Quat b = readLuaQuat(state, 2);
 pushLuaQuat(state, quatMul(a, b));
 return 1;
}
int luaBox3dQuatRotate(lua_State* state) {
 const Quat q = readLuaQuat(state, 1);
 const Vec3 p = readLuaV3(state, 2);
 pushLuaV3(state, quatRotate(q, p));
 return 1;
}
int luaBox3dQuatInvRotate(lua_State* state) {
 const Quat q = readLuaQuat(state, 1);
 const Vec3 p = readLuaV3(state, 2);
 pushLuaV3(state, quatInvRotate(q, p));
 return 1;
}
int luaBox3dQuatSlerp(lua_State* state) {
 const Quat a = readLuaQuat(state, 1);
 const Quat b = readLuaQuat(state, 2);
 const double t = luaDoubleArg(state, 3, 0.0);
 pushLuaQuat(state, quatSlerp(a, b, t));
 return 1;
}
int luaBox3dQuatToEulerDegrees(lua_State* state) {
 const Quat q = readLuaQuat(state, 1);
 const double k = 180.0 / 3.14159265358979323846;
 LuaApi& api = luaApi();
 api.pushnumber(state, quatToYaw(q) * k);
 api.pushnumber(state, quatToPitch(q) * k);
 api.pushnumber(state, quatToRoll(q) * k);
 return 3;
}
int luaBox3dMakeQuatFromAxisAngle(lua_State* state) {
 Vec3 axis = readLuaV3(state, 1);
 const double radians = luaDoubleArg(state, 2, 0.0);
 axis = v3Normalize(axis);
 const double h = 0.5 * radians;
 const double s = std::sin(h);
 pushLuaQuat(state, quatNormalize({axis.x * s, axis.y * s, axis.z * s, std::cos(h)}));
 return 1;
}
int luaBox3dMakeSoft(lua_State* state) {
 const double hertz = luaDoubleArg(state, 1, 0.0);
 const double damping = luaDoubleArg(state, 2, 0.0);
 const double h = luaDoubleArg(state, 3, 0.0);
 const Softness soft = makeSoft(hertz, damping, h);
 LuaApi& api = luaApi();
 api.createtable(state, 0, 3);
 setField(state, "bias_rate", soft.bias_rate);
 setField(state, "mass_scale", soft.mass_scale);
 setField(state, "impulse_scale", soft.impulse_scale);
 return 1;
}
int luaBox3dSyncBodyCache(lua_State* state) {
 LuaApi& api = luaApi();
 const int bodyIdx = 1;
 Vec3 half, ori_v, ang_v;
 double inv_mass;
 Mat3 inv_il;
 Vec3 axes[3];
 Mat3 iw;
 readBodyCache(state, bodyIdx, half, ori_v, ang_v, inv_mass, inv_il, axes, iw);
 api.createtable(state, 0, 3);
 api.pushvalue(state, -1);
 api.rawseti(state, -2, 1);
 api.pushvalue(state, -1);
 api.rawseti(state, -2, 2);
 api.pushvalue(state, -1);
 api.rawseti(state, -2, 3);
 for(int i = 0; i < 3; ++i) {
  api.rawseti(state, -2, static_cast<std::int64_t>(i + 1));
  pushLuaV3(state, axes[i]);
  api.rawseti(state, -2, static_cast<std::int64_t>(i + 1));
 }
 api.createtable(state, 0, 6);
 setField(state, "xx", iw.xx);
 setField(state, "yy", iw.yy);
 setField(state, "zz", iw.zz);
 setField(state, "xy", iw.xy);
 setField(state, "xz", iw.xz);
 setField(state, "yz", iw.yz);
 const int axesIdx = api.gettop(state) - 1;
 const int iwIdx = api.gettop(state);
 api.pushvalue(state, axesIdx);
 api.pushvalue(state, iwIdx);
 return 2;
}
int luaBox3dApplyInvInertia(lua_State* state) {
 LuaApi& api = luaApi();
 const int bodyIdx = 1;
 const Vec3 world_vec = readLuaV3(state, 2);
 const double inv_mass = luaDoubleField(state, bodyIdx, "inv_mass", 0.0);
 if(inv_mass <= 0.0) {
  pushLuaV3(state, {0.0, 0.0, 0.0});
  return 1;
 }
 Vec3 half, ori_v, ang_v;
 double inv_mass_read;
 Mat3 inv_il;
 Vec3 axes[3];
 Mat3 iw;
 readBodyCache(state, bodyIdx, half, ori_v, ang_v, inv_mass_read, inv_il, axes, iw);
 pushLuaV3(state, applyInvInertiaImpl(iw, world_vec));
 return 1;
}
int luaBox3dNewBox(lua_State* state) {
 LuaApi& api = luaApi();
 const double half_x = luaDoubleArg(state, 1, 0.0);
 const double half_y = luaDoubleArg(state, 2, 0.0);
 const double half_z = luaDoubleArg(state, 3, 0.0);
 double mass = luaDoubleArg(state, 4, 1.0);
 mass = std::max(0.0, mass);
 const double ix = (mass / 3.0) * (half_y * half_y + half_z * half_z);
 const double iy = (mass / 3.0) * (half_x * half_x + half_z * half_z);
 const double iz = (mass / 3.0) * (half_x * half_x + half_y * half_y);
 api.createtable(state, 0, 10);
 pushLuaV3(state, {half_x, half_y, half_z});
 api.setfield(state, -2, "half");
 setField(state, "mass", mass);
 setField(state, "inv_mass", mass > kEps ? 1.0 / mass : 0.0);
 api.createtable(state, 0, 6);
 setField(state, "xx", ix);
 setField(state, "yy", iy);
 setField(state, "zz", iz);
 setField(state, "xy", 0.0);
 setField(state, "xz", 0.0);
 setField(state, "yz", 0.0);
 api.setfield(state, -2, "inertia_local");
 api.createtable(state, 0, 6);
 setField(state, "xx", ix > kEps ? 1.0 / ix : 0.0);
 setField(state, "yy", iy > kEps ? 1.0 / iy : 0.0);
 setField(state, "zz", iz > kEps ? 1.0 / iz : 0.0);
 setField(state, "xy", 0.0);
 setField(state, "xz", 0.0);
 setField(state, "yz", 0.0);
 api.setfield(state, -2, "inv_inertia_local");
 pushLuaQuat(state, {0.0, 0.0, 0.0, 1.0});
 api.setfield(state, -2, "orientation");
 pushLuaV3(state, {0.0, 0.0, 0.0});
 api.setfield(state, -2, "angular_velocity");
 return 1;
}
int luaBox3dNewVoxelBody(lua_State* state) {
 LuaApi& api = luaApi();
 double mass = luaDoubleArg(state, 5, 1.0);
 mass = std::max(kEps, mass);
 const double scale_value = luaDoubleArg(state, 4, 1.0);
 const int width = luaIntArg(state, 2, 16);
 const int height = luaIntArg(state, 3, 16);
 double thickness = api.gettop(state) >= 6 ? luaDoubleArg(state, 6, 0.0) : scale_value / std::max(width, 1);
 if(thickness <= 0.0) {
  thickness = scale_value / std::max(width, 1);
 }
 if(api.type(state, 1) != kLuaTTable) {
  return luaBox3dNewBox(state);
 }
 const int pixelsIdx = 1;
 long long count = 0;
 long long sx = 0;
 long long sy = 0;
 api.pushnil(state);
 while(api.next(state, pixelsIdx) != 0) {
  const int idx = static_cast<int>(api.tointegerx(state, -2, nullptr));
  if(idx >= 1 && idx <= width * height) {
   const long long c = api.tointegerx(state, -1, nullptr);
   if(((c / 16777216) % 256) > 0) {
    const int y_idx = (idx - 1) / width;
    const int x_idx = (idx - 1) % width;
    ++count;
    sx += x_idx;
    sy += y_idx;
   }
  }
  api.settop(state, -2);
 }
 api.settop(state, -2);
 if(count == 0) {
  return luaBox3dNewBox(state);
 }
 const double dx = scale_value / width;
 const double dy = scale_value / height;
 const double cx = static_cast<double>(sx) / static_cast<double>(count);
 const double cy = static_cast<double>(sy) / static_cast<double>(count);
 const double com_x = -0.5 * scale_value + (cx + 0.5) * dx;
 const double com_y = 0.5 * scale_value - (cy + 0.5) * dy;
 const double voxel_mass = mass / static_cast<double>(count);
 const double ix0 = voxel_mass * (dy * dy + thickness * thickness) / 12.0;
 const double iy0 = voxel_mass * (dx * dx + thickness * thickness) / 12.0;
 const double iz0 = voxel_mass * (dx * dx + dy * dy) / 12.0;
 double ix = 0.0, iy = 0.0, iz = 0.0, ixy = 0.0;
 for(int y = 0; y < height; ++y) {
  for(int x = 0; x < width; ++x) {
   const int idx = 1 + y * width + x;
   api.rawgeti(state, pixelsIdx, static_cast<std::int64_t>(idx));
   const long long c = api.tointegerx(state, -1, nullptr);
   api.settop(state, -2);
   if(c > 0 && ((c / 16777216) % 256) > 0) {
    const double px = -0.5 * scale_value + (x + 0.5) * dx - com_x;
    const double py = 0.5 * scale_value - (y + 0.5) * dy - com_y;
    ix += ix0 + voxel_mass * py * py;
    iy += iy0 + voxel_mass * px * px;
    iz += iz0 + voxel_mass * (px * px + py * py);
    ixy -= voxel_mass * px * py;
   }
  }
 }
 Mat3 inertia{ix, iy, iz, ixy, 0.0, 0.0};
 Mat3 inv = invertSymmetric(inertia);
 api.createtable(state, 0, 10);
 pushLuaV3(state, {scale_value * 0.5, scale_value * 0.5, thickness * 0.5});
 api.setfield(state, -2, "half");
 setField(state, "mass", mass);
 setField(state, "inv_mass", 1.0 / mass);
 api.createtable(state, 0, 6);
 setField(state, "xx", inertia.xx);
 setField(state, "yy", inertia.yy);
 setField(state, "zz", inertia.zz);
 setField(state, "xy", inertia.xy);
 setField(state, "xz", 0.0);
 setField(state, "yz", 0.0);
 api.setfield(state, -2, "inertia_local");
 api.createtable(state, 0, 6);
 setField(state, "xx", inv.xx);
 setField(state, "yy", inv.yy);
 setField(state, "zz", inv.zz);
 setField(state, "xy", inv.xy);
 setField(state, "xz", inv.xz);
 setField(state, "yz", inv.yz);
 api.setfield(state, -2, "inv_inertia_local");
 pushLuaQuat(state, {0.0, 0.0, 0.0, 1.0});
 api.setfield(state, -2, "orientation");
 pushLuaV3(state, {0.0, 0.0, 0.0});
 api.setfield(state, -2, "angular_velocity");
 pushLuaV3(state, {com_x, com_y, 0.0});
 return 2;
}
int luaBox3dIntegrateRotation(lua_State* state) {
 LuaApi& api = luaApi();
 const int bodyIdx = 1;
 const double h = luaDoubleArg(state, 2, 0.0);
 api.getfield(state, bodyIdx, "orientation");
 const Quat q = readLuaQuat(state, -1);
 api.settop(state, -2);
 api.getfield(state, bodyIdx, "angular_velocity");
 const Vec3 w = readLuaV3(state, -1);
 api.settop(state, -2);
 const Quat result = quatIntegrate(q, w, h);
 pushLuaQuat(state, result);
 api.setfield(state, bodyIdx, "orientation");
 return 0;
}
int luaBox3dBodyAabb(lua_State* state) {
 LuaApi& api = luaApi();
 const Vec3 position = readLuaV3(state, 1);
 const int bodyIdx = 2;
 const Vec3 com_offset = api.gettop(state) >= 3 ? readLuaV3(state, 3) : Vec3{};
 const double inflate = api.gettop(state) >= 4 ? luaDoubleArg(state, 4, 0.0) : 0.0;
 Vec3 half, ori_v, ang_v;
 double inv_mass;
 Mat3 inv_il;
 Vec3 axes[3];
 Mat3 iw;
 readBodyCache(state, bodyIdx, half, ori_v, ang_v, inv_mass, inv_il, axes, iw);
 const Vec3& ax = axes[0];
 const Vec3& ay = axes[1];
 const Vec3& az = axes[2];
 const Obb obb = makeBodyObb(position, half, axes, com_offset);
 const double ex = obb.half.x * std::abs(ax.x) + obb.half.y * std::abs(ay.x) +
                   obb.half.z * std::abs(az.x);
 const double ey = obb.half.x * std::abs(ax.y) + obb.half.y * std::abs(ay.y) +
                   obb.half.z * std::abs(az.y);
 const double ez = obb.half.x * std::abs(ax.z) + obb.half.y * std::abs(ay.z) +
                   obb.half.z * std::abs(az.z);
 Aabb aabb;
 aabb.min_x = obb.center.x - ex - inflate;
 aabb.max_x = obb.center.x + ex + inflate;
 aabb.min_y = obb.center.y - ey - inflate;
 aabb.max_y = obb.center.y + ey + inflate;
 aabb.min_z = obb.center.z - ez - inflate;
 aabb.max_z = obb.center.z + ez + inflate;
 const bool hasOut = api.gettop(state) >= 5 && api.type(state, 5) == kLuaTTable;
 if(hasOut) {
  api.pushvalue(state, 5);
 } else {
  pushLuaAabb(state, aabb);
  return 1;
 }
 api.pushnumber(state, aabb.min_x);
 api.setfield(state, -2, "min_x");
 api.pushnumber(state, aabb.max_x);
 api.setfield(state, -2, "max_x");
 api.pushnumber(state, aabb.min_y);
 api.setfield(state, -2, "min_y");
 api.pushnumber(state, aabb.max_y);
 api.setfield(state, -2, "max_y");
 api.pushnumber(state, aabb.min_z);
 api.setfield(state, -2, "min_z");
 api.pushnumber(state, aabb.max_z);
 api.setfield(state, -2, "max_z");
 return 1;
}
int luaBox3dBoxAabbManifold(lua_State* state) {
 LuaApi& api = luaApi();
 const Vec3 position = readLuaV3(state, 1);
 const int bodyIdx = 2;
 const Vec3 com_offset = api.gettop(state) >= 3 ? readLuaV3(state, 3) : Vec3{};
 const Aabb aabb = readLuaAabb(state, 4);
 const double margin = api.gettop(state) >= 5 ? luaDoubleArg(state, 5, 0.0) : 0.0;
 Vec3 half, ori_v, ang_v;
 double inv_mass;
 Mat3 inv_il;
 Vec3 axes[3];
 Mat3 iw;
 readBodyCache(state, bodyIdx, half, ori_v, ang_v, inv_mass, inv_il, axes, iw);
 const Obb a = makeBodyObb(position, half, axes, com_offset);
 const Obb b = makeAabbObb(aabb);
 SatResult sat;
 const SatResult* best = satObb(a, b, margin, sat);
 if(best == nullptr) {
  api.pushnil(state);
  return 1;
 }
 CachedManifold manif = buildCachedManifold(a, b, *best, margin, false);
 api.createtable(state, 0, 7);
 pushLuaV3(state, manif.base.normal);
 api.setfield(state, -2, "normal");
 setField(state, "separation", manif.base.separation);
 setField(state, "feature", static_cast<std::int64_t>(manif.base.feature));
 api.createtable(state, static_cast<int>(manif.contacts.size()), 0);
 for(std::size_t i = 0; i < manif.contacts.size(); ++i) {
  const ContactPoint& cp = manif.contacts[i];
  api.createtable(state, 0, 5);
  pushLuaV3(state, cp.point);
  api.setfield(state, -2, "point");
  setField(state, "id", static_cast<std::int64_t>(cp.id));
  setField(state, "separation", cp.separation);
  setField(state, "normal_impulse", cp.normal_impulse);
  setField(state, "tangent_impulse1", cp.tangent_impulse1);
  setField(state, "tangent_impulse2", cp.tangent_impulse2);
  api.rawseti(state, -2, static_cast<std::int64_t>(i + 1));
 }
 api.setfield(state, -2, "contacts");
 return 1;
}
int luaBox3dBoxBoxManifold(lua_State* state) {
 LuaApi& api = luaApi();
 const Vec3 position_a = readLuaV3(state, 1);
 const int bodyAIdx = 2;
 const Vec3 offset_a = api.gettop(state) >= 3 ? readLuaV3(state, 3) : Vec3{};
 const Vec3 position_b = readLuaV3(state, 4);
 const int bodyBIdx = 5;
 const Vec3 offset_b = api.gettop(state) >= 6 ? readLuaV3(state, 6) : Vec3{};
 const double margin = api.gettop(state) >= 7 ? luaDoubleArg(state, 7, 0.0) : 0.0;
 const bool single_contact = api.gettop(state) >= 8 && api.toboolean(state, 8);
 Vec3 half_a, ori_a, ang_a, half_b, ori_b, ang_b;
 double inv_mass_a, inv_mass_b;
 Mat3 inv_il_a, inv_il_b;
 Vec3 axes_a[3], axes_b[3];
 Mat3 iw_a, iw_b;
 readBodyCache(state, bodyAIdx, half_a, ori_a, ang_a, inv_mass_a, inv_il_a, axes_a, iw_a);
 readBodyCache(state, bodyBIdx, half_b, ori_b, ang_b, inv_mass_b, inv_il_b, axes_b, iw_b);
 const Obb a = makeBodyObb(position_a, half_a, axes_a, offset_a);
 const Obb b = makeBodyObb(position_b, half_b, axes_b, offset_b);
 SatResult sat;
 const SatResult* best = satObb(a, b, margin, sat);
 if(best == nullptr) {
  api.pushnil(state);
  return 1;
 }
 CachedManifold manif = buildCachedManifold(a, b, *best, margin, single_contact);
 api.createtable(state, 0, 7);
 pushLuaV3(state, manif.base.normal);
 api.setfield(state, -2, "normal");
 setField(state, "separation", manif.base.separation);
 setField(state, "feature", static_cast<std::int64_t>(manif.base.feature));
 api.createtable(state, static_cast<int>(manif.contacts.size()), 0);
 for(std::size_t i = 0; i < manif.contacts.size(); ++i) {
  const ContactPoint& cp = manif.contacts[i];
  api.createtable(state, 0, 5);
  pushLuaV3(state, cp.point);
  api.setfield(state, -2, "point");
  setField(state, "id", static_cast<std::int64_t>(cp.id));
  setField(state, "separation", cp.separation);
  setField(state, "normal_impulse", cp.normal_impulse);
  setField(state, "tangent_impulse1", cp.tangent_impulse1);
  setField(state, "tangent_impulse2", cp.tangent_impulse2);
  api.rawseti(state, -2, static_cast<std::int64_t>(i + 1));
 }
 api.setfield(state, -2, "contacts");
 return 1;
}
int luaBox3dSweepBoxAabb(lua_State* state) {
 LuaApi& api = luaApi();
 const Vec3 position = readLuaV3(state, 1);
 const Vec3 delta = readLuaV3(state, 2);
 const int bodyIdx = 3;
 const Vec3 com_offset = api.gettop(state) >= 4 ? readLuaV3(state, 4) : Vec3{};
 const Aabb aabb = readLuaAabb(state, 5);
 const double margin = api.gettop(state) >= 6 ? luaDoubleArg(state, 6, 0.0) : 0.0;
 Vec3 half, ori_v, ang_v;
 double inv_mass;
 Mat3 inv_il;
 Vec3 axes[3];
 Mat3 iw;
 readBodyCache(state, bodyIdx, half, ori_v, ang_v, inv_mass, inv_il, axes, iw);
 const Obb a = makeBodyObb(position, half, axes, com_offset);
 const Obb b = makeAabbObb(aabb);
 const Vec3 center_delta = v3Sub(b.center, a.center);
 const Vec3 relative_motion = v3Neg(delta);
 double enter = 0.0;
 double exit = 1.0;
 Vec3 hit_normal;
 Vec3 all_axes[15];
 int axis_count = 0;
 for(int i = 0; i < 3; ++i) {
  all_axes[axis_count++] = a.axes[i];
 }
 for(int i = 0; i < 3; ++i) {
  all_axes[axis_count++] = b.axes[i];
 }
 for(int i = 0; i < 3; ++i) {
  for(int j = 0; j < 3; ++j) {
   all_axes[axis_count++] = v3Cross(a.axes[i], b.axes[j]);
  }
 }
 for(int i = 0; i < axis_count; ++i) {
  const Vec3& raw = all_axes[i];
  const double ls = v3LenSqr(raw);
  if(ls > 1.0e-12) {
   const Vec3 axis = v3Scale(raw, 1.0 / std::sqrt(ls));
   const double radius = radiusOnAxis(a, axis) + radiusOnAxis(b, axis) + margin;
   const double d0 = v3Dot(center_delta, axis);
   const double dv = v3Dot(relative_motion, axis);
   if(std::abs(dv) <= kEps) {
    if(std::abs(d0) > radius) {
     api.pushnil(state);
     return 1;
    }
   } else {
    double t0 = (-radius - d0) / dv;
    double t1 = (radius - d0) / dv;
    if(t0 > t1) {
     std::swap(t0, t1);
    }
    if(t0 > enter) {
     enter = t0;
     const double d_at = d0 + dv * t0;
     hit_normal = d_at >= 0.0 ? axis : v3Neg(axis);
    }
    exit = std::min(exit, t1);
    if(enter > exit) {
     api.pushnil(state);
     return 1;
    }
   }
  }
 }
 if(enter < 0.0 || enter > 1.0) {
  api.pushnil(state);
  return 1;
 }
 api.pushnumber(state, enter);
 pushLuaV3(state, hit_normal);
 return 2;
}
int luaBox3dSolveStaticManifolds(lua_State* state) {
 LuaApi& api = luaApi();
 const int bodyIdx = 1;
 const Vec3 position = readLuaV3(state, 2);
 const Vec3 velocity_in = readLuaV3(state, 3);
 Vec3 half, ori_v, ang_v;
 double inv_mass;
 Mat3 inv_il;
 Vec3 axes[3];
 Mat3 iw;
 readBodyCache(state, bodyIdx, half, ori_v, ang_v, inv_mass, inv_il, axes, iw);
 Vec3 linear_velocity = velocity_in;
 Vec3 angular_velocity = ang_v;
 const int optionsIdx = api.gettop(state) >= 5 ? 5 : 4;
 double h = luaDoubleField(state, optionsIdx, "h", 1.0);
 h = std::max(h, kEps);
 api.getfield(state, optionsIdx, "softness");
 Softness softness;
 if(api.type(state, -1) == kLuaTTable) {
  softness.bias_rate = luaDoubleField(state, -1, "bias_rate", 0.2 / h);
  softness.mass_scale = luaDoubleField(state, -1, "mass_scale", 1.0);
  softness.impulse_scale = luaDoubleField(state, -1, "impulse_scale", 0.0);
 } else {
  softness = {0.2 / h, 1.0, 0.0};
 }
 api.settop(state, -2);
 const double slop = luaDoubleField(state, optionsIdx, "slop", 0.0015);
 const double restitution_threshold = luaDoubleField(state, optionsIdx, "restitution_threshold", 0.05);
 const double max_push = luaDoubleField(state, optionsIdx, "max_push_speed", 0.15);
 const bool warm_start = luaBoolField(state, optionsIdx, "warm_start", true);
 const int iterations = luaIntField(state, optionsIdx, "iterations", 1);
 std::vector<CachedManifold> manifolds_vec;
 const int manifoldsIdx = 4;
 if(api.gettop(state) >= manifoldsIdx && api.type(state, manifoldsIdx) == kLuaTTable) {
  const int savedTop = api.gettop(state);
  api.pushnil(state);
  while(api.next(state, manifoldsIdx) != 0) {
   if(api.type(state, -1) == kLuaTTable) {
    CachedManifold cm;
    cm.base.normal = readLuaV3(state, -1);
    cm.base.separation = luaDoubleField(state, -1, "separation", 0.0);
    cm.base.friction = luaDoubleField(state, -1, "friction", 0.0);
    cm.base.restitution = luaDoubleField(state, -1, "restitution", 0.0);
    cm.base.feature = luaIntField(state, -1, "feature", 0);
    api.getfield(state, -1, "contacts");
    if(api.type(state, -1) == kLuaTTable) {
     api.pushnil(state);
     while(api.next(state, -2) != 0) {
      if(api.type(state, -1) == kLuaTTable) {
       ContactPoint cp;
       cp.point = readLuaV3(state, -1);
       cp.id = luaIntField(state, -1, "id", 0);
       cp.separation = luaDoubleField(state, -1, "separation", 0.0);
       cp.normal_impulse = luaDoubleField(state, -1, "normal_impulse", 0.0);
       cp.tangent_impulse1 = luaDoubleField(state, -1, "tangent_impulse1", 0.0);
       cp.tangent_impulse2 = luaDoubleField(state, -1, "tangent_impulse2", 0.0);
       cm.contacts.push_back(cp);
      }
      api.settop(state, -2);
     }
    }
    api.settop(state, -2);
    manifolds_vec.push_back(std::move(cm));
   }
   api.settop(state, -2);
  }
  api.settop(state, savedTop);
 }
 if(manifolds_vec.empty()) {
  api.pushvalue(state, 3);
  api.pushnumber(state, 0.0);
  return 2;
 }
 Vec3 position_b{};
 Vec3 linear_velocity_b{};
 Vec3 angular_velocity_b{};
 std::vector<SolverPoint> points = preparePoints(
  iw, inv_mass, linear_velocity, angular_velocity,
  nullptr, 0.0, linear_velocity_b, angular_velocity_b,
  position, position_b, manifolds_vec,
  h, softness, slop, restitution_threshold, max_push, 0.6, 0.0);
 if(warm_start) {
  warmStartImpl(iw, inv_mass, linear_velocity, angular_velocity,
                nullptr, 0.0, linear_velocity_b, angular_velocity_b, points);
 }
 solveVelocityImpl(iw, inv_mass, linear_velocity, angular_velocity,
                   nullptr, 0.0, linear_velocity_b, angular_velocity_b,
                   points, std::max(1, iterations));
 double total_normal = 0.0;
 for(const auto& p : points) {
  total_normal += p.normal_impulse;
 }
 pushLuaV3(state, linear_velocity);
 api.pushnumber(state, total_normal);
 return 2;
}
int luaBox3dSolvePairManifold(lua_State* state) {
 LuaApi& api = luaApi();
 const int bodyAIdx = 1;
 const Vec3 position_a = readLuaV3(state, 2);
 const Vec3 velocity_a_in = readLuaV3(state, 3);
 const int bodyBIdx = 4;
 const Vec3 position_b = readLuaV3(state, 5);
 const Vec3 velocity_b_in = readLuaV3(state, 6);
 Vec3 half_a, ori_a, ang_a, half_b, ori_b, ang_b;
 double inv_mass_a, inv_mass_b;
 Mat3 inv_il_a, inv_il_b;
 Vec3 axes_a[3], axes_b[3];
 Mat3 iw_a, iw_b;
 readBodyCache(state, bodyAIdx, half_a, ori_a, ang_a, inv_mass_a, inv_il_a, axes_a, iw_a);
 readBodyCache(state, bodyBIdx, half_b, ori_b, ang_b, inv_mass_b, inv_il_b, axes_b, iw_b);
 Vec3 linear_velocity_a = velocity_a_in;
 Vec3 angular_velocity_a = ang_a;
 Vec3 linear_velocity_b = velocity_b_in;
 Vec3 angular_velocity_b = ang_b;
 const int manifoldIdx = 7;
 const int optionsIdx = api.gettop(state) >= 8 ? 8 : 7;
 double h = luaDoubleField(state, optionsIdx, "h", 1.0);
 h = std::max(h, kEps);
 api.getfield(state, optionsIdx, "softness");
 Softness softness;
 if(api.type(state, -1) == kLuaTTable) {
  softness.bias_rate = luaDoubleField(state, -1, "bias_rate", 0.2 / h);
  softness.mass_scale = luaDoubleField(state, -1, "mass_scale", 1.0);
  softness.impulse_scale = luaDoubleField(state, -1, "impulse_scale", 0.0);
 } else {
  softness = {0.2 / h, 1.0, 0.0};
 }
 api.settop(state, -2);
 const double slop = luaDoubleField(state, optionsIdx, "slop", 0.0015);
 const double restitution_threshold = luaDoubleField(state, optionsIdx, "restitution_threshold", 0.05);
 const double max_push = luaDoubleField(state, optionsIdx, "max_push_speed", 0.15);
 const bool warm_start_opt = luaBoolField(state, optionsIdx, "warm_start", true);
 const int iterations = luaIntField(state, optionsIdx, "iterations", 1);
 std::vector<CachedManifold> manifolds_vec;
 if(api.gettop(state) >= manifoldIdx && api.type(state, manifoldIdx) == kLuaTTable) {
  CachedManifold cm;
  cm.base.normal = readLuaV3(state, manifoldIdx);
  cm.base.separation = luaDoubleField(state, manifoldIdx, "separation", 0.0);
  cm.base.friction = luaDoubleField(state, manifoldIdx, "friction", 0.0);
  cm.base.restitution = luaDoubleField(state, manifoldIdx, "restitution", 0.0);
  cm.base.feature = luaIntField(state, manifoldIdx, "feature", 0);
  api.getfield(state, manifoldIdx, "contacts");
  if(api.type(state, -1) == kLuaTTable) {
   const int savedTop = api.gettop(state);
   api.pushnil(state);
   while(api.next(state, -2) != 0) {
    if(api.type(state, -1) == kLuaTTable) {
     ContactPoint cp;
     cp.point = readLuaV3(state, -1);
     cp.id = luaIntField(state, -1, "id", 0);
     cp.separation = luaDoubleField(state, -1, "separation", 0.0);
     cp.normal_impulse = luaDoubleField(state, -1, "normal_impulse", 0.0);
     cp.tangent_impulse1 = luaDoubleField(state, -1, "tangent_impulse1", 0.0);
     cp.tangent_impulse2 = luaDoubleField(state, -1, "tangent_impulse2", 0.0);
     cm.contacts.push_back(cp);
    }
    api.settop(state, -2);
   }
   api.settop(state, savedTop);
  }
  api.settop(state, -2);
  manifolds_vec.push_back(std::move(cm));
 }
 if(manifolds_vec.empty()) {
  pushLuaV3(state, linear_velocity_a);
  pushLuaV3(state, linear_velocity_b);
  api.pushnumber(state, 0.0);
  return 3;
 }
 std::vector<SolverPoint> points = preparePoints(
  iw_a, inv_mass_a, linear_velocity_a, angular_velocity_a,
  &iw_b, inv_mass_b, linear_velocity_b, angular_velocity_b,
  position_a, position_b, manifolds_vec,
  h, softness, slop, restitution_threshold, max_push, 0.6, 0.0);
 if(warm_start_opt) {
  warmStartImpl(iw_a, inv_mass_a, linear_velocity_a, angular_velocity_a,
                &iw_b, inv_mass_b, linear_velocity_b, angular_velocity_b, points);
 }
 solveVelocityImpl(iw_a, inv_mass_a, linear_velocity_a, angular_velocity_a,
                   &iw_b, inv_mass_b, linear_velocity_b, angular_velocity_b,
                   points, std::max(1, iterations));
 double total_normal = 0.0;
 for(const auto& p : points) {
  total_normal += p.normal_impulse;
 }
 pushLuaV3(state, linear_velocity_a);
 pushLuaV3(state, linear_velocity_b);
 api.pushnumber(state, total_normal);
 return 3;
}
int luaBox3dCorrectStaticPosition(lua_State* state) {
 LuaApi& api = luaApi();
 Vec3 position = readLuaV3(state, 1);
 const int manifoldIdx = 2;
 const double slop = api.gettop(state) >= 3 ? luaDoubleArg(state, 3, 0.0015) : 0.0015;
 const double percent = api.gettop(state) >= 4 ? luaDoubleArg(state, 4, 0.75) : 0.75;
 const double max_correction = api.gettop(state) >= 5 ? luaDoubleArg(state, 5, 0.15) : 0.15;
 const double separation = luaDoubleField(state, manifoldIdx, "separation", 0.0);
 const double penetration = std::max(0.0, -separation - slop);
 if(penetration <= 0.0) {
  pushLuaV3(state, position);
  api.pushnumber(state, 0.0);
  return 2;
 }
 const Vec3 normal = readLuaV3(state, manifoldIdx);
 const double correction = std::min(max_correction, penetration * percent);
 pushLuaV3(state, v3Sub(position, v3Scale(normal, correction)));
 api.pushnumber(state, correction);
 return 2;
}
int luaBox3dCorrectPairPositions(lua_State* state) {
 LuaApi& api = luaApi();
 Vec3 position_a = readLuaV3(state, 1);
 const int bodyAIdx = 2;
 Vec3 position_b = readLuaV3(state, 3);
 const int bodyBIdx = 4;
 const int manifoldIdx = 5;
 const double slop = api.gettop(state) >= 6 ? luaDoubleArg(state, 6, 0.0015) : 0.0015;
 const double percent = api.gettop(state) >= 7 ? luaDoubleArg(state, 7, 0.75) : 0.75;
 const double max_correction = api.gettop(state) >= 8 ? luaDoubleArg(state, 8, 0.15) : 0.15;
 const double separation = luaDoubleField(state, manifoldIdx, "separation", 0.0);
 const double penetration = std::max(0.0, -separation - slop);
 if(penetration <= 0.0) {
  pushLuaV3(state, position_a);
  pushLuaV3(state, position_b);
  api.pushnumber(state, 0.0);
  return 3;
 }
 const double inv_a = luaDoubleField(state, bodyAIdx, "inv_mass", 0.0);
 const double inv_b = luaDoubleField(state, bodyBIdx, "inv_mass", 0.0);
 const double sum = inv_a + inv_b;
 if(sum <= kEps) {
  pushLuaV3(state, position_a);
  pushLuaV3(state, position_b);
  api.pushnumber(state, 0.0);
  return 3;
 }
 const Vec3 normal = readLuaV3(state, manifoldIdx);
 const double correction = std::min(max_correction, penetration * percent);
 pushLuaV3(state, v3Sub(position_a, v3Scale(normal, correction * inv_a / sum)));
 pushLuaV3(state, v3Add(position_b, v3Scale(normal, correction * inv_b / sum)));
 api.pushnumber(state, correction);
 return 3;
}
} // namespace
void setBox3dPreloaded(lua_State* state) {
 LuaApi& api = luaApi();
 api.rawgeti(state, kLuaRegistryIndex, 2);
 if(api.type(state, -1) != kLuaTTable) {
  api.settop(state, -1);
  return;
 }
 api.getfield(state, -1, "package");
 if(api.type(state, -1) != kLuaTTable) {
  api.settop(state, -2);
  return;
 }
 api.getfield(state, -1, "preload");
 if(api.type(state, -1) != kLuaTTable) {
  api.settop(state, -3);
  return;
 }
 api.pushcclosure(state, [](lua_State* L) -> int {
   installBox3dApi(L);
   return 1;
 }, 0);
 api.setfield(state, -2, "lib.box3d");
 api.settop(state, 0);
}
void installBox3dApi(lua_State* state) {
 LuaApi& api = luaApi();
 api.createtable(state, 0, 32);
 bindFunctions(state,
               {
                   {"v3", luaBox3dV3},
                   {"v3_add", luaBox3dV3Add},
                   {"v3_sub", luaBox3dV3Sub},
                   {"v3_scale", luaBox3dV3Scale},
                   {"v3_dot", luaBox3dV3Dot},
                   {"v3_cross", luaBox3dV3Cross},
                   {"v3_len", luaBox3dV3Len},
                   {"v3_len_sqr", luaBox3dV3LenSqr},
                   {"v3_normalize", luaBox3dV3Normalize},
                   {"v3_lerp", luaBox3dV3Lerp},
                   {"quat_identity", luaBox3dQuatIdentity},
                   {"quat_normalize", luaBox3dQuatNormalize},
                   {"quat_mul", luaBox3dQuatMul},
                   {"quat_rotate", luaBox3dQuatRotate},
                   {"quat_inv_rotate", luaBox3dQuatInvRotate},
                   {"quat_slerp", luaBox3dQuatSlerp},
                   {"quat_to_euler_degrees", luaBox3dQuatToEulerDegrees},
                   {"make_quat_from_axis_angle", luaBox3dMakeQuatFromAxisAngle},
                   {"make_soft", luaBox3dMakeSoft},
                   {"sync_body_cache", luaBox3dSyncBodyCache},
                   {"apply_inv_inertia", luaBox3dApplyInvInertia},
                   {"new_box", luaBox3dNewBox},
                   {"new_voxel_body", luaBox3dNewVoxelBody},
                   {"integrate_rotation", luaBox3dIntegrateRotation},
                   {"body_aabb", luaBox3dBodyAabb},
                   {"box_aabb_manifold", luaBox3dBoxAabbManifold},
                   {"box_box_manifold", luaBox3dBoxBoxManifold},
                   {"sweep_box_aabb", luaBox3dSweepBoxAabb},
                   {"solve_static_manifolds", luaBox3dSolveStaticManifolds},
                   {"solve_pair_manifold", luaBox3dSolvePairManifold},
                   {"correct_static_position", luaBox3dCorrectStaticPosition},
                   {"correct_pair_positions", luaBox3dCorrectPairPositions},
               });
}
} // namespace net::minecraft::mod::runtime
