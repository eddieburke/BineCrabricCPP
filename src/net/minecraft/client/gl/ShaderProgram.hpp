#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
namespace net::minecraft::util::math {
struct Matrix4f;
}
namespace net::minecraft::client::gl {
struct ProgramBinaryBlob {
 std::uint64_t contentHash = 0;
 unsigned int binaryFormat = 0;
 std::uint32_t flags = 0;
 bool compute = false;
 bool tessellation = false;
 std::vector<unsigned char> bytes;
};
class ShaderProgram {
 public:
 static constexpr std::uint32_t kFlagCompute = 1u << 0;
 static constexpr std::uint32_t kFlagTessellation = 1u << 2;
 ShaderProgram();
 ~ShaderProgram();
 ShaderProgram(const ShaderProgram&) = delete;
 ShaderProgram& operator=(const ShaderProgram&) = delete;
 ShaderProgram(ShaderProgram&& other) noexcept;
 ShaderProgram& operator=(ShaderProgram&& other) noexcept;
 bool compile(const std::string& vertexSource,
              const std::string& fragmentSource,
              const std::string& versionPreamble,
              const std::string& geometrySource = {},
              const std::string& tessControlSource = {},
              const std::string& tessEvaluationSource = {});
 bool compileCompute(const std::string& computeSource,
                     const std::string& versionPreamble);
 bool loadFromBinary(const ProgramBinaryBlob& binary);
 // Compile-and-extract combo — one glLinkProgram, then reads the binary back for
 // ProgramCache to store on disk. See ProgramCache::compileSync.
 bool compileToBinary(ProgramBinaryBlob& out,
                      const std::string& vertexSource,
                      const std::string& fragmentSource,
                      const std::string& versionPreamble,
                      const std::string& geometrySource = {},
                      const std::string& tessControlSource = {},
                      const std::string& tessEvaluationSource = {});
 bool compileComputeToBinary(ProgramBinaryBlob& out,
                             const std::string& computeSource,
                             const std::string& versionPreamble);
 [[nodiscard]] static std::uint64_t contentHash(bool compute,
                                                const std::string& preamble,
                                                const std::string& a,
                                                const std::string& b = {},
                                                const std::string& c = {},
                                                const std::string& d = {},
                                                const std::string& e = {},
                                                const std::string& abiSalt = {});
 // Exactly what one stage hands the driver: preamble, then the body with its own
 // leading #version stripped. Compile failures dump through this too, so a
 // driver's "0(3872)" points at the same line in the dump.
 [[nodiscard]] static std::string assembleStageSource(const std::string& versionPreamble,
                                                      const std::string& body);
 void destroy();
 [[nodiscard]] bool valid() const {
  return program_ != 0;
 }
 [[nodiscard]] unsigned int handle() const {
  return program_;
 }
 const std::string& lastError() const {
  return lastError_;
 }
 void bind() const;
 static void unbind();
 [[nodiscard]] bool needsUniformSnapshot(unsigned int generation) const noexcept {
  return uniformSnapshotGeneration_ != generation;
 }
 void markUniformSnapshotPushed(unsigned int generation) const noexcept {
  uniformSnapshotGeneration_ = generation;
 }
 // Iris /* RENDERTARGETS: a,b */ → glDrawBuffers(COLOR_ATTACHMENT0+a, ...).
 // Empty = leave DrawBuffers unchanged (composite write FBOs remap attachments).
 void setDrawBufferColortexIndices(const std::vector<int>& colortexIndices);
 [[nodiscard]] const std::vector<int>& drawBufferColortexIndices() const {
  return drawBufferColortexIndices_;
 }
 void applyDrawBuffers(int colorAttachmentCount = 8) const;
 int location(std::string_view name) const;
 enum class SamplerKind {
  None,
  Float,
  Integer,
  Unsigned,
  Shadow,
  Volume
 };
 enum class IrisUniformSlot : std::uint8_t {
  FrameTimeCounter,
  FrameTime,
  FrameCounter,
  ViewWidth,
  ViewHeight,
  AspectRatio,
  Near,
  Far,
  ShadowMapResolution,
  CameraPosition,
  CameraPositionFract,
  CameraPositionInt,
  PreviousCameraPosition,
  PreviousCameraPositionFract,
  PreviousCameraPositionInt,
  SunPosition,
  MoonPosition,
  ShadowLightPosition,
  UpPosition,
  GbufferModelView,
  GbufferProjection,
  GbufferModelViewInverse,
  GbufferProjectionInverse,
  GbufferPreviousProjection,
  GbufferPreviousModelView,
  ShadowModelView,
  ShadowModelViewInverse,
  ShadowProjection,
  ShadowProjectionInverse,
  SunColor,
  SunIntensity,
  FogColor,
  FogDensity,
  FogStart,
  FogEnd,
  FogMode,
  FogShape,
  SkyColor,
  ThunderStrength,
  CurrentPlayerHealth,
  MaxPlayerHealth,
  Count
 };
 static constexpr std::string_view kIrisUniformSlotNames[static_cast<std::size_t>(IrisUniformSlot::Count)] = {
     "frameTimeCounter",
     "frameTime",
     "frameCounter",
     "viewWidth",
     "viewHeight",
     "aspectRatio",
     "near",
     "far",
     "shadowMapResolution",
     "cameraPosition",
     "cameraPositionFract",
     "cameraPositionInt",
     "previousCameraPosition",
     "previousCameraPositionFract",
     "previousCameraPositionInt",
     "sunPosition",
     "moonPosition",
     "shadowLightPosition",
     "upPosition",
     "gbufferModelView",
     "gbufferProjection",
     "gbufferModelViewInverse",
     "gbufferProjectionInverse",
     "gbufferPreviousProjection",
     "gbufferPreviousModelView",
     "shadowModelView",
     "shadowModelViewInverse",
     "shadowProjection",
     "shadowProjectionInverse",
     "sunColor",
     "sunIntensity",
     "fogColor",
     "fogDensity",
     "fogStart",
     "fogEnd",
     "fogMode",
     "fogShape",
     "skyColor",
     "thunderStrength",
     "currentPlayerHealth",
     "maxPlayerHealth"};
 [[nodiscard]] int uniformLocation(IrisUniformSlot slot) const noexcept {
  return uniformLocations_[static_cast<std::size_t>(slot)];
 }
 [[nodiscard]] SamplerKind samplerKind(std::string_view name) const;
 [[nodiscard]] const std::vector<std::string>& declaredSamplers() const;
 [[nodiscard]] bool tessellation() const {
  return tessellation_;
 }
 // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/gl/program/ComputeProgram.java
 [[nodiscard]] const int (&computeLocalSize() const)[3] {
  return computeLocalSize_;
 }
 void set1iAt(int location, int value) const;
 void set1fAt(int location, float value) const;
 // see third_party/mcp/iris/pipeline/programs/ShaderKey.java
 void setFogClass(bool enabled) const noexcept {
  fogClass_ = enabled;
 }
 [[nodiscard]] bool fogClass() const noexcept {
  return fogClass_;
 }
 void set2fAt(int location, const float* values) const;
 void set3fAt(int location, const float* values) const;
 void set4fAt(int location, const float* values) const;
 void set2iAt(int location, const int* values) const;
 void set3iAt(int location, const int* values) const;
 void set4iAt(int location, const int* values) const;
 void setMatrix3At(int location, const float* values) const;
 void setMatrix4At(int location, const float* values) const;
 void set1i(std::string_view name, int value) const;
 void set1f(std::string_view name, float value) const;
 void set2f(std::string_view name, float x, float y) const;
 void set3f(std::string_view name, float x, float y, float z) const;
 void set4f(std::string_view name, float x, float y, float z, float w) const;
 void setMatrix3(std::string_view name, const float* value, bool transpose = false) const;
 void setMatrix4(std::string_view name, const float* value, bool transpose = false) const;
 void setMatrix4(std::string_view name, const net::minecraft::util::math::Matrix4f& value) const;
 [[nodiscard]] static bool supported();

 private:
 struct TransparentStringHash {
  using is_transparent = void;
  std::size_t operator()(std::string_view value) const noexcept {
   return std::hash<std::string_view>{}(value);
  }
 };
 template <typename Value>
 using NameMap = std::unordered_map<std::string, Value, TransparentStringHash, std::equal_to<>>;
 void reflectSamplers();
 void refreshUniformLocations();
 void resetUniformLocations();
 bool extractProgramBinary(ProgramBinaryBlob& out);
 unsigned int program_ = 0;
 mutable int uniformLocations_[static_cast<std::size_t>(IrisUniformSlot::Count)];
 mutable NameMap<int> uniformCache_;
 NameMap<SamplerKind> samplerKinds_;
 std::vector<std::string> samplerNames_;
 bool tessellation_ = false;
 int computeLocalSize_[3] = {1, 1, 1};
 mutable bool fogClass_ = true;
 std::vector<unsigned int> drawBuffers_{};
 std::vector<int> drawBufferColortexIndices_{};
 std::string lastError_;
 mutable unsigned int uniformSnapshotGeneration_ = 0;
};
} // namespace net::minecraft::client::gl
