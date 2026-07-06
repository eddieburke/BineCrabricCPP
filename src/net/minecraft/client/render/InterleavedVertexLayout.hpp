#pragma once
namespace net::minecraft::client::render {
struct InterleavedVertexLayout {
  const void* position = nullptr;
  const void* texCoord = nullptr;
  const void* color = nullptr;
  const void* normal = nullptr;
  int stride = 0;
  bool hasTexture = false;
  bool hasColor = false;
  bool hasNormals = false;
};
void bindGuiVertexLayout(const InterleavedVertexLayout& layout) noexcept;
void unbindGuiVertexLayout() noexcept;
} // namespace net::minecraft::client::render
