#include "net/minecraft/client/render/shaders/CustomUniforms.hpp"
#include <algorithm>
#include <bit>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <random>
#include <string_view>
#include <utility>
#include "net/minecraft/client/gl/ShaderProgram.hpp"
#include "net/minecraft/client/render/shaderpack/BiomeTables.hpp"
namespace net::minecraft::client::render {
namespace {
constexpr float kPi = 3.14159265358979323846f;
constexpr float kLn2 = 0.69314718055994530942f;
enum class TokenKind {
 End,
 Number,
 Ident,
 Plus,
 Minus,
 Star,
 Slash,
 Percent,
 Bang,
 AmpAmp,
 PipePipe,
 EqEq,
 BangEq,
 Lt,
 Le,
 Gt,
 Ge,
 LParen,
 RParen,
 Comma,
 Dot
};
struct Token {
 TokenKind kind = TokenKind::End;
 std::string text;
 double number = 0.0;
};
struct Lexer {
 explicit Lexer(std::string_view source) : src_(source) {}
 Token next() {
  const Token token = lex();
  memberIndex_ = token.kind == TokenKind::Dot && afterValue_;
  afterValue_ = token.kind == TokenKind::Ident || token.kind == TokenKind::Number ||
                token.kind == TokenKind::RParen;
  return token;
 }
 Token lex() {
  while(i_ < src_.size() && std::isspace(static_cast<unsigned char>(src_[i_]))) ++i_;
  if(i_ >= src_.size()) return {TokenKind::End, {}, 0.0f};
  const char c = src_[i_];
  if(c == '.' && afterValue_) {
   ++i_;
   return {TokenKind::Dot, ".", 0.0f};
  }
  if(std::isdigit(static_cast<unsigned char>(c)) ||
     (c == '.' && i_ + 1 < src_.size() && std::isdigit(static_cast<unsigned char>(src_[i_ + 1])))) {
   std::size_t end = i_;
   bool seenDot = false;
   while(end < src_.size()) {
    if(std::isdigit(static_cast<unsigned char>(src_[end]))) {
     ++end;
     continue;
    }
    if(src_[end] == '.' && !seenDot && !memberIndex_) {
     seenDot = true;
     ++end;
     continue;
    }
    break;
   }
   const std::string text(src_.substr(i_, end - i_));
   i_ = end;
   return {TokenKind::Number, text, std::strtod(text.c_str(), nullptr)};
  }
  if(std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
   std::size_t end = i_ + 1;
   while(end < src_.size() && (std::isalnum(static_cast<unsigned char>(src_[end])) || src_[end] == '_')) ++end;
   const std::string text(src_.substr(i_, end - i_));
   i_ = end;
   return {TokenKind::Ident, text, 0.0f};
  }
  auto two = [&](char a, char b, TokenKind kind) -> bool {
   if(c == a && i_ + 1 < src_.size() && src_[i_ + 1] == b) {
    i_ += 2;
    pending_ = {kind, std::string{a, b}, 0.0f};
    return true;
   }
   return false;
  };
  if(two('&', '&', TokenKind::AmpAmp) || two('|', '|', TokenKind::PipePipe) || two('=', '=', TokenKind::EqEq) ||
     two('!', '=', TokenKind::BangEq) || two('<', '=', TokenKind::Le) || two('>', '=', TokenKind::Ge)) {
   return pending_;
  }
  ++i_;
  switch(c) {
  case '+': return {TokenKind::Plus, "+", 0.0f};
  case '-': return {TokenKind::Minus, "-", 0.0f};
  case '*': return {TokenKind::Star, "*", 0.0f};
  case '/': return {TokenKind::Slash, "/", 0.0f};
  case '%': return {TokenKind::Percent, "%", 0.0f};
  case '!': return {TokenKind::Bang, "!", 0.0f};
  case '<': return {TokenKind::Lt, "<", 0.0f};
  case '>': return {TokenKind::Gt, ">", 0.0f};
  case '(': return {TokenKind::LParen, "(", 0.0f};
  case ')': return {TokenKind::RParen, ")", 0.0f};
  case ',': return {TokenKind::Comma, ",", 0.0f};
  case '.': return {TokenKind::Dot, ".", 0.0f};
  default:
   throw std::string(std::string("unexpected character '") + c + "'");
  }
 }
 std::string_view src_;
 std::size_t i_ = 0;
 Token pending_{};
 bool afterValue_ = false;
 bool memberIndex_ = false;
};
enum class NodeKind { Lit,
                      Ident,
                      Unary,
                      Binary,
                      Call,
                      Construct,
                      Swizzle,
                      MatrixElem };
struct ExprNode {
 NodeKind kind = NodeKind::Lit;
 CustomUniformType type = CustomUniformType::Float;
 std::string name;
 double number = 0.0;
 CustomUniformType resultType = CustomUniformType::Float;
 CustomUniformType operationType = CustomUniformType::Float;
 const CustomUniformValue* customValue = nullptr;
 bool numericLiteral = false;
 bool smoothHasId = false;
 float smoothValue[4]{};
 bool smoothInitialized = false;
 std::vector<CustomUniformValue> args;
 int component = 0;
 int matrixCol = 0;
 TokenKind op = TokenKind::End;
 std::vector<std::unique_ptr<ExprNode>> kids;
};
CustomUniformValue makeFloat(float v) {
 CustomUniformValue out;
 out.type = CustomUniformType::Float;
 out.f[0] = v;
 return out;
}
CustomUniformValue makeInt(int v) {
 CustomUniformValue out;
 out.type = CustomUniformType::Int;
 out.i = v;
 out.f[0] = static_cast<float>(v);
 return out;
}
CustomUniformValue makeBool(bool v) {
 CustomUniformValue out;
 out.type = CustomUniformType::Bool;
 out.b = v;
 out.i = v ? 1 : 0;
 out.f[0] = v ? 1.0f : 0.0f;
 return out;
}
CustomUniformValue makeVec(CustomUniformType type, const float* v) {
 CustomUniformValue out;
 out.type = type;
 const int n = type == CustomUniformType::Vec2 ? 2 : type == CustomUniformType::Vec3 ? 3
                                                                                     : 4;
 for(int i = 0; i < n; ++i) out.f[i] = v[i];
 return out;
}
bool isVector(CustomUniformType t) {
 return t == CustomUniformType::Vec2 || t == CustomUniformType::Vec3 || t == CustomUniformType::Vec4;
}
int vecLen(CustomUniformType t) {
 return t == CustomUniformType::Vec2 ? 2 : t == CustomUniformType::Vec3 ? 3
                                       : t == CustomUniformType::Vec4   ? 4
                                                                        : 1;
}
float componentOf(const CustomUniformValue& v, int index) {
 if(v.type == CustomUniformType::Int) return static_cast<float>(v.i);
 if(v.type == CustomUniformType::Bool) return v.b ? 1.0f : 0.0f;
 return v.f[std::clamp(index, 0, 3)];
}
struct EvalContext {
 const PackUniformValues* frame = nullptr;
 const std::unordered_map<std::string, std::string>* options = nullptr;
 std::mt19937* rng = nullptr;
};
bool lookupConstant(const std::string& name, CustomUniformValue& out) {
 static const std::unordered_map<std::string, int> kBiomes = [] {
  std::unordered_map<std::string, int> map;
  for(std::size_t i = 0; i < kBiomeNames.size(); ++i) {
   map.emplace(kBiomeNames[i], static_cast<int>(i));
   map.emplace("BIOME_" + std::string(kBiomeNames[i]), static_cast<int>(i));
  }
  for(const BiomeNameAlias& alias : kBiomeNameAliases) {
   map.emplace(alias.name, static_cast<int>(alias.index));
   map.emplace("BIOME_" + std::string(alias.name), static_cast<int>(alias.index));
  }
  return map;
 }();
 static const std::unordered_map<std::string, int> kCats = [] {
  std::unordered_map<std::string, int> map;
  for(std::size_t i = 0; i < kBiomeCategoryNames.size(); ++i) {
   map.emplace(kBiomeCategoryNames[i], static_cast<int>(i));
   map.emplace("CAT_" + std::string(kBiomeCategoryNames[i]), static_cast<int>(i));
   map.emplace("BIOME_CATEGORY_" + std::string(kBiomeCategoryNames[i]), static_cast<int>(i));
  }
  return map;
 }();
 if(const auto it = kBiomes.find(name); it != kBiomes.end()) {
  out = makeInt(it->second);
  return true;
 }
 if(const auto it = kCats.find(name); it != kCats.end()) {
  out = makeInt(it->second);
  return true;
 }
 return false;
}
bool matrixNamed(const std::string& name, const PackUniformValues& frame, const float*& out) {
 if(name == "gbufferProjection") {
  out = frame.gbufferProjection;
  return true;
 }
 if(name == "gbufferProjectionInverse") {
  out = frame.gbufferProjectionInverse;
  return true;
 }
 if(name == "gbufferPreviousProjection") {
  out = frame.gbufferPreviousProjection;
  return true;
 }
 if(name == "gbufferModelView") {
  out = frame.gbufferModelView;
  return true;
 }
 if(name == "gbufferModelViewInverse") {
  out = frame.gbufferModelViewInverse;
  return true;
 }
 if(name == "gbufferPreviousModelView") {
  out = frame.gbufferPreviousModelView;
  return true;
 }
 if(name == "shadowModelView") {
  out = frame.shadowModelView;
  return true;
 }
 if(name == "shadowModelViewInverse") {
  out = frame.shadowModelViewInverse;
  return true;
 }
 if(name == "shadowProjection") {
  out = frame.shadowProjection;
  return true;
 }
 if(name == "shadowProjectionInverse") {
  out = frame.shadowProjectionInverse;
  return true;
 }
 return false;
}
bool lookupBuiltin(const std::string& name, const PackUniformValues& frame, CustomUniformValue& out) {
 if(lookupConstant(name, out)) return true;
 auto f1 = [&](float v) {
  out = makeFloat(v);
  return true;
 };
 auto i1 = [&](int v) {
  out = makeInt(v);
  return true;
 };
 auto v3 = [&](const float* v) {
  out = makeVec(CustomUniformType::Vec3, v);
  return true;
 };
 auto v4 = [&](const float* v) {
  out = makeVec(CustomUniformType::Vec4, v);
  return true;
 };
 auto v2i = [&](const int* v) {
  const float f[2] = {static_cast<float>(v[0]), static_cast<float>(v[1])};
  out = makeVec(CustomUniformType::Vec2, f);
  return true;
 };
 if(name == "frameTimeCounter") return f1(frame.frameTimeCounter);
 if(name == "frameTime") return f1(frame.frameTime);
 if(name == "frameCounter") return i1(frame.frameCounter);
 if(name == "viewWidth") return f1(frame.viewWidth);
 if(name == "viewHeight") return f1(frame.viewHeight);
 if(name == "aspectRatio") return f1(frame.aspectRatio);
 if(name == "near") return f1(frame.nearPlane);
 if(name == "far") return f1(frame.renderDistanceBlocks);
 if(name == "shadowMapResolution") return f1(frame.shadowMapResolution);
 if(name == "cameraPosition") return v3(frame.cameraPosition);
 if(name == "cameraPositionFract") return v3(frame.cameraPositionFract);
 if(name == "cameraPositionInt") {
  const float f[3] = {static_cast<float>(frame.cameraPositionInt[0]),
                      static_cast<float>(frame.cameraPositionInt[1]),
                      static_cast<float>(frame.cameraPositionInt[2])};
  out = makeVec(CustomUniformType::Vec3, f);
  return true;
 }
 if(name == "previousCameraPosition") return v3(frame.previousCameraPosition);
 if(name == "previousCameraPositionFract") return v3(frame.previousCameraPositionFract);
 if(name == "previousCameraPositionInt") {
  const float f[3] = {static_cast<float>(frame.previousCameraPositionInt[0]),
                      static_cast<float>(frame.previousCameraPositionInt[1]),
                      static_cast<float>(frame.previousCameraPositionInt[2])};
  out = makeVec(CustomUniformType::Vec3, f);
  return true;
 }
 if(name == "sunPosition") return v3(frame.sunPosition);
 if(name == "moonPosition") return v3(frame.moonPosition);
 if(name == "shadowLightPosition") return v3(frame.shadowLightPosition);
 if(name == "upPosition") return v3(frame.upPosition);
 if(name == "sunColor") return v3(frame.sunColor);
 if(name == "sunIntensity") return f1(frame.sunIntensity);
 if(name == "fogColor") return v3(frame.fogColor);
 if(name == "fogDensity") return f1(frame.fogDensity);
 if(name == "fogStart") return f1(frame.fogStart);
 if(name == "fogEnd") return f1(frame.fogEnd);
 if(name == "fogMode") return i1(frame.fogMode);
 if(name == "fogShape") return i1(frame.fogShape);
 if(name == "skyColor") return v3(frame.skyColor);
 if(name == "lightningBoltPosition") return v4(frame.lightningBoltPosition);
 if(name == "thunderStrength") return f1(frame.thunderStrength);
 if(name == "rainStrength") return f1(frame.rainStrength);
 if(name == "wetness") return f1(frame.wetness);
 if(name == "eyeAltitude") return f1(frame.eyeAltitude);
 if(name == "eyeBrightness") return v2i(frame.eyeBrightness);
 if(name == "eyeBrightnessSmooth") return v2i(frame.eyeBrightnessSmooth);
 if(name == "centerDepthSmooth") return f1(frame.centerDepthSmooth);
 if(name == "nightVision") return f1(frame.nightVision);
 if(name == "blindness") return f1(frame.blindness);
 if(name == "darknessFactor") return f1(frame.darknessFactor);
 if(name == "darknessLightFactor") return f1(frame.darknessLightFactor);
 if(name == "playerMood") return f1(frame.playerMood);
 if(name == "constantMood") return f1(frame.constantMood);
 if(name == "screenBrightness") return f1(frame.screenBrightness);
 if(name == "sunAngle") return f1(frame.sunAngle);
 if(name == "shadowAngle") return f1(frame.shadowAngle);
 if(name == "endFlashPosition") return v3(frame.endFlashPosition);
 if(name == "endFlashIntensity") return f1(frame.endFlashIntensity);
 if(name == "previousEndFlashIntensity") return f1(frame.previousEndFlashIntensity);
 if(name == "ambientLight") return f1(frame.ambientLight);
 if(name == "cloudHeight") return f1(frame.cloudHeight);
 if(name == "rainfall") return f1(frame.rainfall);
 if(name == "temperature") return f1(frame.temperature);
 if(name == "eyePosition") return v3(frame.eyePosition);
 if(name == "relativeEyePosition") return v3(frame.relativeEyePosition);
 if(name == "playerLookVector") return v3(frame.playerLookVector);
 if(name == "playerBodyVector") return v3(frame.playerBodyVector);
 if(name == "vehicleLookVector") return v3(frame.vehicleLookVector);
 if(name == "relativeVehiclePosition") return v3(frame.relativeVehiclePosition);
 if(name == "firstPersonCamera") return i1(frame.firstPersonCamera);
 if(name == "isSpectator") return i1(frame.isSpectator);
 if(name == "isRightHanded") return i1(frame.isRightHanded);
 if(name == "bedrockLevel") return i1(frame.bedrockLevel);
 if(name == "heightLimit") return i1(frame.heightLimit);
 if(name == "hasCeiling") return i1(frame.hasCeiling);
 if(name == "hasSkylight") return i1(frame.hasSkylight);
 if(name == "logicalHeightLimit") return i1(frame.logicalHeightLimit);
 if(name == "is_sneaking" || name == "isSneaking") return i1(frame.isSneaking);
 if(name == "is_sprinting" || name == "isSprinting") return i1(frame.isSprinting);
 if(name == "is_hurt" || name == "isHurt") return i1(frame.isHurt);
 if(name == "is_invisible" || name == "isInvisible") return i1(frame.isInvisible);
 if(name == "is_burning" || name == "isBurning") return i1(frame.isBurning);
 if(name == "is_on_ground" || name == "isOnGround") return i1(frame.isOnGround);
 if(name == "isRiding") return i1(frame.isRiding);
 if(name == "vehicleInWater") return i1(frame.vehicleInWater);
 if(name == "inSwimmingAnimation") return i1(frame.inSwimmingAnimation);
 if(name == "feetInWater") return i1(frame.feetInWater);
 if(name == "isElytraFlying") return i1(frame.isElytraFlying);
 if(name == "hideGUI") return i1(frame.hideGUI);
 if(name == "currentColorSpace") return i1(frame.currentColorSpace);
 if(name == "textureFilteringMode") return i1(frame.textureFilteringMode);
 if(name == "biome") return i1(frame.biome);
 if(name == "biome_category") return i1(frame.biomeCategory);
 if(name == "biome_precipitation") return i1(frame.biomePrecipitation);
 if(name == "currentDate") {
  const float f[3] = {static_cast<float>(frame.currentDate[0]), static_cast<float>(frame.currentDate[1]),
                      static_cast<float>(frame.currentDate[2])};
  out = makeVec(CustomUniformType::Vec3, f);
  return true;
 }
 if(name == "currentTime") {
  const float f[3] = {static_cast<float>(frame.currentTime[0]), static_cast<float>(frame.currentTime[1]),
                      static_cast<float>(frame.currentTime[2])};
  out = makeVec(CustomUniformType::Vec3, f);
  return true;
 }
 if(name == "currentYearTime") return v2i(frame.currentYearTime);
 if(name == "heldItemId") return i1(frame.heldItemId);
 if(name == "heldItemId2") return i1(frame.heldItemId2);
 if(name == "heldBlockLightValue") return i1(frame.heldBlockLightValue);
 if(name == "heldBlockLightValue2") return i1(frame.heldBlockLightValue2);
 if(name == "handLightPackedLR") return i1(frame.handLightPackedLR);
 if(name == "vehicleId") return i1(frame.vehicleId);
 if(name == "currentRenderedItemId") return i1(frame.currentRenderedItemId);
 if(name == "currentSelectedBlockId") return i1(frame.currentSelectedBlockId);
 if(name == "currentSelectedBlockPos") return v3(frame.currentSelectedBlockPos);
 if(name == "worldTime") return i1(frame.worldTime);
 if(name == "worldDay") return i1(frame.worldDay);
 if(name == "moonPhase") return i1(frame.moonPhase);
 if(name == "isEyeInWater") return i1(frame.isEyeInWater);
 if(name == "shadowAvailable") return i1(frame.shadowAvailable);
 if(name == "normalAvailable") return i1(frame.normalAvailable);
 if(name == "currentPlayerHealth") return f1(frame.currentPlayerHealth);
 if(name == "maxPlayerHealth") return f1(frame.maxPlayerHealth);
 if(name == "currentPlayerAir") return f1(frame.currentPlayerAir);
 if(name == "maxPlayerAir") return f1(frame.maxPlayerAir);
 if(name == "currentPlayerHunger") return f1(frame.currentPlayerHunger);
 if(name == "maxPlayerHunger") return f1(frame.maxPlayerHunger);
 if(name == "currentPlayerArmor") return f1(frame.currentPlayerArmor);
 if(name == "maxPlayerArmor") return f1(frame.maxPlayerArmor);
 if(name == "entityId") return i1(frame.entityId);
 if(name == "blockEntityId") return i1(frame.blockEntityId);
 if(name == "anisotropicFiltering") return i1(frame.anisotropicFiltering);
 if(name == "heldBlockLightColor") return v3(frame.heldBlockLightColor);
 if(name == "heldBlockLightColor2") return v3(frame.heldBlockLightColor2);
 if(name == "entityColor") return v4(frame.entityColor);
 if(name == "chunkFadeTimeInv") return f1(frame.chunkFadeTimeInv);
 if(name == "seaLevel") return i1(frame.seaLevel);
 if(name == "cloudTime") return f1(frame.cloudTime);
 if(name == "heavyFog") return i1(frame.heavyFog);
 if(name == "timeAngle") return f1(frame.timeAngle);
 if(name == "timeBrightness") return f1(frame.timeBrightness);
 if(name == "moonBrightness") return f1(frame.moonBrightness);
 if(name == "shadowFade") return f1(frame.shadowFade);
 if(name == "rainStrengthS") return f1(frame.rainStrengthS);
 if(name == "rainStrengthShiningStars") return f1(frame.rainStrengthShiningStars);
 if(name == "rainStrengthS2") return f1(frame.rainStrengthS2);
 if(name == "blindFactor") return f1(frame.blindFactor);
 if(name == "isDry") return f1(frame.isDry);
 if(name == "isRainy") return f1(frame.isRainy);
 if(name == "isSnowy") return f1(frame.isSnowy);
 if(name == "isEyeInCave") return f1(frame.isEyeInCave);
 if(name == "velocity") return f1(frame.velocity);
 if(name == "starter") return f1(frame.starter);
 if(name == "frameTimeSmooth") return f1(frame.frameTimeSmooth);
 if(name == "eyeBrightnessM") return f1(frame.eyeBrightnessM);
 if(name == "rainFactor") return f1(frame.rainFactor);
 if(name == "BiomeTemp") return f1(frame.biomeTemp);
 if(name == "day") return f1(frame.day);
 if(name == "night") return f1(frame.night);
 if(name == "dawnDusk") return f1(frame.dawnDusk);
 if(name == "shdFade") return f1(frame.shdFade);
 if(name == "isPrecipitationRain") return f1(frame.isPrecipitationRain);
 if(name == "touchmybody") return f1(frame.touchmybody);
 if(name == "sneakSmooth") return f1(frame.sneakSmooth);
 if(name == "burningSmooth") return f1(frame.burningSmooth);
 if(name == "effectStrength") return f1(frame.effectStrength);
 return false;
}
CustomUniformValue applyBinary(TokenKind op, const CustomUniformValue& a, const CustomUniformValue& b) {
 if(op == TokenKind::AmpAmp) return makeBool(a.asBool() && b.asBool());
 if(op == TokenKind::PipePipe) return makeBool(a.asBool() || b.asBool());
 if(op == TokenKind::EqEq) {
  if(isVector(a.type) || isVector(b.type)) {
   const int n = std::max(vecLen(a.type), vecLen(b.type));
   for(int i = 0; i < n; ++i) {
    if(componentOf(a, i) != componentOf(b, i)) return makeBool(false);
   }
   return makeBool(true);
  }
  if(a.type == CustomUniformType::Int && b.type == CustomUniformType::Int) return makeBool(a.i == b.i);
  return makeBool(a.asFloat() == b.asFloat());
 }
 if(op == TokenKind::BangEq) return makeBool(!applyBinary(TokenKind::EqEq, a, b).asBool());
 if(a.type == CustomUniformType::Int && b.type == CustomUniformType::Int) {
  if(op == TokenKind::Lt) return makeBool(a.i < b.i);
  if(op == TokenKind::Le) return makeBool(a.i <= b.i);
  if(op == TokenKind::Gt) return makeBool(a.i > b.i);
  if(op == TokenKind::Ge) return makeBool(a.i >= b.i);
 }
 if(op == TokenKind::Lt) return makeBool(a.asFloat() < b.asFloat());
 if(op == TokenKind::Le) return makeBool(a.asFloat() <= b.asFloat());
 if(op == TokenKind::Gt) return makeBool(a.asFloat() > b.asFloat());
 if(op == TokenKind::Ge) return makeBool(a.asFloat() >= b.asFloat());
 if(isVector(a.type) || isVector(b.type)) {
  const CustomUniformType type = isVector(a.type) ? a.type : b.type;
  float out[4]{};
  for(int i = 0; i < vecLen(type); ++i) {
   const float x = componentOf(a, i);
   const float y = componentOf(b, i);
   switch(op) {
   case TokenKind::Plus: out[i] = x + y; break;
   case TokenKind::Minus: out[i] = x - y; break;
   case TokenKind::Star: out[i] = x * y; break;
   case TokenKind::Slash: out[i] = y != 0.0f ? x / y : 0.0f; break;
   case TokenKind::Percent: out[i] = y != 0.0f ? std::fmod(x, y) : 0.0f; break;
   default: break;
   }
  }
  return makeVec(type, out);
 }
 if(a.type == CustomUniformType::Int && b.type == CustomUniformType::Int && op != TokenKind::Slash) {
  const int x = a.asInt();
  const int y = b.asInt();
  switch(op) {
  case TokenKind::Plus: return makeInt(std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(x) + static_cast<std::uint32_t>(y)));
  case TokenKind::Minus: return makeInt(std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(x) - static_cast<std::uint32_t>(y)));
  case TokenKind::Star: return makeInt(std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(x) * static_cast<std::uint32_t>(y)));
  case TokenKind::Percent: return makeInt(y != 0 && y != -1 ? x % y : 0);
  default: break;
  }
 }
 const float x = a.asFloat();
 const float y = b.asFloat();
 switch(op) {
 case TokenKind::Plus: return makeFloat(x + y);
 case TokenKind::Minus: return makeFloat(x - y);
 case TokenKind::Star: return makeFloat(x * y);
 case TokenKind::Slash: return makeFloat(y != 0.0f ? x / y : 0.0f);
 case TokenKind::Percent: return makeFloat(y != 0.0f ? std::fmod(x, y) : 0.0f);
 default: return makeFloat(0.0f);
 }
}
CustomUniformValue callFunction(const std::string& name, const std::vector<CustomUniformValue>& args, EvalContext& ctx) {
 auto need = [&](std::size_t n) { return args.size() >= n; };
 auto f0 = [&](std::size_t i) { return i < args.size() ? args[i].asFloat() : 0.0f; };
 auto i0 = [&](std::size_t i) { return i < args.size() ? args[i].asInt() : 0; };
 if(name == "torad" || name == "radians") return makeFloat(f0(0) * (kPi / 180.0f));
 if(name == "todeg" || name == "degrees") return makeFloat(f0(0) * (180.0f / kPi));
 if(name == "sin") return makeFloat(std::sin(f0(0)));
 if(name == "cos") return makeFloat(std::cos(f0(0)));
 if(name == "tan") return makeFloat(std::tan(f0(0)));
 if(name == "asin") return makeFloat(std::asin(std::clamp(f0(0), -1.0f, 1.0f)));
 if(name == "acos") return makeFloat(std::acos(std::clamp(f0(0), -1.0f, 1.0f)));
 if(name == "atan" || name == "atan2") {
  if(args.size() >= 2) return makeFloat(std::atan2(f0(0), f0(1)));
  return makeFloat(std::atan(f0(0)));
 }
 if(name == "exp") return makeFloat(std::exp(f0(0)));
 if(name == "pow") return makeFloat(std::pow(f0(0), f0(1)));
 if(name == "exp2") return makeFloat(std::exp2(f0(0)));
 if(name == "exp10") return makeFloat(std::pow(10.0f, f0(0)));
 if(name == "log") {
  if(args.size() >= 2) {
   const float base = f0(0);
   const float value = f0(1);
   if(base <= 0.0f || value <= 0.0f) return makeFloat(0.0f);
   return makeFloat(std::log(value) / std::log(base));
  }
  return makeFloat(f0(0) > 0.0f ? std::log(f0(0)) : 0.0f);
 }
 if(name == "log2") return makeFloat(f0(0) > 0.0f ? std::log2(f0(0)) : 0.0f);
 if(name == "log10") return makeFloat(f0(0) > 0.0f ? std::log10(f0(0)) : 0.0f);
 if(name == "sqrt") return makeFloat(std::sqrt(std::max(0.0f, f0(0))));
 auto mapUnary = [&](auto fn) -> CustomUniformValue {
  if(!need(1)) return makeFloat(0.0f);
  if(isVector(args[0].type)) {
   float out[4]{};
   for(int i = 0; i < vecLen(args[0].type); ++i) out[i] = fn(componentOf(args[0], i));
   return makeVec(args[0].type, out);
  }
  if(args[0].type == CustomUniformType::Int) return makeInt(static_cast<int>(fn(static_cast<float>(args[0].asInt()))));
  return makeFloat(fn(f0(0)));
 };
 if(name == "abs") {
  if(need(1) && args[0].type == CustomUniformType::Int) {
   const int value = args[0].i;
   return makeInt(value < 0 ? std::bit_cast<std::int32_t>(0u - static_cast<std::uint32_t>(value)) : value);
  }
  return mapUnary([](float x) { return std::fabs(x); });
 }
 if(name == "sign" || name == "signum")
  return mapUnary([](float x) { return x < 0.0f ? -1.0f : x > 0.0f ? 1.0f
                                                                   : 0.0f; });
 if(name == "floor") return mapUnary([](float x) { return std::floor(x); });
 if(name == "ceil") return mapUnary([](float x) { return std::ceil(x); });
 if(name == "frac") return mapUnary([](float x) { return x - std::floor(x); });
 if((name == "min" || name == "max") && need(2) && args.size() <= 16) {
  const bool minimum = name == "min";
  const auto combine = [minimum](float left, float right) {
   return minimum ? std::min(left, right) : std::max(left, right);
  };
  const auto vector = std::find_if(args.begin(), args.end(), [](const CustomUniformValue& value) {
   return isVector(value.type);
  });
  if(vector != args.end()) {
   const CustomUniformType type = vector->type;
   float out[4]{};
   for(int component = 0; component < vecLen(type); ++component) {
    out[component] = componentOf(args[0], component);
    for(std::size_t argument = 1; argument < args.size(); ++argument) {
     out[component] = combine(out[component], componentOf(args[argument], component));
    }
   }
   return makeVec(type, out);
  }
  const bool integers = std::all_of(args.begin(), args.end(), [](const CustomUniformValue& value) {
   return value.type == CustomUniformType::Int;
  });
  if(integers) {
   int result = args.front().asInt();
   for(std::size_t argument = 1; argument < args.size(); ++argument) {
    result = minimum ? std::min(result, args[argument].asInt())
                     : std::max(result, args[argument].asInt());
   }
   return makeInt(result);
  }
  float result = f0(0);
  for(std::size_t argument = 1; argument < args.size(); ++argument) result = combine(result, f0(argument));
  return makeFloat(result);
 }
 if(name == "clamp" && need(3)) {
  if(args[0].type == CustomUniformType::Int && args[1].type == CustomUniformType::Int &&
     args[2].type == CustomUniformType::Int) return makeInt(std::max(i0(1), std::min(i0(2), i0(0))));
  if(isVector(args[0].type)) {
   float out[4]{};
   for(int i = 0; i < vecLen(args[0].type); ++i) {
    out[i] = std::max(componentOf(args[1], i), std::min(componentOf(args[2], i), componentOf(args[0], i)));
   }
   return makeVec(args[0].type, out);
  }
  return makeFloat(std::clamp(f0(0), f0(1), f0(2)));
 }
 if(name == "mix" && need(3)) return makeFloat(f0(0) * (1.0f - f0(2)) + f0(1) * f0(2));
 if(name == "edge") {
  if(args[0].type == CustomUniformType::Int && args[1].type == CustomUniformType::Int) {
   return makeInt(i0(1) < i0(0) ? 0 : 1);
  }
  return makeFloat(f0(1) < f0(0) ? 0.0f : 1.0f);
 }
 if(name == "fmod" && need(2)) {
  if(args[0].type == CustomUniformType::Int && args[1].type == CustomUniformType::Int) {
   const int x = i0(0), y = i0(1);
   const int remainder = y != 0 && y != -1 ? x % y : 0;
   return makeInt(remainder != 0 && ((remainder < 0) != (y < 0)) ? remainder + y : remainder);
  }
  const float y = f0(1);
  if(y == 0.0f) return makeFloat(0.0f);
  return makeFloat(f0(0) - y * std::floor(f0(0) / y));
 }
 if(name == "random") {
  std::uniform_real_distribution<float> dist(0.0f, 1.0f);
  float u = ctx.rng != nullptr ? dist(*ctx.rng) : 0.0f;
  if(args.size() >= 2) u = f0(0) + u * (f0(1) - f0(0));
  return makeFloat(u);
 }
 if(name == "randomInt") {
  if(args.size() == 1) {
   const int bound = i0(0);
   if(bound <= 0) return makeInt(0);
   std::uniform_int_distribution<int> dist(0, bound - 1);
   return makeInt(ctx.rng != nullptr ? dist(*ctx.rng) : 0);
  }
  if(args.size() >= 2) {
   const int lo = i0(0);
   const int hi = i0(1);
   if(hi <= lo) return makeInt(lo);
   std::uniform_int_distribution<int> dist(lo, hi - 1);
   return makeInt(ctx.rng != nullptr ? dist(*ctx.rng) : lo);
  }
  std::uniform_int_distribution<int> dist(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
  return makeInt(ctx.rng != nullptr ? dist(*ctx.rng) : 0);
 }
 if(name == "between" && need(3)) {
  if(args[0].type == CustomUniformType::Int && args[1].type == CustomUniformType::Int &&
     args[2].type == CustomUniformType::Int) return makeBool(i0(0) >= i0(1) && i0(0) <= i0(2));
  return makeBool(f0(0) >= f0(1) && f0(0) <= f0(2));
 }
 if(name == "equals" && args.size() == 2) return applyBinary(TokenKind::EqEq, args[0], args[1]);
 if(name == "equals" && need(3)) return makeBool(std::fabs(f0(0) - f0(1)) <= f0(2));
 if(name == "in" && need(1)) {
  const float x = f0(0);
  for(std::size_t i = 1; i < args.size(); ++i) {
   if(x == args[i].asFloat()) return makeBool(true);
  }
  return makeBool(false);
 }
 return makeFloat(0.0f);
}
CustomUniformValue eval(ExprNode& node, EvalContext& ctx) {
 const auto evaluate = [&]() -> CustomUniformValue {
 switch(node.kind) {
 case NodeKind::Lit:
  if(node.type == CustomUniformType::Bool) return makeBool(node.number != 0.0f);
  if(node.type == CustomUniformType::Int) return makeInt(static_cast<int>(node.number));
  return makeFloat(node.number);
 case NodeKind::Ident: {
  if(node.customValue != nullptr) return *node.customValue;
  CustomUniformValue out;
  if(ctx.frame != nullptr && lookupBuiltin(node.name, *ctx.frame, out)) return out;
  if(ctx.options != nullptr) {
   if(const auto it = ctx.options->find(node.name); it != ctx.options->end()) {
    char* end = nullptr;
    const double parsed = std::strtod(it->second.c_str(), &end);
    if(end != it->second.c_str() && end != nullptr && *end == '\0') {
     if(node.type == CustomUniformType::Int) return makeInt(static_cast<int>(parsed));
     return makeFloat(static_cast<float>(parsed));
    }
    if(it->second == "true" || it->second == "1") return makeBool(true);
    if(it->second == "false" || it->second == "0") return makeBool(false);
   }
  }
  return makeFloat(0.0f);
 }
 case NodeKind::Unary: {
  const CustomUniformValue v = eval(*node.kids[0], ctx);
  if(node.op == TokenKind::Bang) return makeBool(!v.asBool());
  if(node.op == TokenKind::Plus) return v;
  if(isVector(v.type)) {
   float out[4]{};
   for(int i = 0; i < vecLen(v.type); ++i) out[i] = -componentOf(v, i);
   return makeVec(v.type, out);
  }
  if(v.type == CustomUniformType::Int) return makeInt(std::bit_cast<std::int32_t>(0u - static_cast<std::uint32_t>(v.i)));
  return makeFloat(-v.asFloat());
 }
 case NodeKind::Binary: {
  const CustomUniformValue left = eval(*node.kids[0], ctx);
  const CustomUniformValue right = eval(*node.kids[1], ctx);
  return applyBinary(node.op, left, right);
 }
 case NodeKind::Construct: {
  float out[4]{};
  for(std::size_t i = 0; i < node.kids.size() && i < 4; ++i) out[i] = eval(*node.kids[i], ctx).asFloat();
  return makeVec(node.type, out);
 }
 case NodeKind::Call: {
  if(node.name == "if") {
   CustomUniformValue result;
   for(std::size_t i = 0; i + 1 < node.kids.size(); i += 2) {
    if(eval(*node.kids[i], ctx).asBool()) return eval(*node.kids[i + 1], ctx);
    result = eval(*node.kids.back(), ctx);
   }
   return result;
  }
  if(node.name == "in") {
   const float value = eval(*node.kids[0], ctx).asFloat();
   for(std::size_t i = 1; i < node.kids.size(); ++i) {
    if(value == eval(*node.kids[i], ctx).asFloat()) return makeBool(true);
   }
   return makeBool(false);
  }
  for(std::size_t i = 0; i < node.kids.size(); ++i) node.args[i] = eval(*node.kids[i], ctx);
  if(node.name == "smooth") {
   const std::size_t first = node.smoothHasId ? 1 : 0;
   const auto& target = node.args[first];
   const float up = node.args.size() > first + 1 ? node.args[first + 1].asFloat() : 1.0f;
   const float down = node.args.size() > first + 2 ? node.args[first + 2].asFloat() : up;
   for(int i = 0; i < vecLen(target.type); ++i) {
    const float value = componentOf(target, i);
    float& previous = node.smoothValue[i];
    const float fade = value > previous ? up : down;
    if(!node.smoothInitialized || fade == 0.0f) {
     previous = value;
    } else {
     const float halfLife = fade * 0.1f;
     const float decay = kLn2 / halfLife;
     const float alpha = 1.0f - std::exp(-decay * ctx.frame->frameTime);
     previous = (1.0f - alpha) * previous + alpha * value;
    }
   }
   node.smoothInitialized = true;
   return isVector(target.type) ? makeVec(target.type, node.smoothValue) : makeFloat(node.smoothValue[0]);
  }
  return callFunction(node.name, node.args, ctx);
 }
 case NodeKind::Swizzle: return makeFloat(componentOf(eval(*node.kids[0], ctx), node.component));
 case NodeKind::MatrixElem:
  if(node.kids[0]->kind == NodeKind::Ident && ctx.frame != nullptr) {
   const float* m = nullptr;
   if(matrixNamed(node.kids[0]->name, *ctx.frame, m)) {
    const int row = std::clamp(node.component, 0, 3);
    const int col = std::clamp(node.matrixCol, 0, 3);
    return makeFloat(m[col * 4 + row]);
   }
  }
  return makeFloat(0.0f);
 }
 return makeFloat(0.0f);
 };
 CustomUniformValue value = evaluate();
 if(node.operationType == CustomUniformType::Int) value = makeInt(value.asInt());
 if(node.resultType == CustomUniformType::Float) return makeFloat(value.asFloat());
 if(node.resultType == CustomUniformType::Int) return makeInt(value.asInt());
 if(node.resultType == CustomUniformType::Bool) return makeBool(value.asBool());
 return value;
}
struct Parser {
 explicit Parser(std::string_view source) : lex_(source), cur_(lex_.next()) {}
 std::unique_ptr<ExprNode> parse(std::string& error) {
  try {
   auto root = parseOr();
   if(cur_.kind != TokenKind::End) {
    error = "unexpected token near '" + cur_.text + "'";
    return nullptr;
   }
   return root;
  } catch(const std::string& msg) {
   error = msg;
   return nullptr;
  }
 }
 Lexer lex_;
 Token cur_;
 void advance() { cur_ = lex_.next(); }
 bool accept(TokenKind kind) {
  if(cur_.kind != kind) return false;
  advance();
  return true;
 }
 void expect(TokenKind kind, const char* what) {
  if(!accept(kind)) throw std::string("expected ") + what;
 }
 std::unique_ptr<ExprNode> parseOr() {
  auto left = parseAnd();
  while(cur_.kind == TokenKind::PipePipe) {
   advance();
   auto node = std::make_unique<ExprNode>();
   node->kind = NodeKind::Binary;
   node->op = TokenKind::PipePipe;
   node->type = CustomUniformType::Bool;
   node->kids.push_back(std::move(left));
   node->kids.push_back(parseAnd());
   left = std::move(node);
  }
  return left;
 }
 std::unique_ptr<ExprNode> parseAnd() {
  auto left = parseEq();
  while(cur_.kind == TokenKind::AmpAmp) {
   advance();
   auto node = std::make_unique<ExprNode>();
   node->kind = NodeKind::Binary;
   node->op = TokenKind::AmpAmp;
   node->type = CustomUniformType::Bool;
   node->kids.push_back(std::move(left));
   node->kids.push_back(parseEq());
   left = std::move(node);
  }
  return left;
 }
 std::unique_ptr<ExprNode> parseEq() {
  auto left = parseRel();
  while(cur_.kind == TokenKind::EqEq || cur_.kind == TokenKind::BangEq) {
   const TokenKind op = cur_.kind;
   advance();
   auto node = std::make_unique<ExprNode>();
   node->kind = NodeKind::Binary;
   node->op = op;
   node->type = CustomUniformType::Bool;
   node->kids.push_back(std::move(left));
   node->kids.push_back(parseRel());
   left = std::move(node);
  }
  return left;
 }
 std::unique_ptr<ExprNode> parseRel() {
  auto left = parseAdd();
  while(cur_.kind == TokenKind::Lt || cur_.kind == TokenKind::Le || cur_.kind == TokenKind::Gt ||
        cur_.kind == TokenKind::Ge) {
   const TokenKind op = cur_.kind;
   advance();
   auto node = std::make_unique<ExprNode>();
   node->kind = NodeKind::Binary;
   node->op = op;
   node->type = CustomUniformType::Bool;
   node->kids.push_back(std::move(left));
   node->kids.push_back(parseAdd());
   left = std::move(node);
  }
  return left;
 }
 std::unique_ptr<ExprNode> parseAdd() {
  auto left = parseMul();
  while(cur_.kind == TokenKind::Plus || cur_.kind == TokenKind::Minus) {
   const TokenKind op = cur_.kind;
   advance();
   auto right = parseMul();
   auto node = std::make_unique<ExprNode>();
   node->kind = NodeKind::Binary;
   node->op = op;
   node->type = left->type;
   if(isVector(right->type)) node->type = right->type;
   if(left->type == CustomUniformType::Int && right->type == CustomUniformType::Int)
    node->type = CustomUniformType::Int;
   else if(!isVector(node->type) && node->type != CustomUniformType::Bool)
    node->type = CustomUniformType::Float;
   node->kids.push_back(std::move(left));
   node->kids.push_back(std::move(right));
   left = std::move(node);
  }
  return left;
 }
 std::unique_ptr<ExprNode> parseMul() {
  auto left = parseUnary();
  while(cur_.kind == TokenKind::Star || cur_.kind == TokenKind::Slash || cur_.kind == TokenKind::Percent) {
   const TokenKind op = cur_.kind;
   advance();
   auto right = parseUnary();
   auto node = std::make_unique<ExprNode>();
   node->kind = NodeKind::Binary;
   node->op = op;
   node->type = left->type;
   if(isVector(right->type)) node->type = right->type;
   if(left->type == CustomUniformType::Int && right->type == CustomUniformType::Int && op != TokenKind::Slash)
    node->type = CustomUniformType::Int;
   else if(!isVector(node->type))
    node->type = CustomUniformType::Float;
   node->kids.push_back(std::move(left));
   node->kids.push_back(std::move(right));
   left = std::move(node);
  }
  return left;
 }
 std::unique_ptr<ExprNode> parseUnary() {
  if(cur_.kind == TokenKind::Bang || cur_.kind == TokenKind::Plus || cur_.kind == TokenKind::Minus) {
   const TokenKind op = cur_.kind;
   advance();
   auto node = std::make_unique<ExprNode>();
   node->kind = NodeKind::Unary;
   node->op = op;
   node->kids.push_back(parseUnary());
   node->type = op == TokenKind::Bang ? CustomUniformType::Bool : node->kids[0]->type;
   return node;
  }
  return parsePostfix();
 }
 std::unique_ptr<ExprNode> parsePostfix() {
  auto left = parsePrimary();
  while(cur_.kind == TokenKind::Dot) {
   advance();
   if(cur_.kind == TokenKind::Number) {
    const int column = static_cast<int>(cur_.number);
    advance();
    if(cur_.kind == TokenKind::Dot) {
     advance();
     if(cur_.kind == TokenKind::Number) {
      const int row = static_cast<int>(cur_.number);
      advance();
      auto node = std::make_unique<ExprNode>();
      node->kind = NodeKind::MatrixElem;
      node->type = CustomUniformType::Float;
      node->component = row;
      node->matrixCol = column;
      node->kids.push_back(std::move(left));
      left = std::move(node);
     } else if(cur_.kind == TokenKind::Ident) {
      const std::string comp = cur_.text;
      advance();
      int index = -1;
      if(comp == "x" || comp == "r" || comp == "s")
       index = 0;
      else if(comp == "y" || comp == "g" || comp == "t")
       index = 1;
      else if(comp == "z" || comp == "b" || comp == "p")
       index = 2;
      else if(comp == "w" || comp == "a" || comp == "q")
       index = 3;
      else
       throw std::string("unknown component '." + comp + "'");
      auto node = std::make_unique<ExprNode>();
      node->kind = NodeKind::MatrixElem;
      node->type = CustomUniformType::Float;
      node->component = index;
      node->matrixCol = column;
      node->kids.push_back(std::move(left));
      left = std::move(node);
     } else {
      throw std::string("expected matrix column index");
     }
    } else {
     auto node = std::make_unique<ExprNode>();
     node->kind = NodeKind::Swizzle;
     node->type = CustomUniformType::Float;
     node->component = column;
     node->kids.push_back(std::move(left));
     left = std::move(node);
    }
    continue;
   }
   if(cur_.kind != TokenKind::Ident) throw std::string("expected component after '.'");
   const std::string comp = cur_.text;
   advance();
   int index = -1;
   if(comp == "x" || comp == "r" || comp == "s")
    index = 0;
   else if(comp == "y" || comp == "g" || comp == "t")
    index = 1;
   else if(comp == "z" || comp == "b" || comp == "p")
    index = 2;
   else if(comp == "w" || comp == "a" || comp == "q")
    index = 3;
   else
    throw std::string("unknown component '." + comp + "'");
   auto node = std::make_unique<ExprNode>();
   node->kind = NodeKind::Swizzle;
   node->type = CustomUniformType::Float;
   node->component = index;
   node->kids.push_back(std::move(left));
   left = std::move(node);
  }
  return left;
 }
 std::unique_ptr<ExprNode> parsePrimary() {
  if(cur_.kind == TokenKind::Number) {
   auto node = std::make_unique<ExprNode>();
   node->kind = NodeKind::Lit;
   node->number = cur_.number;
   node->numericLiteral = true;
   const bool integer = cur_.text.find('.') == std::string::npos &&
                        cur_.number <= std::numeric_limits<int>::max();
   node->type = integer ? CustomUniformType::Int : CustomUniformType::Float;
   advance();
   return node;
  }
  if(cur_.kind == TokenKind::Ident) {
   const std::string name = cur_.text;
   advance();
   if(accept(TokenKind::LParen)) {
    auto node = std::make_unique<ExprNode>();
    if(name == "vec2" || name == "vec3" || name == "vec4") {
     node->kind = NodeKind::Construct;
     node->name = name;
     node->type = name == "vec2" ? CustomUniformType::Vec2 : name == "vec3" ? CustomUniformType::Vec3
                                                                            : CustomUniformType::Vec4;
    } else {
     node->kind = NodeKind::Call;
     node->name = name;
     node->type = CustomUniformType::Float;
    }
    if(cur_.kind != TokenKind::RParen) {
     node->kids.push_back(parseOr());
     while(accept(TokenKind::Comma)) node->kids.push_back(parseOr());
    }
    expect(TokenKind::RParen, ")");
    if(node->kind == NodeKind::Call) {
     if(node->name == "if") {
      node->type = node->kids.size() >= 2 ? node->kids[1]->type : CustomUniformType::Float;
     } else if(node->name == "in" || node->name == "between" || node->name == "equals") {
      node->type = CustomUniformType::Bool;
     } else if(node->name == "randomInt") {
      node->type = CustomUniformType::Int;
     } else if(!node->kids.empty() && isVector(node->kids[0]->type) &&
               (node->name == "abs" || node->name == "floor" || node->name == "ceil" || node->name == "min" ||
                node->name == "max" || node->name == "clamp" || node->name == "smooth")) {
      node->type = node->kids[0]->type;
     }
    }
    return node;
   }
   auto node = std::make_unique<ExprNode>();
   node->kind = NodeKind::Ident;
   node->name = name;
   if(name == "true" || name == "false") {
    node->kind = NodeKind::Lit;
    node->type = CustomUniformType::Bool;
    node->number = name == "true" ? 1.0f : 0.0f;
   } else if(name == "pi") {
    node->kind = NodeKind::Lit;
    node->type = CustomUniformType::Float;
    node->number = kPi;
   }
   return node;
  }
  if(accept(TokenKind::LParen)) {
   auto node = parseOr();
   expect(TokenKind::RParen, ")");
   return node;
  }
  throw std::string("unexpected token '" + cur_.text + "'");
 }
};
bool resolveExpression(ExprNode& node, CustomUniformType target, bool direct = true, bool implicit = true) {
 using Type = CustomUniformType;
 const auto match = [&](bool casts) {
  const auto child = [&](std::size_t index, Type type) {
   return resolveExpression(*node.kids[index], type, !casts || node.kids.size() > 1, casts);
  };
  const auto arguments = [&](Type type) {
   for(std::size_t i = 0; i < node.kids.size(); ++i) {
    if(!child(i, type)) return false;
   }
   return true;
  };
  const auto condition = [&](std::size_t index) {
   return child(index, Type::Bool) || child(index, Type::Int);
  };
  switch(node.kind) {
  case NodeKind::Lit:
  case NodeKind::Ident:
   return node.type == target || (casts && node.type == Type::Int && target == Type::Float);
  case NodeKind::Construct:
   return target == node.type && node.kids.size() == static_cast<std::size_t>(vecLen(target)) && arguments(Type::Float);
  case NodeKind::Swizzle:
   return target == Type::Float && child(0, node.kids[0]->type);
  case NodeKind::MatrixElem:
   return target == Type::Float;
  case NodeKind::Unary:
   if(node.op == TokenKind::Bang) return target == Type::Bool && condition(0);
   return target != Type::Bool && arguments(target);
  case NodeKind::Binary:
   if(node.op == TokenKind::AmpAmp || node.op == TokenKind::PipePipe) {
    return target == Type::Bool && condition(0) && condition(1);
   }
   if(node.op == TokenKind::EqEq || node.op == TokenKind::BangEq || node.op == TokenKind::Lt ||
      node.op == TokenKind::Le || node.op == TokenKind::Gt || node.op == TokenKind::Ge) {
    if(target != Type::Bool) return false;
    if(arguments(Type::Int) || arguments(Type::Float)) return true;
    if(node.op == TokenKind::EqEq || node.op == TokenKind::BangEq) {
     return arguments(Type::Bool) || arguments(Type::Vec2) || arguments(Type::Vec3) || arguments(Type::Vec4);
    }
    return false;
   }
   return target != Type::Bool && !(target == Type::Int && node.op == TokenKind::Slash) && arguments(target);
  case NodeKind::Call:
   if(node.name == "if") {
    if(node.kids.size() < 3 || node.kids.size() > 33 || node.kids.size() % 2 == 0) return false;
    if(node.kids.size() > 3 && isVector(target)) return false;
    for(std::size_t i = 0; i + 1 < node.kids.size(); i += 2) {
     if(!condition(i) || !child(i + 1, target)) return false;
    }
    return child(node.kids.size() - 1, target);
   }
   if(node.name == "smooth") {
    if(target != Type::Float || node.kids.empty() || node.kids.size() > 4) return false;
    node.smoothHasId = node.kids.size() > 1 && node.kids[0]->numericLiteral;
    return (node.kids.size() < 4 || node.smoothHasId) && arguments(Type::Float);
   }
   if(node.name == "in") {
    return target == Type::Bool && node.kids.size() >= 2 && node.kids.size() <= 32 && arguments(Type::Float);
   }
   if(node.name == "between") {
    return target == Type::Bool && node.kids.size() == 3 && (arguments(Type::Int) || arguments(Type::Float));
   }
   if(node.name == "equals") {
    if(target != Type::Bool) return false;
    if(node.kids.size() == 2) return arguments(Type::Int) || arguments(Type::Float) || arguments(Type::Bool);
    return node.kids.size() == 3 && arguments(Type::Float);
   }
   if(node.name == "floor" || node.name == "ceil") {
    return node.kids.size() == 1 && target != Type::Bool && arguments(isVector(target) ? target : Type::Float);
   }
   if(node.name == "abs" || node.name == "min" || node.name == "max" || node.name == "clamp" ||
      node.name == "edge" || node.name == "fmod") {
    if(target == Type::Bool) return false;
    const std::size_t count = node.kids.size();
    if(node.name == "abs" && count != 1) return false;
    if((node.name == "min" || node.name == "max") && (count < 2 || count > 16)) return false;
    if(node.name == "clamp" && count != 3) return false;
    if((node.name == "edge" || node.name == "fmod") && (count != 2 || isVector(target))) return false;
    return arguments(target);
   }
   if(node.name == "randomInt") return target == Type::Int && node.kids.size() <= 2 && arguments(Type::Int);
   if(target != Type::Float) return false;
   static constexpr std::string_view unary[] = {
       "torad", "radians", "todeg", "degrees", "sin", "cos", "tan", "asin", "acos", "exp",
       "exp2", "exp10", "log2", "log10", "sqrt", "sign", "signum", "frac"};
   if(std::find(std::begin(unary), std::end(unary), node.name) != std::end(unary)) {
    return node.kids.size() == 1 && arguments(Type::Float);
   }
   if(node.name == "atan" || node.name == "atan2" || node.name == "log") {
    return (node.kids.size() == 1 || node.kids.size() == 2) && arguments(Type::Float);
   }
   if(node.name == "pow") return node.kids.size() == 2 && arguments(Type::Float);
   if(node.name == "mix") return node.kids.size() == 3 && arguments(Type::Float);
   if(node.name == "random") return (node.kids.empty() || node.kids.size() == 2) && arguments(Type::Float);
   return false;
  }
  return false;
 };
 if(direct && match(false)) {
  node.operationType = node.resultType = target;
  return true;
 }
 if(!implicit) return false;
 if(target == Type::Float && node.kind != NodeKind::Lit && node.kind != NodeKind::Ident &&
    resolveExpression(node, Type::Int)) {
  node.resultType = target;
  return true;
 }
 if(match(true)) {
  node.operationType = node.resultType = target;
  return true;
 }
 return false;
}
} // namespace
float CustomUniformValue::asFloat() const noexcept {
 if(type == CustomUniformType::Int) return static_cast<float>(i);
 if(type == CustomUniformType::Bool) return b ? 1.0f : 0.0f;
 return f[0];
}
int CustomUniformValue::asInt() const noexcept {
 if(type == CustomUniformType::Bool) return b ? 1 : 0;
 if(type == CustomUniformType::Int) return i;
 if(std::isnan(f[0])) return 0;
 if(f[0] >= static_cast<float>(std::numeric_limits<int>::max())) return std::numeric_limits<int>::max();
 if(f[0] <= static_cast<float>(std::numeric_limits<int>::min())) return std::numeric_limits<int>::min();
 return static_cast<int>(f[0]);
}
bool CustomUniformValue::asBool() const noexcept {
 if(type == CustomUniformType::Bool) return b;
 if(type == CustomUniformType::Int) return i != 0;
 return f[0] != 0.0f;
}
bool parseCustomUniformType(std::string_view text, CustomUniformType& out) noexcept {
 if(text == "float") {
  out = CustomUniformType::Float;
  return true;
 }
 if(text == "int") {
  out = CustomUniformType::Int;
  return true;
 }
 if(text == "bool") {
  out = CustomUniformType::Bool;
  return true;
 }
 if(text == "vec2") {
  out = CustomUniformType::Vec2;
  return true;
 }
 if(text == "vec3") {
  out = CustomUniformType::Vec3;
  return true;
 }
 if(text == "vec4") {
  out = CustomUniformType::Vec4;
  return true;
 }
 return false;
}
const char* customUniformTypeName(CustomUniformType type) noexcept {
 switch(type) {
 case CustomUniformType::Float: return "float";
 case CustomUniformType::Int: return "int";
 case CustomUniformType::Bool: return "bool";
 case CustomUniformType::Vec2: return "vec2";
 case CustomUniformType::Vec3: return "vec3";
 case CustomUniformType::Vec4: return "vec4";
 }
 return "float";
}
CustomUniformRuntime::CustomUniformRuntime() = default;
CustomUniformRuntime::~CustomUniformRuntime() = default;
struct CustomUniformRuntime::Compiled {
 CustomUniformDecl decl;
 std::unique_ptr<ExprNode> root;
 CustomUniformValue* value = nullptr;
};
CustomUniformRuntime::CustomUniformRuntime(CustomUniformRuntime&& other) noexcept
    : compiled_(std::move(other.compiled_)), values_(std::move(other.values_)), options_(std::move(other.options_)) {
 bindValues();
}
CustomUniformRuntime& CustomUniformRuntime::operator=(CustomUniformRuntime&& other) noexcept {
 if(this == &other) return *this;
 compiled_ = std::move(other.compiled_);
 values_ = std::move(other.values_);
 options_ = std::move(other.options_);
 bindValues();
 return *this;
}
void CustomUniformRuntime::bindValues() {
 const auto bind = [&](auto&& self, ExprNode& node) -> void {
  if(node.kind == NodeKind::Ident && node.customValue != nullptr) node.customValue = &values_.at(node.name);
  for(auto& kid : node.kids) self(self, *kid);
 };
 for(const auto& compiled : compiled_) {
  compiled->value = &values_.at(compiled->decl.name);
  bind(bind, *compiled->root);
 }
}
void CustomUniformRuntime::clear() {
 compiled_.clear();
 values_.clear();
}
void CustomUniformRuntime::setOptions(std::unordered_map<std::string, std::string> options) {
 options_ = std::move(options);
}
bool CustomUniformRuntime::compile(const std::vector<CustomUniformDecl>& decls, std::string& error) {
 compiled_.clear();
 values_.clear();
 error.clear();
 for(const CustomUniformDecl& decl : decls) {
  if(decl.expression.empty()) {
   if(!error.empty()) error += "; ";
   error += "custom " + std::string(decl.upload ? "uniform" : "variable") + "." + customUniformTypeName(decl.type) +
            "." + decl.name + ": empty expression";
   continue;
  }
  std::string localError;
  try {
   Parser parser(decl.expression);
   auto root = parser.parse(localError);
   if(root == nullptr) {
    if(!error.empty()) error += "; ";
    error += "custom " + std::string(decl.upload ? "uniform" : "variable") + "." + customUniformTypeName(decl.type) +
             "." + decl.name + ": " + localError;
    continue;
   }
   auto compiled = std::make_unique<Compiled>();
   compiled->decl = decl;
   compiled->root = std::move(root);
   compiled_.push_back(std::move(compiled));
  } catch(const std::string& msg) {
   if(!error.empty()) error += "; ";
   error += "custom " + std::string(decl.upload ? "uniform" : "variable") + "." + customUniformTypeName(decl.type) +
            "." + decl.name + ": " + msg;
  }
 }
 if(!error.empty()) {
  clear();
  return false;
 }
 std::unordered_map<std::string, std::size_t> indices;
 std::vector<std::vector<std::size_t>> dependencies(compiled_.size());
 try {
  for(std::size_t i = 0; i < compiled_.size(); ++i) {
   auto& compiled = *compiled_[i];
   if(!indices.emplace(compiled.decl.name, i).second) throw std::string("duplicate custom variable: ") + compiled.decl.name;
   auto& value = values_[compiled.decl.name];
   value.type = compiled.decl.type;
   compiled.value = &value;
  }
  const PackUniformValues frame{};
  const auto bind = [&](auto&& self, ExprNode& node, std::size_t owner) -> void {
   if(node.kind == NodeKind::Call) node.args.resize(node.kids.size());
   if(node.kind == NodeKind::MatrixElem) {
    const float* matrix = nullptr;
    if(node.kids[0]->kind != NodeKind::Ident || !matrixNamed(node.kids[0]->name, frame, matrix)) {
     throw std::string("unknown matrix: ") + node.kids[0]->name;
    }
    return;
   }
   for(auto& kid : node.kids) self(self, *kid, owner);
   if(node.kind == NodeKind::Ident) {
    if(const auto found = indices.find(node.name); found != indices.end()) {
     dependencies[owner].push_back(found->second);
     node.customValue = compiled_[found->second]->value;
     node.type = node.customValue->type;
    } else {
     CustomUniformValue builtin;
     if(lookupBuiltin(node.name, frame, builtin)) {
      node.type = builtin.type;
     } else if(const auto option = options_.find(node.name); option != options_.end()) {
      if(option->second == "true" || option->second == "false") {
       node.type = CustomUniformType::Bool;
      } else {
       char* end = nullptr;
       const double number = std::strtod(option->second.c_str(), &end);
       if(end == option->second.c_str() || *end != '\0') throw std::string("non-numeric option: ") + node.name;
       node.type = option->second.find_first_of(".eE") == std::string::npos &&
                           number >= std::numeric_limits<int>::min() && number <= std::numeric_limits<int>::max()
                       ? CustomUniformType::Int : CustomUniformType::Float;
      }
     } else {
      throw std::string("unknown custom variable: ") + node.name;
     }
    }
   } else if(node.kind == NodeKind::Binary && node.type != CustomUniformType::Bool) {
    node.type = node.kids[0]->type;
    if(isVector(node.kids[1]->type)) node.type = node.kids[1]->type;
    else if(!isVector(node.type) && (node.kids[1]->type == CustomUniformType::Float || node.op == TokenKind::Slash)) {
     node.type = CustomUniformType::Float;
    }
   } else if(node.kind == NodeKind::Unary && node.op != TokenKind::Bang) {
    node.type = node.kids[0]->type;
   } else if(node.kind == NodeKind::Call && !node.kids.empty()) {
    if(node.name == "if" && node.kids.size() > 1) node.type = node.kids[1]->type;
    else if(node.name == "abs" || node.name == "min" || node.name == "max" || node.name == "clamp") {
     node.type = node.kids[0]->type;
    }
   }
  };
  for(std::size_t i = 0; i < compiled_.size(); ++i) {
   auto& compiled = *compiled_[i];
   bind(bind, *compiled.root, i);
   if(!resolveExpression(*compiled.root, compiled.decl.type)) {
    throw std::string("cannot resolve custom ") + compiled.decl.name + " as " + customUniformTypeName(compiled.decl.type);
   }
  }
  std::vector<unsigned char> visited(compiled_.size());
  std::vector<std::size_t> order;
  order.reserve(compiled_.size());
  const auto visit = [&](auto&& self, std::size_t index) -> void {
   if(visited[index] == 2) return;
   if(visited[index] == 1) throw std::string("circular custom variable reference: ") + compiled_[index]->decl.name;
   visited[index] = 1;
   for(const std::size_t dependency : dependencies[index]) self(self, dependency);
   visited[index] = 2;
   order.push_back(index);
  };
  for(std::size_t i = 0; i < compiled_.size(); ++i) visit(visit, i);
  std::vector<std::unique_ptr<Compiled>> sorted;
  sorted.reserve(compiled_.size());
  for(const std::size_t index : order) sorted.push_back(std::move(compiled_[index]));
  compiled_ = std::move(sorted);
 } catch(const std::string& message) {
  error = message;
  clear();
  return false;
 }
 return true;
}
void CustomUniformRuntime::evaluate(const PackUniformValues& frame) {
 static thread_local std::mt19937 rng{std::random_device{}()};
 EvalContext ctx;
 ctx.frame = &frame;
 ctx.options = &options_;
 ctx.rng = &rng;
 for(const auto& compiled : compiled_) {
  CustomUniformValue value = eval(*compiled->root, ctx);
  switch(compiled->decl.type) {
  case CustomUniformType::Float: value = makeFloat(value.asFloat()); break;
  case CustomUniformType::Int: value = makeInt(value.asInt()); break;
  case CustomUniformType::Bool: value = makeBool(value.asBool()); break;
  case CustomUniformType::Vec2:
  case CustomUniformType::Vec3:
  case CustomUniformType::Vec4:
   if(!isVector(value.type)) {
    float f[4] = {value.asFloat(), value.asFloat(), value.asFloat(), value.asFloat()};
    value = makeVec(compiled->decl.type, f);
   } else {
    value.type = compiled->decl.type;
   }
   break;
  }
  *compiled->value = value;
 }
}
void CustomUniformRuntime::upload(const gl::ShaderProgram& program) const {
 for(const auto& compiled : compiled_) {
  if(!compiled->decl.upload) continue;
  const CustomUniformValue& v = *compiled->value;
  const int loc = program.location(compiled->decl.name);
  if(loc < 0) continue;
  switch(compiled->decl.type) {
  case CustomUniformType::Float: program.set1fAt(loc, v.asFloat()); break;
  case CustomUniformType::Int:
  case CustomUniformType::Bool: program.set1iAt(loc, v.asInt()); break;
  case CustomUniformType::Vec2: program.set2fAt(loc, v.f); break;
  case CustomUniformType::Vec3: program.set3fAt(loc, v.f); break;
  case CustomUniformType::Vec4: program.set4fAt(loc, v.f); break;
  }
 }
}
} // namespace net::minecraft::client::render
