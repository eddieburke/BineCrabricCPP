#include "net/minecraft/client/render/ColorSpace.hpp"
#include <string>
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/GlConstants.hpp"
#include "net/minecraft/client/gl/ShaderProgram.hpp"
#include "net/minecraft/client/render/GlState.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"

namespace net::minecraft::client::render {
namespace {

constexpr const char* kPreamble = "#version 330 core\n";
constexpr const char* kVertex = R"GLSL(
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUv;
out vec2 uv;
void main() {
 gl_Position = vec4(aPos, 0.0, 1.0);
 uv = aUv;
}
)GLSL";

std::string fragmentFor(ColorSpace space) {
 std::string src;
 src += "#define CURRENT_COLOR_SPACE " + std::to_string(static_cast<int>(space)) + "\n";
 src += "#define SRGB 0\n#define DCI_P3 1\n#define DISPLAY_P3 2\n#define REC2020 3\n#define ADOBE_RGB 4\n";
 src += R"GLSL(
uniform sampler2D readImage;
in vec2 uv;
out vec4 outColor;

vec3 EOTF_Curve(vec3 LinearCV, const float LinearFactor, const float Exponent, const float Alpha, const float Beta) {
 return mix(LinearCV * LinearFactor, clamp(Alpha * pow(LinearCV, vec3(Exponent)) - (Alpha - 1.0), 0.0, 1.0), step(Beta, LinearCV));
}
vec3 EOTF_IEC61966(vec3 LinearCV) {
 return EOTF_Curve(LinearCV, 12.92, 1.0 / 2.4, 1.055, 0.0031308);
}
vec3 InverseEOTF_IEC61966(vec3 DisplayCV) {
 return max(mix(DisplayCV / 12.92, pow(0.947867 * DisplayCV + 0.0521327, vec3(2.4)), step(0.04045, DisplayCV)), 0.0);
}
vec3 EOTF_BT709(vec3 LinearCV) {
 return EOTF_Curve(LinearCV, 4.5, 0.45, 1.099, 0.018);
}
vec3 EOTF_P3DCI(vec3 LinearCV) {
 return pow(LinearCV, vec3(1.0 / 2.6));
}
vec3 EOTF_Adobe(vec3 LinearCV) {
 return pow(LinearCV, vec3(1.0 / 2.2));
}

const mat3 sRGB_XYZ = mat3(
 0.4124564, 0.3575761, 0.1804375,
 0.2126729, 0.7151522, 0.0721750,
 0.0193339, 0.1191920, 0.9503041
);
const mat3 XYZ_P3D65 = mat3(
 2.4933963, -0.9313459, -0.4026945,
 -0.8294868, 1.7626597, 0.0236246,
 0.0358507, -0.0761827, 0.9570140
);
const mat3 XYZ_REC2020 = mat3(
 1.7166511880, -0.3556707838, -0.2533662814,
 -0.6666843518, 1.6164812366, 0.0157685458,
 0.0176398574, -0.0427706133, 0.9421031212
);
const mat3 XYZ_AdobeRGB = mat3(
 2.04158790381075, -0.56500697427886, -0.34473135077833,
 -0.96924363628088, 1.87596750150772, 0.0415550574071756,
 0.0134442806320311, -0.118362392231018, 1.01517499439121
);
const mat3 D65_DCI = mat3(
 1.02449672775258, 0.0151635410224164, 0.0196885223342068,
 0.0256121933371582, 0.972586305624413, 0.00471635229242733,
 0.00638423065008769, -0.0122680827367302, 1.14794244517368
);
const mat3 sRGB_to_P3DCI = ((sRGB_XYZ) * XYZ_P3D65) * D65_DCI;
const mat3 sRGB_to_P3D65 = sRGB_XYZ * XYZ_P3D65;
const mat3 sRGB_to_REC2020 = sRGB_XYZ * XYZ_REC2020;
const mat3 sRGB_to_AdobeRGB = sRGB_XYZ * XYZ_AdobeRGB;

void main() {
#if CURRENT_COLOR_SPACE == SRGB
 outColor = texture(readImage, uv);
#else
 vec4 SourceColor = texture(readImage, uv);
 SourceColor.rgb = InverseEOTF_IEC61966(SourceColor.rgb);
 vec3 TargetColor = SourceColor.rgb;
#if CURRENT_COLOR_SPACE == DCI_P3
 TargetColor = TargetColor * sRGB_to_P3DCI;
 TargetColor = EOTF_P3DCI(TargetColor);
#elif CURRENT_COLOR_SPACE == DISPLAY_P3
 TargetColor = TargetColor * sRGB_to_P3D65;
 TargetColor = EOTF_IEC61966(TargetColor);
#elif CURRENT_COLOR_SPACE == REC2020
 TargetColor = TargetColor * sRGB_to_REC2020;
 TargetColor = EOTF_BT709(TargetColor);
#elif CURRENT_COLOR_SPACE == ADOBE_RGB
 TargetColor = TargetColor * sRGB_to_AdobeRGB;
 TargetColor = EOTF_Adobe(TargetColor);
#endif
 outColor = vec4(TargetColor, SourceColor.a);
#endif
}
)GLSL";
 return src;
}

constexpr unsigned kTex2D = 0x0DE1;
constexpr unsigned kRgba8 = 0x8058;
constexpr unsigned kRgba = 0x1908;
constexpr unsigned kUByte = 0x1401;
constexpr unsigned kFbo = 0x8D40;
constexpr unsigned kColor0 = 0x8CE0;
constexpr unsigned kNearest = 0x2600;
constexpr unsigned kClamp = 0x812F;

