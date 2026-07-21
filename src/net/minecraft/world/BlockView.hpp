#pragma once
namespace net::minecraft::block::material {
class Material;
}
namespace net::minecraft::block::entity {
class BlockEntity;
}
namespace net::minecraft {
class BiomeSource;
class BlockView {
 public:
 virtual ~BlockView() = default;
 [[nodiscard]] virtual int getBlockId(int x, int y, int z) const = 0;
 [[nodiscard]] virtual block::entity::BlockEntity* getBlockEntity(int x, int y, int z) = 0;
 [[nodiscard]] virtual float getNaturalBrightness(int x, int y, int z, int blockLight) const = 0;
 [[nodiscard]] virtual float getLightBrightness(int x, int y, int z) const = 0;
 [[nodiscard]] virtual int getBlockMeta(int x, int y, int z) const = 0;
 [[nodiscard]] virtual block::material::Material& getMaterial(int x, int y, int z) const = 0;
 [[nodiscard]] virtual bool isBlockOpaqueCube(int x, int y, int z) const = 0;
 [[nodiscard]] virtual bool shouldSuffocate(int x, int y, int z) const = 0;
 [[nodiscard]] virtual BiomeSource* getBiomeSource() const = 0;
};
} // namespace net::minecraft
