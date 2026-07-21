#pragma once
#include <cstdint>
// RenderSystem - the single facade for all fixed-function GL-like state. Call sites
// should use the SEMANTIC helpers (blendAlpha, depthTest, cullBackFaces, alphaTest,
// clearColor, ...) rather than naming raw GL constants. Low-level `int` entry points
// remain for internal use and the handful of unusual blend modes.
namespace net::minecraft::client::render {
class RenderSystem {
 public:
 // --- Matrix stack (legacy global; prefer threading an explicit MatrixStack) ---
 static void pushMatrix();
 static void popMatrix();
 // RAII push/pop of the matrix stack. Canonical home for what a dozen render
 // files each redeclare as a private `MatrixScope`.
 class MatrixScope {
  public:
  MatrixScope() {
   pushMatrix();
  }
  ~MatrixScope() {
   popMatrix();
  }
  MatrixScope(const MatrixScope&) = delete;
  MatrixScope& operator=(const MatrixScope&) = delete;
 };
 static void loadIdentity();
 static void translate(float x, float y, float z);
 static void scale(float x, float y, float z);
 static void rotate(float angle, float x, float y, float z);
 static void matrixMode(int mode);
 static void ortho(double left, double right, double bottom, double top, double zNear, double zFar);
 // --- Blending (semantic) ---
 // Standard alpha blending: src.a over (1 - src.a).
 static void blendAlpha();
 // Additive: result = src + dst (used for glows).
 static void blendAdditive();
 // Modulate by destination alpha (TNT).
 static void blendDstAlpha();
 // Inverted-source-color blend (HUD vignette/fx).
 static void blendInverseColor();
 // Multiply-style blend (HUD fx).
 static void blendMultiply();
 // Escape hatch for any blend mode not covered above.
 static void blendCustom(int src, int dst);
 static void blendFunc(int src, int dst);
 static void defaultBlendFunc();
 static void enableBlend();
 static void disableBlend();
 // --- Depth ---
 static void enableDepthTest();
 static void depthTest(); // enable depth test, LEQUAL
 static void depthTestWrite(bool write); // enable + set depth mask
 static void disableDepthTest();
 static void depthFunc(int func);
 static void depthMask(bool enabled);
 // --- Face culling ---
 static void enableCull();
 static void cullFace(int mode);
 static void cullBackFaces();
 static void cullFrontFaces();
 static void disableCull();
 // --- Polygon offset ---
 static void enablePolygonOffset();
 static void disablePolygonOffset();
 static void polygonOffset(float factor, float units);
 // --- Alpha test (always GREATER) ---
 static void alphaFunc(int func, float ref);
 static void alphaTest(float ref = 0.1f);
 // --- Clear ---
 static void clearColor(float r, float g, float b, float a);
 static void clear(int mask);
 static void clearDepth(double depth);
 // --- Textures ---
 static void activeTexture(int texture);
 static void bindTexture(int texture);
 static void bindTexture(unsigned int texture);
 static void bindTexture(int target, int texture);
 static void unbindTexture(int texture);
 static unsigned int genTexture();
 static void deleteTexture(unsigned int texture);
 static void clearAllocatedTextures();
 static void enableTexture();
 static void disableTexture();
  static void enableLighting();
  static void disableLighting();
  static void setupGui3DLiveLighting();
  static void setupGuiFlatItemLighting();
  // RAII guard that forces lighting OFF for its scope and restores it on destruction.
  class LightingOffGuard {
   public:
    LightingOffGuard() { disableLighting(); }
    ~LightingOffGuard() { enableLighting(); }
    LightingOffGuard(const LightingOffGuard&) = delete;
    LightingOffGuard& operator=(const LightingOffGuard&) = delete;
  };
  // --- Color mask ---
  static void colorMask(bool r, bool g, bool b, bool a);
  // RAII guard that saves/restores color mask.
  class ColorMaskScope {
   public:
    ColorMaskScope(bool r, bool g, bool b, bool a);
    ~ColorMaskScope();
    ColorMaskScope(const ColorMaskScope&) = delete;
    ColorMaskScope& operator=(const ColorMaskScope&) = delete;
   private:
    bool savedR_, savedG_, savedB_, savedA_;
  };
  // --- Fog shadow ---
  static void hintFogEnabled(bool enabled);
  // --- Misc ---
  static void viewport(int x, int y, int width, int height);
  static bool getCachedViewport(int outViewport[4]);
  static void color4f(float r, float g, float b, float a);
  static void color3f(float r, float g, float b);
  static void getFloatv(int pname, float* params);
  static void getIntegerv(int pname, int* params);
  struct StateShadow {
   bool blend = false;
   bool depthTest = false;
   bool cullFace = false;
   bool polygonOffset = false;
   bool depthWrite = true;
   bool texture2D = false;
   bool colorMaskR = true;
   bool colorMaskG = true;
   bool colorMaskB = true;
   bool colorMaskA = true;
   bool fogEnabled = true;
   int blendSrc = 1;
   int blendDst = 0;
   int depthFunc = 0x0201;
   int cullFaceMode = 0x0405;
   float polygonFactor = 0.0f;
   float polygonUnits = 0.0f;
   float clearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
   int viewport[4] = {0, 0, 0, 0};
   bool viewportValid = false;
   int activeTexture = 0;
   unsigned boundTextures[32] = {0};
   float constColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
   float alphaRef = 0.1f;
   int alphaFunc = 0x0204;
  };
 static StateShadow getShadow();
 static void setShadow(const StateShadow& shadow);
};
} // namespace net::minecraft::client::render
