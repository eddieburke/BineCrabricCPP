#include "net/minecraft/client/render/block/BlockRenderers.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/LiquidBlock.hpp"
#include "net/minecraft/world/BlockView.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/client/option/ResolvedRenderOptions.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/block/BlockVertexEmitter.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include <array>
namespace net::minecraft::client::render::block {
namespace option = net::minecraft::client::option;
bool FluidBlockRenderer::renderFluid(net::minecraft::block::Block& block, int x, int y, int z) {
  Tessellator& tessellator = *ctx_.tess;
  int colorMult = block.getColorMultiplier(ctx_.blockView, x, y, z);
  float red = (float)(colorMult >> 16 & 0xFF) / 255.0f;
  float green = (float)(colorMult >> 8 & 0xFF) / 255.0f;
  float blue = (float)(colorMult & 0xFF) / 255.0f;
  bool topVisible = block.isSideVisible(ctx_.blockView, x, y + 1, z, 1);
  bool bottomVisible = block.isSideVisible(ctx_.blockView, x, y - 1, z, 0);
  std::array<bool, 4> sideVisible{
      block.isSideVisible(ctx_.blockView, x, y, z - 1, 2),
      block.isSideVisible(ctx_.blockView, x, y, z + 1, 3),
      block.isSideVisible(ctx_.blockView, x - 1, y, z, 4),
      block.isSideVisible(ctx_.blockView, x + 1, y, z, 5),
  };
  if(!(topVisible || bottomVisible || sideVisible[0] || sideVisible[1] || sideVisible[2] || sideVisible[3])) {
    return false;
  }
  bool drewAnyFace = false;
  const float downShade = 0.5f;
  const float upShade = 1.0f;
  const float horizShade = 0.8f;
  const float nsShade = 0.6f;
  const double minY = 0.0;
  const double maxY = 1.0;
  net::minecraft::block::material::Material& material = block.material;
  int meta = ctx_.blockView->getBlockMeta(x, y, z);
  float h00 = getFluidHeight(x, y, z, material);
  float h01 = getFluidHeight(x, y, z + 1, material);
  float h11 = getFluidHeight(x + 1, y, z + 1, material);
  float h10 = getFluidHeight(x + 1, y, z, material);
  if(ctx_.skipFaceCulling || topVisible) {
    drewAnyFace = true;
    int topTex = block.getTexture(1, meta);
    float flowAngle = 0.0f;
    if(ctx_.opts.fancyWater) {
      flowAngle = static_cast<float>(
          net::minecraft::block::LiquidBlock::getFlowingAngle(ctx_.blockView, x, y, z, material));
    }
    if(flowAngle > -999.0f) {
      topTex = block.getTexture(2, meta);
    }
    const int topTexU = (topTex & 0xF) << 4;
    const int topTexV = topTex & 0xF0;
    double uvBaseU = (static_cast<double>(topTexU) + 8.0) / 256.0;
    double uvBaseV = (static_cast<double>(topTexV) + 8.0) / 256.0;
    if(flowAngle < -999.0f) {
      flowAngle = 0.0f;
    } else {
      uvBaseU = static_cast<float>(topTexU + 16) / 256.0f;
      uvBaseV = static_cast<float>(topTexV + 16) / 256.0f;
    }
    const float flowSin = net::minecraft::util::math::MathHelper::sin(flowAngle) * 8.0f / 256.0f;
    const float flowCos = net::minecraft::util::math::MathHelper::cos(flowAngle) * 8.0f / 256.0f;
    const float topBrightness = block.getLuminance(ctx_.blockView, x, y, z);
    tessellator.color(upShade * topBrightness * red, upShade * topBrightness * green,
                      upShade * topBrightness * blue);
    const double topU0 = uvBaseU - static_cast<double>(flowCos) - static_cast<double>(flowSin);
    const double topV0 = uvBaseV - static_cast<double>(flowCos) + static_cast<double>(flowSin);
    const double topU1 = uvBaseU - static_cast<double>(flowCos) + static_cast<double>(flowSin);
    const double topV1 = uvBaseV + static_cast<double>(flowCos) + static_cast<double>(flowSin);
    const double topU2 = uvBaseU + static_cast<double>(flowCos) + static_cast<double>(flowSin);
    const double topV2 = uvBaseV + static_cast<double>(flowCos) - static_cast<double>(flowSin);
    const double topU3 = uvBaseU + static_cast<double>(flowCos) - static_cast<double>(flowSin);
    const double topV3 = uvBaseV - static_cast<double>(flowCos) - static_cast<double>(flowSin);
    emitBlockVertex(tessellator, 0.0f, 1.0f, 0.0f, x + 0, static_cast<float>(y) + h00, z + 0, topU0, topV0);
    emitBlockVertex(tessellator, 0.0f, 1.0f, 0.0f, x + 0, static_cast<float>(y) + h01, z + 1, topU1, topV1);
    emitBlockVertex(tessellator, 0.0f, 1.0f, 0.0f, x + 1, static_cast<float>(y) + h11, z + 1, topU2, topV2);
    emitBlockVertex(tessellator, 0.0f, 1.0f, 0.0f, x + 1, static_cast<float>(y) + h10, z + 0, topU3, topV3);
  }
  if(ctx_.skipFaceCulling || bottomVisible) {
    float brightness = block.getLuminance(ctx_.blockView, x, y - 1, z);
    tessellator.color(downShade * brightness, downShade * brightness, downShade * brightness);
    faces_.renderBottomFace(block, x, y, z, block.getTexture(0));
    drewAnyFace = true;
  }
  for(int i = 0; i < 4; ++i) {
    float sideH0;
    float sideH1;
    float sideX0;
    float sideX1;
    float sideZ0;
    float sideZ1;
    int nx = x;
    int ny = y;
    int nz = z;
    if(i == 0) {
      --nz;
    }
    if(i == 1) {
      ++nz;
    }
    if(i == 2) {
      --nx;
    }
    if(i == 3) {
      ++nx;
    }
    const int sideTex = ctx_.resolveTexture(i + 2, block.getTexture(i + 2, meta));
    const net::minecraft::block::TerrainAtlasUv baseUv = net::minecraft::block::Block::terrainTileUv(sideTex);
    const double tileHeight = baseUv.vMax - baseUv.vMin;
    if(!ctx_.skipFaceCulling && !sideVisible[i])
      continue;
    if(i == 0) {
      sideH0 = h00;
      sideH1 = h10;
      sideX0 = x;
      sideX1 = x + 1;
      sideZ0 = z;
      sideZ1 = z;
    } else if(i == 1) {
      sideH0 = h11;
      sideH1 = h01;
      sideX0 = x + 1;
      sideX1 = x;
      sideZ0 = z + 1;
      sideZ1 = z + 1;
    } else if(i == 2) {
      sideH0 = h01;
      sideH1 = h00;
      sideX0 = x;
      sideX1 = x;
      sideZ0 = z + 1;
      sideZ1 = z;
    } else {
      sideH0 = h10;
      sideH1 = h11;
      sideX0 = x + 1;
      sideX1 = x + 1;
      sideZ0 = z;
      sideZ1 = z + 1;
    }
    drewAnyFace = true;
    const double uMin = baseUv.uMin;
    const double uMax = baseUv.uMax;
    const double vTop0 = baseUv.vMin + static_cast<double>(1.0f - sideH0) * tileHeight;
    const double vTop1 = baseUv.vMin + static_cast<double>(1.0f - sideH1) * tileHeight;
    const double vBottom = baseUv.vMax;
    float sideNx = 0.0f;
    float sideNy = 0.0f;
    float sideNz = 0.0f;
    if(i == 0) {
      sideNz = -1.0f;
    } else if(i == 1) {
      sideNz = 1.0f;
    } else if(i == 2) {
      sideNx = -1.0f;
    } else {
      sideNx = 1.0f;
    }
    float brightness = block.getLuminance(ctx_.blockView, nx, ny, nz);
    brightness *= i < 2 ? horizShade : nsShade;
    tessellator.color(upShade * brightness * red, upShade * brightness * green, upShade * brightness * blue);
    emitBlockVertex(tessellator, sideNx, sideNy, sideNz, sideX0, (float)y + sideH0, sideZ0, uMin, vTop0);
    emitBlockVertex(tessellator, sideNx, sideNy, sideNz, sideX1, (float)y + sideH1, sideZ1, uMax, vTop1);
    emitBlockVertex(tessellator, sideNx, sideNy, sideNz, sideX1, y + 0, sideZ1, uMax, vBottom);
    emitBlockVertex(tessellator, sideNx, sideNy, sideNz, sideX0, y + 0, sideZ0, uMin, vBottom);
  }
  ctx_.renderBounds.minY = minY;
  ctx_.renderBounds.maxY = maxY;
  return drewAnyFace;
}
float FluidBlockRenderer::getFluidHeight(int x, int y, int z, net::minecraft::block::material::Material& material) {
  int sampleWeight = 0;
  float heightSum = 0.0f;
  for(int i = 0; i < 4; ++i) {
    const int nx = x - (i & 1);
    const int ny = y;
    const int nz = z - (i >> 1 & 1);
    if(&ctx_.blockView->getMaterial(nx, ny + 1, nz) == &material) {
      return 1.0f;
    }
    net::minecraft::block::material::Material& neighborMaterial = ctx_.blockView->getMaterial(nx, ny, nz);
    if(&neighborMaterial == &material) {
      const int neighborMeta = ctx_.blockView->getBlockMeta(nx, ny, nz);
      if(neighborMeta >= 8 || neighborMeta == 0) {
        heightSum += net::minecraft::block::LiquidBlock::getFluidHeightFromMeta(neighborMeta) * 10.0f;
        sampleWeight += 10;
      }
      heightSum += net::minecraft::block::LiquidBlock::getFluidHeightFromMeta(neighborMeta);
      ++sampleWeight;
      continue;
    }
    if(neighborMaterial.isSolid()) {
      continue;
    }
    heightSum += 1.0f;
    ++sampleWeight;
  }
  return 1.0f - heightSum / static_cast<float>(sampleWeight);
}
} // namespace net::minecraft::client::render::block
