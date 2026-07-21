#pragma once
#include <cstddef>
namespace net::minecraft::client::render::quad_index {
// Shared GL_ELEMENT_ARRAY_BUFFER holding the repeating quad->triangle pattern
// (0,1,2, 0,2,3 per quad, uint32). ensure() grows it to cover vertexCount
// vertices of quad data; handle() is 0 until the first successful ensure().
// Main/render thread only.
bool ensure(std::size_t vertexCount);
[[nodiscard]] unsigned handle() noexcept;
} // namespace net::minecraft::client::render::quad_index
