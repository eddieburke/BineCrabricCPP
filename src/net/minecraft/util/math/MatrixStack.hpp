#pragma once
#include <array>
#include <vector>
#include "net/minecraft/util/math/Matrix4f.hpp"
namespace net::minecraft::util::math {
class MatrixStack {
 public:
 static constexpr std::size_t kInlineCapacity = 16;
 void push() {
  if(size_ == kInlineCapacity)
   overflow_.push_back(inline_[kInlineCapacity - 1]);
  else if(size_ > kInlineCapacity)
   overflow_.push_back(overflow_.back());
  else
   inline_[size_] = inline_[size_ - 1];
  ++size_;
 }
 void pop() {
  if(size_ > 1) {
   --size_;
   if(size_ > kInlineCapacity)
    overflow_.pop_back();
  }
 }
 void loadIdentity() {
  at(size_ - 1).identity();
 }
 void translate(float x, float y, float z) {
  at(size_ - 1).translate(x, y, z);
 }
 void scale(float x, float y, float z) {
  at(size_ - 1).scale(x, y, z);
 }
 void rotate(float deg, float x, float y, float z) {
  at(size_ - 1).rotate(deg, x, y, z);
 }
 void multiply(const Matrix4f& mat) {
  at(size_ - 1).multiply(mat);
 }
 void load(const Matrix4f& mat) {
  at(size_ - 1) = mat;
 }
 const Matrix4f& top() const {
  return at(size_ - 1);
 }
 Matrix4f& top() {
  return at(size_ - 1);
 }
 std::size_t size() const {
  return size_;
 }
 MatrixStack() = default;

 private:
 Matrix4f& at(std::size_t index) {
  return index < kInlineCapacity ? inline_[index] : overflow_[index - kInlineCapacity];
 }
 const Matrix4f& at(std::size_t index) const {
  return index < kInlineCapacity ? inline_[index] : overflow_[index - kInlineCapacity];
 }
 std::array<Matrix4f, 16> inline_{};
 std::vector<Matrix4f> overflow_;
 std::size_t size_ = 1;
};
} // namespace net::minecraft::util::math
