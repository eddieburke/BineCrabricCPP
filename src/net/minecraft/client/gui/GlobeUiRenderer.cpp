#include "net/minecraft/client/gui/GlobeUiRenderer.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#ifdef MINECRAFT_GL_REAL
#include <GL/glu.h>
#endif
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <sstream>
#include <string_view>
#include <vector>
namespace net::minecraft::client::gui::globe {
namespace {
constexpr float kDegToRad = 3.141592653589793f / 180.0f;
constexpr float kRadToDeg = 180.0f / 3.141592653589793f;
constexpr float kGridRadius = 1.0f;
constexpr float kCoastRadius = 1.014f;
constexpr float kBorderRadius = 1.018f;
constexpr float kPinRadius = 1.034f;
constexpr float kCamDefault = 3.15f;
std::vector<std::vector<float>> gCoastPaths;
std::vector<std::vector<float>> gBorderPaths;
[[nodiscard]] float clampFloat(float value, float minValue, float maxValue) {
  return std::max(minValue, std::min(maxValue, value));
}
void parsePathSegment(const std::string& segment, std::vector<std::vector<float>>& outPaths) {
  if(segment.find(',') == std::string::npos) {
    return;
  }
  std::vector<float> path;
  std::istringstream tokens(segment);
  std::string token;
  while(tokens >> token) {
    const std::size_t comma = token.find(',');
    if(comma == std::string::npos) {
      continue;
    }
    const float lat = std::strtof(token.substr(0, comma).c_str(), nullptr);
    const float lon = std::strtof(token.substr(comma + 1).c_str(), nullptr);
    path.push_back(lat);
    path.push_back(lon);
  }
  if(path.size() >= 4) {
    outPaths.push_back(std::move(path));
  }
}
void parseCoastBlob(const std::string& text, std::vector<std::vector<float>>& outPaths) {
  outPaths.clear();
  std::string current;
  for(const char ch : text) {
    if(ch == '\r') {
      continue;
    }
    if(ch == '\n' || ch == '|') {
      if(!current.empty()) {
        parsePathSegment(current, outPaths);
        current.clear();
      }
      continue;
    }
    current.push_back(ch);
  }
  if(!current.empty()) {
    parsePathSegment(current, outPaths);
  }
}
void vertexLatLon(float latDeg, float lonDeg, float radius) {
  const float lat = latDeg * kDegToRad;
  const float lon = lonDeg * kDegToRad;
  const float cosLat = std::cos(lat);
  const float x = cosLat * std::sin(lon);
  const float y = std::sin(lat);
  const float z = cosLat * std::cos(lon);
  gl::GL11::glVertex3d(static_cast<double>(x * radius), static_cast<double>(y * radius),
                       static_cast<double>(z * radius));
}
void drawSolidSphere(float radius) {
  gl::GL11::glColor4f(0.11f, 0.13f, 0.17f, 1.0f);
  for(int lat = -90; lat < 90; lat += 10) {
    gl::GL11::glBegin(gl::GL11::GL_QUAD_STRIP);
    for(int lon = 0; lon <= 360; lon += 10) {
      vertexLatLon(static_cast<float>(lat), static_cast<float>(lon), radius);
      vertexLatLon(static_cast<float>(lat + 10), static_cast<float>(lon), radius);
    }
    gl::GL11::glEnd();
  }
}
void drawGraticule() {
  gl::GL11::glColor4f(0.38f, 0.42f, 0.5f, 1.0f);
  for(int lat = -60; lat <= 60; lat += 30) {
    if(lat == 0) {
      continue;
    }
    gl::GL11::glBegin(gl::GL11::GL_LINE_LOOP);
    for(int lon = 0; lon <= 360; lon += 6) {
      vertexLatLon(static_cast<float>(lat), static_cast<float>(lon), kGridRadius);
    }
    gl::GL11::glEnd();
  }
  for(int lon = 0; lon < 360; lon += 45) {
    gl::GL11::glBegin(gl::GL11::GL_LINE_STRIP);
    for(int lat = -86; lat <= 86; lat += 6) {
      vertexLatLon(static_cast<float>(lat), static_cast<float>(lon), kGridRadius);
    }
    gl::GL11::glEnd();
  }
  gl::GL11::glColor4f(0.52f, 0.56f, 0.64f, 1.0f);
  gl::GL11::glLineWidth(1.2f);
  gl::GL11::glBegin(gl::GL11::GL_LINE_LOOP);
  for(int lon = 0; lon <= 360; lon += 6) {
    vertexLatLon(0.0f, static_cast<float>(lon), kGridRadius);
  }
  gl::GL11::glEnd();
  gl::GL11::glLineWidth(1.0f);
}
void drawPaths(const std::vector<std::vector<float>>& paths, float radius, float r, float g, float b) {
  gl::GL11::glColor4f(r, g, b, 1.0f);
  for(const std::vector<float>& path : paths) {
    if(path.size() < 4) {
      continue;
    }
    gl::GL11::glBegin(gl::GL11::GL_LINE_STRIP);
    for(std::size_t index = 0; index + 1 < path.size(); index += 2) {
      vertexLatLon(path[index], path[index + 1], radius);
    }
    gl::GL11::glEnd();
  }
}
void drawPin(float pinLat, float pinLon) {
  const float latR = pinLat * kDegToRad;
  const float lonR = pinLon * kDegToRad;
  const float cosLat = std::cos(latR);
  const float x = cosLat * std::sin(lonR);
  const float y = std::sin(latR);
  const float z = cosLat * std::cos(lonR);
  gl::GL11::glPointSize(6.0f);
  gl::GL11::glBegin(gl::GL11::GL_POINTS);
  gl::GL11::glColor4f(0.98f, 0.62f, 0.12f, 1.0f);
  gl::GL11::glVertex3d(static_cast<double>(x * kPinRadius), static_cast<double>(y * kPinRadius),
                       static_cast<double>(z * kPinRadius));
  gl::GL11::glEnd();
  gl::GL11::glBegin(gl::GL11::GL_LINES);
  gl::GL11::glColor4f(0.9f, 0.45f, 0.08f, 1.0f);
  gl::GL11::glVertex3d(0.0, 0.0, 0.0);
  gl::GL11::glVertex3d(static_cast<double>(x * 1.16f), static_cast<double>(y * 1.16f),
                       static_cast<double>(z * 1.16f));
  gl::GL11::glEnd();
}
#ifdef MINECRAFT_GL_REAL
void gluPerspectiveFov(float fovyDeg, float aspect, float zNear, float zFar) {
  ::gluPerspective(static_cast<GLdouble>(fovyDeg), static_cast<GLdouble>(aspect), static_cast<GLdouble>(zNear),
                   static_cast<GLdouble>(zFar));
}
#else
void gluPerspectiveFov(float fovyDeg, float aspect, float zNear, float zFar) {
  (void)fovyDeg;
  (void)aspect;
  (void)zNear;
  (void)zFar;
}
#endif
void applyGlobeRotation(float yawDeg, float pitchDeg) {
  gl::GL11::glRotatef(pitchDeg, 1.0f, 0.0f, 0.0f);
  gl::GL11::glRotatef(yawDeg, 0.0f, 1.0f, 0.0f);
}
void inverseRotateVector(float yawDeg, float pitchDeg, double vx, double vy, double vz, double& ox, double& oy,
                         double& oz) {
  const float yaw = -yawDeg * kDegToRad;
  const float pitch = -pitchDeg * kDegToRad;
  const float cy = std::cos(yaw);
  const float sy = std::sin(yaw);
  const float cp = std::cos(pitch);
  const float sp = std::sin(pitch);
  const double x1 = cy * vx + sy * vz;
  const double z1 = -sy * vx + cy * vz;
  const double y2 = cp * vy - sp * z1;
  const double z2 = sp * vy + cp * z1;
  ox = x1;
  oy = y2;
  oz = z2;
}
[[nodiscard]] bool computeViewport(const DrawParams& params, int displayWidth, int displayHeight, int& vx, int& vy,
                                   int& vw, int& vh) {
  if(params.globeSize < 2 || params.guiWidth < 4 || params.guiHeight < 4 || displayWidth < 4 || displayHeight < 4) {
    return false;
  }
  vx = params.globeX * displayWidth / params.guiWidth;
  vw = params.globeSize * displayWidth / params.guiWidth;
  vh = params.globeSize * displayHeight / params.guiHeight;
  vy = displayHeight - (params.globeY + params.globeSize) * displayHeight / params.guiHeight;
  return vw >= 2 && vh >= 2;
}
} // namespace
void clearCoastPaths() {
  gCoastPaths.clear();
  gBorderPaths.clear();
}
void loadCoastText(const std::string& text) {
  parseCoastBlob(text, gCoastPaths);
}
void render(const DrawParams& params) {
  client::Minecraft* client = client::Minecraft::INSTANCE;
  if(client == nullptr) {
    return;
  }
  client::render::Tessellator& tess = client::render::Tessellator::INSTANCE;
  if(tess.drawing()) {
    tess.draw();
  }
  int savedViewport[4] = {0, 0, 0, 0};
  gl::GL11::glGetIntegerv(gl::GL11::GL_VIEWPORT, savedViewport);
  int vx = 0;
  int vy = 0;
  int vw = 0;
  int vh = 0;
  if(!computeViewport(params, client->displayWidth, client->displayHeight, vx, vy, vw, vh)) {
    return;
  }
  const gl::AttribGuard attribScope(gl::GL11::GL_ALL_ATTRIB_BITS);
  gl::GL11::glDisable(gl::GL11::GL_LIGHTING);
  gl::GL11::glDisable(gl::GL11::GL_FOG);
  gl::GL11::glDisable(gl::GL11::GL_ALPHA_TEST);
  gl::GL11::glDepthMask(true);
  gl::GL11::glEnable(gl::GL11::GL_DEPTH_TEST);
  gl::GL11::glDepthFunc(gl::GL11::GL_LEQUAL);
  gl::GL11::glShadeModel(gl::GL11::GL_FLAT);
  gl::GL11::glEnable(gl::GL11::GL_SCISSOR_TEST);
  gl::GL11::glScissor(vx, vy, vw, vh);
  gl::GL11::glViewport(vx, vy, vw, vh);
  gl::GL11::glClearColor(0.11f, 0.13f, 0.17f, 1.0f);
  gl::GL11::glClearDepth(1.0);
  gl::GL11::glClear(gl::GL11::GL_DEPTH_BUFFER_BIT | gl::GL11::GL_COLOR_BUFFER_BIT);
  gl::GL11::glMatrixMode(gl::GL11::GL_PROJECTION);
  gl::GL11::glPushMatrix();
  gl::GL11::glLoadIdentity();
  gluPerspectiveFov(40.0f, static_cast<float>(vw) / static_cast<float>(vh), 0.05f, 12.0f);
  gl::GL11::glMatrixMode(gl::GL11::GL_MODELVIEW);
  gl::GL11::glPushMatrix();
  gl::GL11::glLoadIdentity();
  const float camDist = clampFloat(params.camDist, 1.5f, 6.0f);
  gl::GL11::glTranslatef(0.0f, 0.0f, -camDist);
  applyGlobeRotation(params.yawDeg, params.pitchDeg);
  gl::GL11::glDisable(gl::GL11::GL_TEXTURE_2D);
  gl::GL11::glLineWidth(1.0f);
  drawSolidSphere(0.99f);
  drawGraticule();
  if(!gBorderPaths.empty()) {
    drawPaths(gBorderPaths, kBorderRadius, 0.42f, 0.47f, 0.56f);
  }
  if(!gCoastPaths.empty()) {
    drawPaths(gCoastPaths, kCoastRadius, 0.72f, 0.76f, 0.84f);
  }
  drawPin(params.pinLat, params.pinLon);
  gl::GL11::glPopMatrix();
  gl::GL11::glMatrixMode(gl::GL11::GL_PROJECTION);
  gl::GL11::glPopMatrix();
  gl::GL11::glMatrixMode(gl::GL11::GL_MODELVIEW);
  gl::GL11::glViewport(savedViewport[0], savedViewport[1], savedViewport[2], savedViewport[3]);
}
bool pickLatLon(const PickParams& params, float& outLat, float& outLon) {
  const int cx = params.globeX + params.globeSize / 2;
  const int cy = params.globeY + params.globeSize / 2;
  const float camDist = clampFloat(params.camDist, 1.5f, 6.0f);
  const double scale = static_cast<double>(params.globeSize) * 0.48 * (static_cast<double>(kCamDefault) / camDist);
  const double dx = (params.mouseX - cx) / scale;
  const double dy = -(params.mouseY - cy) / scale;
  double r2 = dx * dx + dy * dy;
  if(r2 > 1.02) {
    return false;
  }
  if(r2 > 1.0) {
    const double inv = 1.0 / std::sqrt(r2);
    r2 = 1.0;
    const double ndx = dx * inv;
    const double ndy = dy * inv;
    const double vz = std::sqrt(std::max(0.0, 1.0 - r2));
    double wx = 0.0;
    double wy = 0.0;
    double wz = 0.0;
    inverseRotateVector(params.yawDeg, params.pitchDeg, ndx, ndy, vz, wx, wy, wz);
    const double length = std::sqrt(wx * wx + wy * wy + wz * wz);
    if(length < 1.0e-5) {
      return false;
    }
    wx /= length;
    wy /= length;
    wz /= length;
    double lat = std::asin(wy) * static_cast<double>(kRadToDeg);
    lat = clampFloat(static_cast<float>(lat), -90.0f, 90.0f);
    double lon = std::atan2(wx, wz) * static_cast<double>(kRadToDeg);
    if(lon > 180.0) {
      lon -= 360.0;
    }
    if(lon < -180.0) {
      lon += 360.0;
    }
    outLat = static_cast<float>(lat);
    outLon = static_cast<float>(lon);
    return true;
  }
  const double vz = std::sqrt(std::max(0.0, 1.0 - r2));
  double wx = 0.0;
  double wy = 0.0;
  double wz = 0.0;
  inverseRotateVector(params.yawDeg, params.pitchDeg, dx, dy, vz, wx, wy, wz);
  const double length = std::sqrt(wx * wx + wy * wy + wz * wz);
  if(length < 1.0e-5) {
    return false;
  }
  wx /= length;
  wy /= length;
  wz /= length;
  double lat = std::asin(wy) * static_cast<double>(kRadToDeg);
  lat = clampFloat(static_cast<float>(lat), -90.0f, 90.0f);
  double lon = std::atan2(wx, wz) * static_cast<double>(kRadToDeg);
  if(lon > 180.0) {
    lon -= 360.0;
  }
  if(lon < -180.0) {
    lon += 360.0;
  }
  outLat = static_cast<float>(lat);
  outLon = static_cast<float>(lon);
  return true;
}
bool containsGlobePoint(int mouseX, int mouseY, int globeX, int globeY, int globeSize) {
  return mouseX >= globeX && mouseX < globeX + globeSize && mouseY >= globeY && mouseY < globeY + globeSize;
}
} // namespace net::minecraft::client::gui::globe
