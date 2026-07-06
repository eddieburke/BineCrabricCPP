#include "net/minecraft/client/render/InterleavedVertexLayout.hpp"
#include "net/minecraft/client/gl/GL11.hpp"
namespace net::minecraft::client::render {
void bindGuiVertexLayout(const InterleavedVertexLayout& layout) noexcept {
  gl::GL11::glEnableClientState(gl::GL11::GL_VERTEX_ARRAY);
  gl::GL11::glVertexPointer(3, gl::GL11::GL_FLOAT, layout.stride, layout.position);
  if(layout.hasTexture) {
    gl::GL11::glEnableClientState(gl::GL11::GL_TEXTURE_COORD_ARRAY);
    gl::GL11::glTexCoordPointer(2, gl::GL11::GL_FLOAT, layout.stride, layout.texCoord);
  } else {
    gl::GL11::glDisableClientState(gl::GL11::GL_TEXTURE_COORD_ARRAY);
  }
  if(layout.hasColor) {
    gl::GL11::glEnableClientState(gl::GL11::GL_COLOR_ARRAY);
    gl::GL11::glColorPointer(4, gl::GL11::GL_UNSIGNED_BYTE, layout.stride, layout.color);
  } else {
    gl::GL11::glDisableClientState(gl::GL11::GL_COLOR_ARRAY);
  }
  if(layout.hasNormals) {
    gl::GL11::glEnableClientState(gl::GL11::GL_NORMAL_ARRAY);
    gl::GL11::glNormalPointer(gl::GL11::GL_BYTE, layout.stride, layout.normal);
  } else {
    gl::GL11::glDisableClientState(gl::GL11::GL_NORMAL_ARRAY);
  }
}
void unbindGuiVertexLayout() noexcept {
  gl::GL11::glDisableClientState(gl::GL11::GL_VERTEX_ARRAY);
  gl::GL11::glDisableClientState(gl::GL11::GL_TEXTURE_COORD_ARRAY);
  gl::GL11::glDisableClientState(gl::GL11::GL_COLOR_ARRAY);
  gl::GL11::glDisableClientState(gl::GL11::GL_NORMAL_ARRAY);
}
} // namespace net::minecraft::client::render
