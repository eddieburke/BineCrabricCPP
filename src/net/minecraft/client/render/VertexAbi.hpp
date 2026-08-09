#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace net::minecraft::client::render {
struct TessellatorVertex {
 float x = 0.0f;
 float y = 0.0f;
 float z = 0.0f;
 float u = 0.0f;
 float v = 0.0f;
 std::uint32_t color = 0xFFFFFFFFU;
 std::int32_t normal = 0;
 std::int32_t midBlock = 0;
 std::int32_t light = 0x00F000F0;
 std::int16_t entity[4]{};
 float midU = 0.0f;
 float midV = 0.0f;
 std::int16_t tangent[4]{};
 std::int32_t irisEntity[3]{};
 std::int16_t overlay[2]{};
};

namespace vertex_abi {
inline constexpr unsigned int Position = 0;
inline constexpr unsigned int Texture = 1;
inline constexpr unsigned int Color = 2;
inline constexpr unsigned int Normal = 3;
inline constexpr unsigned int MidBlock = 4;
inline constexpr unsigned int Light = 5;
inline constexpr unsigned int Entity = 6;
inline constexpr unsigned int MidTexCoord = 7;
inline constexpr unsigned int Overlay = 8;
inline constexpr unsigned int IrisEntity = 9;
inline constexpr unsigned int IrisUv2 = 10;
inline constexpr unsigned int Tangent = 11;
inline constexpr unsigned int ChunkFade = 12;
inline constexpr unsigned int IrisLightUv = 13;

struct Binding {
 unsigned int location;
 const char* name;
};

enum class Availability {
 Always,
 Texture,
 Normal
};

struct Format {
 unsigned int location;
 int components;
 unsigned int type;
 bool normalized;
 bool integer;
 std::size_t offset;
 Availability availability;
};

inline constexpr std::array<Binding, 15> Bindings{{
    {Position, "vaPosition"},
    {Texture, "vaUV0"},
    {Color, "vaColor"},
    {Normal, "vaNormal"},
    {MidBlock, "at_midBlock"},
    {Light, "vaUV2"},
    {Entity, "mc_Entity"},
    {MidTexCoord, "mc_midTexCoord"},
    {Overlay, "iris_UV1"},
    {Overlay, "iris_OverlayUV"},
    {IrisEntity, "iris_Entity"},
    {IrisUv2, "iris_UV2"},
    {IrisLightUv, "iris_LightUV"},
    {Tangent, "at_tangent"},
    {ChunkFade, "mc_chunkFade"},
}};

inline constexpr std::size_t PositionOffset = offsetof(TessellatorVertex, x);
inline constexpr std::size_t TextureOffset = offsetof(TessellatorVertex, u);
inline constexpr std::size_t ColorOffset = offsetof(TessellatorVertex, color);
inline constexpr std::size_t NormalOffset = offsetof(TessellatorVertex, normal);
inline constexpr std::size_t MidBlockOffset = offsetof(TessellatorVertex, midBlock);
inline constexpr std::size_t LightOffset = offsetof(TessellatorVertex, light);
inline constexpr std::size_t EntityOffset = offsetof(TessellatorVertex, entity);
inline constexpr std::size_t MidTexCoordOffset = offsetof(TessellatorVertex, midU);
inline constexpr std::size_t TangentOffset = offsetof(TessellatorVertex, tangent);
inline constexpr std::size_t IrisEntityOffset = offsetof(TessellatorVertex, irisEntity);
inline constexpr std::size_t OverlayOffset = offsetof(TessellatorVertex, overlay);
inline constexpr std::size_t Stride = sizeof(TessellatorVertex);

inline constexpr std::array<Format, 13> Formats{{
    {Position, 3, 0x1406, false, false, PositionOffset, Availability::Always},
    {Texture, 2, 0x1406, false, false, TextureOffset, Availability::Texture},
    // Always sourced from the buffer. Every vertex carries a colour (the const
    // colour when no producer set one), so vaColor never rides a generic
    // attribute — that side channel is what let entity draws reach a pack with
    // vaColor = (0,0,0,1) while array-backed geometry stayed white.
    {Color, 4, 0x1401, true, false, ColorOffset, Availability::Always},
    {Normal, 3, 0x1400, true, false, NormalOffset, Availability::Normal},
    {MidBlock, 4, 0x1400, false, false, MidBlockOffset, Availability::Always},
    {Light, 2, 0x1403, false, false, LightOffset, Availability::Always},
    {Entity, 4, 0x1402, false, false, EntityOffset, Availability::Always},
    {MidTexCoord, 2, 0x1406, false, false, MidTexCoordOffset, Availability::Always},
    {Overlay, 2, 0x1402, false, true, OverlayOffset, Availability::Always},
    {IrisEntity, 3, 0x1404, false, true, IrisEntityOffset, Availability::Always},
    {IrisUv2, 2, 0x1403, false, true, LightOffset, Availability::Always},
    {Tangent, 4, 0x1402, true, false, TangentOffset, Availability::Always},
    {IrisLightUv, 2, 0x1403, false, true, LightOffset, Availability::Always},
}};

inline const std::string& abiSaltString() {
 static const std::string salt = [] {
  std::string s;
  for(const auto& binding : Bindings) {
   s += std::to_string(binding.location);
   s += ':';
   s += binding.name;
   s += ';';
  }
  return s;
 }();
 return salt;
}
}

static_assert(vertex_abi::PositionOffset == 0);
static_assert(vertex_abi::TextureOffset == 12);
static_assert(vertex_abi::ColorOffset == 20);
static_assert(vertex_abi::NormalOffset == 24);
static_assert(vertex_abi::MidBlockOffset == 28);
static_assert(vertex_abi::LightOffset == 32);
static_assert(vertex_abi::EntityOffset == 36);
static_assert(vertex_abi::MidTexCoordOffset == 44);
static_assert(vertex_abi::TangentOffset == 52);
static_assert(vertex_abi::IrisEntityOffset == 60);
static_assert(vertex_abi::OverlayOffset == 72);
static_assert(vertex_abi::Stride == 76);
}
