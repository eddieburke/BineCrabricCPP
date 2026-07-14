#pragma once
#include <vector>
#include "net/minecraft/util/math/Matrix4f.hpp"
namespace net::minecraft::util::math {
class MatrixStack {
public:
  void push() {
    stack_.push_back(stack_.back());
  }
  void pop() {
    if(stack_.size() > 1)
      stack_.pop_back();
  }
  void loadIdentity() {
    stack_.back().identity();
  }
  void translate(float x, float y, float z) {
    stack_.back().translate(x, y, z);
  }
  void scale(float x, float y, float z) {
    stack_.back().scale(x, y, z);
  }
  void rotate(float deg, float x, float y, float z) {
    stack_.back().rotate(deg, x, y, z);
  }
  void multiply(const Matrix4f& mat) {
    stack_.back().multiply(mat);
  }
  void load(const Matrix4f& mat) {
    stack_.back() = mat;
  }
  const Matrix4f& top() const {
    return stack_.back();
  }
  Matrix4f& top() {
    return stack_.back();
  }
  std::size_t size() const {
    return stack_.size();
  }
  MatrixStack() {
    stack_.reserve(32);
    stack_.push_back(Matrix4f{});
  }

private:
  std::vector<Matrix4f> stack_;
};
} // namespace net::minecraft::util::math