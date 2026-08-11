#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include "net/minecraft/client/texture/TextureManager.hpp"
namespace net::minecraft::client::render {
// https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/pbr/texture/PBRTextureManager.java
class PbrTextures {
 public:
 // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/pbr/texture/PBRType.java
 enum class Type { Normal,
                   Specular };
 // Resolved GL texture ids for one diffuse texture; -1 means "use the 1x1
 // fallback" (PBRType default value), mirroring PBRTextureManager's
 // defaultHolder.
 struct Holder {
  int normal = -1;
  int specular = -1;
 };
 [[nodiscard]] static constexpr const char* suffix(Type type) noexcept {
  return type == Type::Normal ? "_n" : "_s";
 }
 // Port of LinearBlendFunction.blend.
 // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/pbr/mipmap/LinearBlendFunction.java
 [[nodiscard]] static constexpr int pbrLinearBlend4(int v0, int v1, int v2, int v3) noexcept {
  return (v0 + v1 + v2 + v3) / 4;
 }
 // Port of DiscreteBlendFunction.selectTargetType: majority type wins; in a
 // tie, types that appear first in the argument list are given priority.
 // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/pbr/mipmap/DiscreteBlendFunction.java
 [[nodiscard]] static constexpr int pbrSelectTargetType(int t0, int t1, int t2, int t3) noexcept {
  if(t0 != t1 && t0 != t2) {
   if(t2 == t3) {
    return t2;
   }
   if(t0 != t3 && (t1 == t2 || t1 == t3)) {
    return t1;
   }
  }
  return t0;
 }
 // LabPBR specular channel type classifiers
 // (LabPBRTextureFormat.SPECULAR_MIPMAP_GENERATOR).
 // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/pbr/format/LabPBRTextureFormat.java
 [[nodiscard]] static constexpr int pbrSpecularTypeG(int value) noexcept {
  return value < 230 ? 0 : value - 229;
 }
 [[nodiscard]] static constexpr int pbrSpecularTypeB(int value) noexcept {
  return value < 65 ? 0 : 1;
 }
 [[nodiscard]] static constexpr int pbrSpecularTypeA(int value) noexcept {
  return value < 255 ? 0 : 1;
 }
 // Port of DiscreteBlendFunction.blend: picks the majority type and averages
 // the members of that type.
 [[nodiscard]] static inline int pbrDiscreteBlend4(int v0, int v1, int v2, int v3,
                                                   int (*typeOf)(int)) noexcept {
  const int target = pbrSelectTargetType(typeOf(v0), typeOf(v1), typeOf(v2), typeOf(v3));
  int sum = 0;
  int count = 0;
  if(typeOf(v0) == target) {
   sum += v0;
   ++count;
  }
  if(typeOf(v1) == target) {
   sum += v1;
   ++count;
  }
  if(typeOf(v2) == target) {
   sum += v2;
   ++count;
  }
  if(typeOf(v3) == target) {
   sum += v3;
   ++count;
  }
  return sum / count;
 }
 // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/pbr/mipmap/ChannelMipmapGenerator.java
 [[nodiscard]] static inline std::uint32_t pbrBlendMip4(std::uint32_t c0, std::uint32_t c1,
                                                        std::uint32_t c2, std::uint32_t c3,
                                                        Type type, bool labPbr) noexcept {
  const int r0 = static_cast<int>((c0 >> 16) & 0xFF);
  const int r1 = static_cast<int>((c1 >> 16) & 0xFF);
  const int r2 = static_cast<int>((c2 >> 16) & 0xFF);
  const int r3 = static_cast<int>((c3 >> 16) & 0xFF);
  const int g0 = static_cast<int>((c0 >> 8) & 0xFF);
  const int g1 = static_cast<int>((c1 >> 8) & 0xFF);
  const int g2 = static_cast<int>((c2 >> 8) & 0xFF);
  const int g3 = static_cast<int>((c3 >> 8) & 0xFF);
  const int b0 = static_cast<int>(c0 & 0xFF);
  const int b1 = static_cast<int>(c1 & 0xFF);
  const int b2 = static_cast<int>(c2 & 0xFF);
  const int b3 = static_cast<int>(c3 & 0xFF);
  const int a0 = static_cast<int>((c0 >> 24) & 0xFF);
  const int a1 = static_cast<int>((c1 >> 24) & 0xFF);
  const int a2 = static_cast<int>((c2 >> 24) & 0xFF);
  const int a3 = static_cast<int>((c3 >> 24) & 0xFF);
  const bool discrete = type == Type::Specular && labPbr;
  const int red = pbrLinearBlend4(r0, r1, r2, r3);
  const int green = discrete ? pbrDiscreteBlend4(g0, g1, g2, g3, pbrSpecularTypeG)
                             : pbrLinearBlend4(g0, g1, g2, g3);
  const int blue = discrete ? pbrDiscreteBlend4(b0, b1, b2, b3, pbrSpecularTypeB)
                            : pbrLinearBlend4(b0, b1, b2, b3);
  const int alpha = discrete ? pbrDiscreteBlend4(a0, a1, a2, a3, pbrSpecularTypeA)
                             : pbrLinearBlend4(a0, a1, a2, a3);
  return (static_cast<std::uint32_t>(alpha) << 24) | (static_cast<std::uint32_t>(red) << 16) |
         (static_cast<std::uint32_t>(green) << 8) | static_cast<std::uint32_t>(blue);
 }
 // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/pbr/mipmap/AbstractMipmapGenerator.java
 static void pbrDownsampleMip(const std::uint8_t* src, int srcWidth, int srcHeight,
                              std::uint8_t* dst, int dstWidth, int dstHeight,
                              Type type, bool labPbr, int tileColumns = 1,
                              int tileRows = 1) noexcept;
 [[nodiscard]] static net::minecraft::client::texture::RasterImage pbrScaleNearest(const net::minecraft::client::texture::RasterImage& source,
                                                                                   int newWidth, int newHeight);
 [[nodiscard]] static net::minecraft::client::texture::RasterImage pbrScaleBilinear(const net::minecraft::client::texture::RasterImage& source,
                                                                                    int newWidth, int newHeight);
 [[nodiscard]] static Holder getOrLoad(int diffuseTextureId, net::minecraft::client::texture::TextureManager& manager,
                                       bool labPbr);
 // No-op analog of PBRTextureManager.onNewFrame (IrisRenderingPipeline
 // calls it at beginRender): loading is synchronous, so nothing is deferred.
 static void onNewFrame() noexcept;
 // Drops all cached holders. Mirrors PBRTextureManager.clear() on resource
 // reload (MixinTextureManager); textures themselves are owned by the
 // TextureManager and re-applied on the next getOrLoad.
 static void clear() noexcept;
 // Mirrors PBRTextureManager.onDeleteTexture.
 static void onDeleteTexture(int textureId) noexcept;

 private:
 [[nodiscard]] static int resolveCompanion(int diffuseTextureId, Type type,
                                           net::minecraft::client::texture::TextureManager& manager, bool labPbr);
 static void uploadMipLevels(int textureId, const net::minecraft::client::texture::RasterImage& image, Type type,
                             bool labPbr, bool gridAtlas);
 struct CacheEntry {
  Holder holder;
  bool labPbr = false;
 };
 [[nodiscard]] static std::unordered_map<int, CacheEntry>& cache() noexcept;
};
} // namespace net::minecraft::client::render