bool allocRgba8(unsigned int& tex, int w, int h) {
 if(tex == 0) tex = static_cast<unsigned int>(core::genTexture());
 if(tex == 0) return false;
 core::bindTexture(static_cast<int>(tex));
 ::glTexImage2D(kTex2D, 0, static_cast<int>(kRgba8), w, h, 0, kRgba, kUByte, nullptr);
 ::glTexParameteri(kTex2D, 0x2801, static_cast<int>(kNearest));
 ::glTexParameteri(kTex2D, 0x2800, static_cast<int>(kNearest));
 ::glTexParameteri(kTex2D, 0x2802, static_cast<int>(kClamp));
 ::glTexParameteri(kTex2D, 0x2803, static_cast<int>(kClamp));
 return true;
}

bool attachColor(unsigned int& fbo, unsigned int tex) {
 if(fbo == 0) gl::GLCore::genFramebuffers(1, &fbo);
 if(fbo == 0 || tex == 0) return false;
 gl::GLCore::bindFramebuffer(kFbo, fbo);
 gl::GLCore::framebufferTexture2D(kFbo, kColor0, kTex2D, tex, 0);
 return true;
}

}

ColorSpaceConverter::~ColorSpaceConverter() {
 destroy();
}

bool ColorSpaceConverter::ready() const noexcept {
 return presentFbo_ != 0 && presentTexture_ != 0 && space_ != ColorSpace::Srgb && program_ &&
        program_->valid();
}

void ColorSpaceConverter::destroy() {
 program_.reset();
 if(presentTexture_ != 0) {
  core::deleteTexture(presentTexture_);
  presentTexture_ = 0;
 }
 if(swapTexture_ != 0) {
  core::deleteTexture(swapTexture_);
  swapTexture_ = 0;
 }
 if(gl::GLCore::deleteFramebuffers != nullptr) {
  if(presentFbo_ != 0) {
   gl::GLCore::deleteFramebuffers(1, &presentFbo_);
   presentFbo_ = 0;
  }
  if(swapFbo_ != 0) {
   gl::GLCore::deleteFramebuffers(1, &swapFbo_);
   swapFbo_ = 0;
  }
 }
 width_ = height_ = 0;
 space_ = ColorSpace::Srgb;
}

bool ColorSpaceConverter::ensureProgram(ColorSpace space) {
 if(program_ && space_ == space && program_->valid()) return true;
 auto program = std::make_unique<gl::ShaderProgram>();
 if(!program->compile(kVertex, fragmentFor(space), kPreamble)) return false;
 program_ = std::move(program);
 space_ = space;
 return true;
}

bool ColorSpaceConverter::ensureTargets(int width, int height) {
 if(width <= 0 || height <= 0) return false;
 if(presentTexture_ != 0 && presentFbo_ != 0 && swapTexture_ != 0 && swapFbo_ != 0 && width_ == width &&
    height_ == height) {
  return true;
 }
 width_ = width;
 height_ = height;
 if(!allocRgba8(presentTexture_, width, height) || !allocRgba8(swapTexture_, width, height)) return false;
 if(!attachColor(presentFbo_, presentTexture_) || !attachColor(swapFbo_, swapTexture_)) return false;
 gl::GLCore::bindFramebuffer(kFbo, 0);
 return true;
}

void ColorSpaceConverter::rebuild(int width, int height, ColorSpace space) {
 if(!glutil::hasGlContext()) return;
 space_ = space;
 if(space == ColorSpace::Srgb) {
  ensureTargets(width, height);
  return;
 }
 if(ensureTargets(width, height)) ensureProgram(space);
}

void ColorSpaceConverter::process(unsigned int targetTexture) {
 if(space_ == ColorSpace::Srgb || targetTexture == 0) return;
 if(!ensureProgram(space_) || !ensureTargets(width_, height_) || !program_ || swapFbo_ == 0) return;

 const core::DepthScope depthScope(false, false);
 const core::CullScope cullScope(false);
 const core::BlendScope blendScope(false);
 const core::TextureBindScope textureScope;

 gl::GLCore::bindFramebuffer(kFbo, swapFbo_);
 core::viewport(0, 0, width_, height_);
 program_->bind();
 core::activeTexture(gl::tex::Texture0);
 core::bindTexture(static_cast<int>(targetTexture));
 program_->set1i("readImage", 0);
 core::drawFullscreen();
 gl::ShaderProgram::unbind();

 gl::GLCore::bindFramebuffer(kFbo, swapFbo_);
 core::bindTexture(static_cast<int>(targetTexture));
 ::glCopyTexSubImage2D(kTex2D, 0, 0, 0, 0, 0, width_, height_);
 gl::GLCore::bindFramebuffer(kFbo, 0);
}

bool ColorSpaceConverter::blitPresentToScreen(int screenWidth, int screenHeight) {
 if(presentFbo_ == 0 || width_ <= 0 || height_ <= 0 || gl::GLCore::blitFramebuffer == nullptr) return false;
 constexpr unsigned kRead = 0x8CA8;
 constexpr unsigned kDraw = 0x8CA9;
 constexpr unsigned kColorBit = 0x00004000;
 gl::GLCore::bindFramebuffer(kRead, presentFbo_);
 gl::GLCore::bindFramebuffer(kDraw, 0);
 core::viewport(0, 0, screenWidth, screenHeight);
 gl::GLCore::blitFramebuffer(0, 0, width_, height_, 0, 0, screenWidth, screenHeight, kColorBit, kNearest);
 gl::GLCore::bindFramebuffer(kRead, 0);
 gl::GLCore::bindFramebuffer(kFbo, 0);
 return true;
}

}
