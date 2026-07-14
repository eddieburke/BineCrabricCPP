#pragma once
#include "net/minecraft/block/Block.hpp"
namespace net::minecraft::block {
// Registered in Block.cpp.
class OreStorageBlock : public Block {
public:
  OreStorageBlock(int id, int textureId) : Block(id, textureId, material::Material::METAL) {
  }
  [[nodiscard]] int getTexture(int /*side*/) const override {
    return textureId;
  }
};
} // namespace net::minecraft::block