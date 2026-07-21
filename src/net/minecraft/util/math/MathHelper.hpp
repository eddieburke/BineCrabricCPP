#pragma once
// Faithful 1:1 port of net.minecraft.util.math.MathHelper (beta 1.7.3).
//
// Parity-critical: sin/cos read from a precomputed 65536-entry table indexed by
// (int)(value * 10430.378f) & 0xFFFF, NOT from std::sin. Animation, rendering
// and any gameplay that uses MathHelper.sin must match Java's table lookup bit
// for bit, so this owns the real table.
#include <array>
#include <cmath>
#include <cstdint>
#include <string>
namespace net::minecraft::util::math {
class MathHelper {
 public:
 static float sin(float value) {
  return sineTable()[static_cast<int>(value * 10430.378f) & 0xFFFF];
 }
 static float cos(float value) {
  return sineTable()[static_cast<int>(value * 10430.378f + 16384.0f) & 0xFFFF];
 }
 static float sqrt(float value) {
  return static_cast<float>(std::sqrt(static_cast<double>(value)));
 }
 static float sqrt(double value) {
  return static_cast<float>(std::sqrt(value));
 }
 static int floor(float value) {
  int n = static_cast<int>(value);
  return value < static_cast<float>(n) ? n - 1 : n;
 }
 static int floor(double value) {
  int n = static_cast<int>(value);
  return value < static_cast<double>(n) ? n - 1 : n;
 }
 static float abs(float value) {
  return value >= 0.0f ? value : -value;
 }
 static double absMax(double a, double b) {
  if(a < 0.0) {
   a = -a;
  }
  if(b < 0.0) {
   b = -b;
  }
  return a > b ? a : b;
 }
 static int floorDiv(int dividend, int divisor) {
  if(dividend < 0) {
   return -((-dividend - 1) / divisor) - 1;
  }
  return dividend / divisor;
 }
 // Java Math.floorMod parity — result has the sign of the divisor, not the dividend.
 static int floorMod(int dividend, int divisor) {
  const int mod = dividend % divisor;
  return mod >= 0 ? mod : mod + divisor;
 }
 static bool isNullOrEmpty(const std::string& text) {
  return text.empty();
 }

 private:
 // Meyer's singleton replaces Java's static initializer block. Built once on
 // first use; values are (float)Math.sin(i * 2pi / 65536).
 //
 // Note: table is declared separately from initialization to avoid a 256KB
 // stack-allocated temporary in the lambda, which would overflow at call-time.
 static const std::array<float, 65536>& sineTable() {
  static std::array<float, 65536> table;
  static const bool kInit = [] {
   constexpr double kPi = 3.14159265358979323846;
   for(int i = 0; i < 65536; ++i) {
    table[static_cast<std::size_t>(i)] =
        static_cast<float>(std::sin(static_cast<double>(i) * kPi * 2.0 / 65536.0));
   }
   return true;
  }();
  (void)kInit;
  return table;
 }
};
} // namespace net::minecraft::util::math
namespace net::minecraft {
using util::math::MathHelper;
}
