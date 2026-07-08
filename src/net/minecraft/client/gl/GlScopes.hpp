#pragma once
#include "net/minecraft/client/gl/GlDraw.hpp"
#include <initializer_list>
namespace net::minecraft::client::gl {
class CapScope {
public:
  CapScope(std::initializer_list<int> caps) {
    for(int cap : caps) {
      if(count_ < kMax) {
        entries_[count_++] = Entry{cap, capEnabled(cap)};
      }
    }
  }
  CapScope(const CapScope&) = delete;
  CapScope& operator=(const CapScope&) = delete;
  ~CapScope() {
    for(int i = count_ - 1; i >= 0; --i) {
      setCap(entries_[i].cap, entries_[i].enabled);
    }
  }

private:
  static constexpr int kMax = 12;
  struct Entry {
    int cap;
    bool enabled;
  };
  Entry entries_[kMax]{};
  int count_ = 0;
};
class DepthMaskScope {
public:
  DepthMaskScope() {
    GLboolean value = GL_TRUE;
    ::glGetBooleanv(GL_DEPTH_WRITEMASK, &value);
    previous_ = value == GL_TRUE;
  }
  DepthMaskScope(const DepthMaskScope&) = delete;
  DepthMaskScope& operator=(const DepthMaskScope&) = delete;
  ~DepthMaskScope() { depthMask(previous_); }

private:
  bool previous_ = true;
};
class ShadeModelScope {
public:
  ShadeModelScope() {
    GLint mode = shade::Flat;
    ::glGetIntegerv(query::ShadeModel, &mode);
    previous_ = static_cast<int>(mode);
  }
  ShadeModelScope(const ShadeModelScope&) = delete;
  ShadeModelScope& operator=(const ShadeModelScope&) = delete;
  ~ShadeModelScope() { shadeModel(previous_); }

private:
  int previous_ = shade::Flat;
};
class BlendFuncScope {
public:
  BlendFuncScope() {
    GLint src = blend::One;
    GLint dst = blend::Zero;
    ::glGetIntegerv(query::BlendSrc, &src);
    ::glGetIntegerv(query::BlendDst, &dst);
    src_ = static_cast<int>(src);
    dst_ = static_cast<int>(dst);
  }
  BlendFuncScope(const BlendFuncScope&) = delete;
  BlendFuncScope& operator=(const BlendFuncScope&) = delete;
  ~BlendFuncScope() { blendFunc(src_, dst_); }

private:
  int src_ = blend::One;
  int dst_ = blend::Zero;
};
class DepthFuncScope {
public:
  DepthFuncScope() {
    GLint mode = compare::Lequal;
    ::glGetIntegerv(query::DepthFunc, &mode);
    previous_ = static_cast<int>(mode);
  }
  DepthFuncScope(const DepthFuncScope&) = delete;
  DepthFuncScope& operator=(const DepthFuncScope&) = delete;
  ~DepthFuncScope() { depthFunc(previous_); }

private:
  int previous_ = compare::Lequal;
};
class AlphaFuncScope {
public:
  AlphaFuncScope() {
    GLint func = compare::Greater;
    ::glGetIntegerv(query::AlphaFunc, &func);
    ::glGetFloatv(query::AlphaRef, &ref_);
    func_ = static_cast<int>(func);
  }
  AlphaFuncScope(const AlphaFuncScope&) = delete;
  AlphaFuncScope& operator=(const AlphaFuncScope&) = delete;
  ~AlphaFuncScope() { alphaFunc(func_, ref_); }

private:
  int func_ = compare::Greater;
  float ref_ = 0.1f;
};
class BoundTextureScope {
public:
  BoundTextureScope() {
    int bound = 0;
    getIntegerv(tex::Binding2D, &bound);
    previous_ = static_cast<unsigned>(bound);
  }
  BoundTextureScope(const BoundTextureScope&) = delete;
  BoundTextureScope& operator=(const BoundTextureScope&) = delete;
  ~BoundTextureScope() { bindTexture(cap::Texture2D, static_cast<int>(previous_)); }

private:
  unsigned previous_ = 0;
};
class ClientArrayBind {
public:
  ClientArrayBind(const void* position, const void* texCoord, const void* color, const void* normal, int stride,
                  bool hasTexture, bool hasColor, bool hasNormals) {
    setCap(client_array::VertexArray, true);
    vertexPointer(3, pixel::Float, stride, position);
    if(hasTexture) {
      setCap(client_array::TextureCoordArray, true);
      texCoordPointer(2, pixel::Float, stride, texCoord);
    } else {
      setCap(client_array::TextureCoordArray, false);
    }
    if(hasColor) {
      setCap(client_array::ColorArray, true);
      colorPointer(4, pixel::UnsignedByte, stride, color);
    } else {
      setCap(client_array::ColorArray, false);
    }
    if(hasNormals) {
      setCap(client_array::NormalArray, true);
      normalPointer(pixel::Byte, stride, normal);
    } else {
      setCap(client_array::NormalArray, false);
    }
  }
  ClientArrayBind(const ClientArrayBind&) = delete;
  ClientArrayBind& operator=(const ClientArrayBind&) = delete;
  ~ClientArrayBind() {
    setCap(client_array::VertexArray, false);
    setCap(client_array::TextureCoordArray, false);
    setCap(client_array::ColorArray, false);
    setCap(client_array::NormalArray, false);
  }
};
namespace pass {
inline void applyOrthoProjection(const util::UiScale& scale) {
  matrixMode(matrix_::Projection);
  loadIdentity();
  ortho(0.0, scale.rawWidth, scale.rawHeight, 0.0, 1000.0, 3000.0);
  matrixMode(matrix_::ModelView);
  loadIdentity();
  translatef(0.0f, 0.0f, -2000.0f);
}
inline void applyHudEnables() {
  disable(cap::CullFace);
  enable(cap::Texture2D);
  enable(cap::AlphaTest);
  alphaFunc(compare::Greater, 0.1f);
  color4f(1.0f, 1.0f, 1.0f, 1.0f);
}
inline void applyScreenEnables() {
  disable(cap::CullFace);
  disable(cap::Lighting);
  disable(cap::Fog);
  enable(cap::Texture2D);
  enable(cap::AlphaTest);
  alphaFunc(compare::Greater, 0.1f);
  color4f(1.0f, 1.0f, 1.0f, 1.0f);
}
inline void bindAtlas2D(int textureId) {
  enable(cap::Texture2D);
  bindTexture(cap::Texture2D, static_cast<unsigned>(textureId));
  color4f(1.0f, 1.0f, 1.0f, 1.0f);
}
inline void beginHud(const util::UiScale& scale) {
  clear(attrib::DepthBufferBit);
  applyOrthoProjection(scale);
  applyHudEnables();
}
inline void beginScreen(const util::UiScale& scale, int viewportW, int viewportH) {
  viewport(0, 0, viewportW, viewportH);
  applyScreenEnables();
  clear(attrib::DepthBufferBit);
  applyOrthoProjection(scale);
  clear(attrib::DepthBufferBit);
}
} // namespace pass
} // namespace net::minecraft::client::gl
