#include "net/minecraft/client/render/platform/Lighting.hpp"
#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/util/math/Types.hpp"
namespace net::minecraft::client::render::platform {
namespace {
float* lightBuffer() {
  static float buffer[4];
  return buffer;
}
void setLight(int light, int pname, float r, float g, float b, float a) {
  float* buffer = lightBuffer();
  buffer[0] = r;
  buffer[1] = g;
  buffer[2] = b;
  buffer[3] = a;
  gl::GL11::glLightfv(light, pname, buffer);
}
} // namespace
void Lighting::turnOff() noexcept {
  gl::GL11::glDisable(gl::GL11::GL_LIGHTING);
  gl::GL11::glDisable(gl::GL11::GL_LIGHT0);
  gl::GL11::glDisable(gl::GL11::GL_LIGHT1);
  gl::GL11::glDisable(gl::GL11::GL_COLOR_MATERIAL);
}
void Lighting::turnOn() noexcept {
  gl::GL11::glEnable(gl::GL11::GL_LIGHTING);
  gl::GL11::glEnable(gl::GL11::GL_LIGHT0);
  gl::GL11::glEnable(gl::GL11::GL_LIGHT1);
  gl::GL11::glEnable(gl::GL11::GL_COLOR_MATERIAL);
  gl::GL11::glColorMaterial(gl::GL11::GL_FRONT, gl::GL11::GL_AMBIENT_AND_DIFFUSE);
  constexpr float diffuse = 0.6f;
  constexpr float ambient = 0.4f;
  constexpr float specular = 0.0f;
  const Vec3d sun0 = Vec3d{0.2, 1.0, -0.7}.normalize();
  setLight(gl::GL11::GL_LIGHT0, gl::GL11::GL_POSITION, static_cast<float>(sun0.x), static_cast<float>(sun0.y),
           static_cast<float>(sun0.z), 0.0f);
  setLight(gl::GL11::GL_LIGHT0, gl::GL11::GL_DIFFUSE, diffuse, diffuse, diffuse, 1.0f);
  setLight(gl::GL11::GL_LIGHT0, gl::GL11::GL_AMBIENT, 0.0f, 0.0f, 0.0f, 1.0f);
  setLight(gl::GL11::GL_LIGHT0, gl::GL11::GL_SPECULAR, specular, specular, specular, 1.0f);
  const Vec3d sun1 = Vec3d{-0.2, 1.0, 0.7}.normalize();
  setLight(gl::GL11::GL_LIGHT1, gl::GL11::GL_POSITION, static_cast<float>(sun1.x), static_cast<float>(sun1.y),
           static_cast<float>(sun1.z), 0.0f);
  setLight(gl::GL11::GL_LIGHT1, gl::GL11::GL_DIFFUSE, diffuse, diffuse, diffuse, 1.0f);
  setLight(gl::GL11::GL_LIGHT1, gl::GL11::GL_AMBIENT, 0.0f, 0.0f, 0.0f, 1.0f);
  setLight(gl::GL11::GL_LIGHT1, gl::GL11::GL_SPECULAR, specular, specular, specular, 1.0f);
  gl::GL11::glShadeModel(gl::GL11::GL_FLAT);
  float* modelAmbient = lightBuffer();
  modelAmbient[0] = ambient;
  modelAmbient[1] = ambient;
  modelAmbient[2] = ambient;
  modelAmbient[3] = 1.0f;
  gl::GL11::glLightModelfv(gl::GL11::GL_LIGHT_MODEL_AMBIENT, modelAmbient);
}
} // namespace net::minecraft::client::render::platform
