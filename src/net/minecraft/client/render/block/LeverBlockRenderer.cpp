#include "net/minecraft/client/render/block/LeverBlockRenderer.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/util/math/Vec3dClient.hpp"
#include "net/minecraft/world/BlockView.hpp"

#include <array>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace net::minecraft::client::render::block {
bool LeverBlockRenderer::render(net::minecraft::block::Block& block, int x, int y, int z)
{

        bool bl;
        int n = ctx_.blockView->getBlockMeta(x, y, z);
        int n2 = n & 7;
        bool bl2 = (n & 8) > 0;
        Tessellator& tessellator = render::INSTANCE;
        bl = ctx_.textureOverride >= 0;
        if (!bl) {
            ctx_.textureOverride = net::minecraft::block::Block::BLOCKS[4]->textureId;
        }
        float f = 0.25f;
        float f2 = 0.1875f;
        float f3 = 0.1875f;
        if (n2 == 5) {
            block.setBoundingBox(0.5f - f2, 0.0f, 0.5f - f, 0.5f + f2, f3, 0.5f + f);
        } else if (n2 == 6) {
            block.setBoundingBox(0.5f - f, 0.0f, 0.5f - f2, 0.5f + f, f3, 0.5f + f2);
        } else if (n2 == 4) {
            block.setBoundingBox(0.5f - f2, 0.5f - f, 1.0f - f3, 0.5f + f2, 0.5f + f, 1.0f);
        } else if (n2 == 3) {
            block.setBoundingBox(0.5f - f2, 0.5f - f, 0.0f, 0.5f + f2, 0.5f + f, f3);
        } else if (n2 == 2) {
            block.setBoundingBox(1.0f - f3, 0.5f - f, 0.5f - f2, 1.0f, 0.5f + f, 0.5f + f2);
        } else if (n2 == 1) {
            block.setBoundingBox(0.0f, 0.5f - f, 0.5f - f2, f3, 0.5f + f, 0.5f + f2);
        }
        cube_.renderBlock(block, x, y, z);
        if (!bl) {
            ctx_.textureOverride = -1;
        }
        float f4 = block.getLuminance(ctx_.blockView, x, y, z);
        if (net::minecraft::block::Block::BLOCKS_LIGHT_LUMINANCE[block.id] > 0) {
            f4 = 1.0f;
        }
        tessellator.color(f4, f4, f4);
        int n3 = block.getTexture(0);
        if (ctx_.textureOverride >= 0) {
            n3 = ctx_.textureOverride;
        }
        int n4 = (n3 & 0xF) << 4;
        int n5 = n3 & 0xF0;
        float f5 = (float)n4 / 256.0f;
        float f6 = ((float)n4 + 15.99f) / 256.0f;
        float f7 = (float)n5 / 256.0f;
        float f8 = ((float)n5 + 15.99f) / 256.0f;
        
        float f9 = 0.0625f;
        float f10 = 0.0625f;
        float f11 = 0.625f;
        std::array<net::minecraft::util::math::ClientVec3d*, 8> vec3dArray {};
        vec3dArray[0] = &net::minecraft::util::math::ClientVec3d::createCached(-f9, 0.0, -f10);
        vec3dArray[1] = &net::minecraft::util::math::ClientVec3d::createCached(f9, 0.0, -f10);
        vec3dArray[2] = &net::minecraft::util::math::ClientVec3d::createCached(f9, 0.0, f10);
        vec3dArray[3] = &net::minecraft::util::math::ClientVec3d::createCached(-f9, 0.0, f10);
        vec3dArray[4] = &net::minecraft::util::math::ClientVec3d::createCached(-f9, f11, -f10);
        vec3dArray[5] = &net::minecraft::util::math::ClientVec3d::createCached(f9, f11, -f10);
        vec3dArray[6] = &net::minecraft::util::math::ClientVec3d::createCached(f9, f11, f10);
        vec3dArray[7] = &net::minecraft::util::math::ClientVec3d::createCached(-f9, f11, f10);
        for (int i = 0; i < 8; ++i) {
            if (bl2) {
                vec3dArray[static_cast<std::size_t>(i)]->z -= 0.0625;
                vec3dArray[static_cast<std::size_t>(i)]->rotateX(0.69813174f);
            } else {
                vec3dArray[static_cast<std::size_t>(i)]->z += 0.0625;
                vec3dArray[static_cast<std::size_t>(i)]->rotateX(-0.69813174f);
            }
            if (n2 == 6) {
                vec3dArray[static_cast<std::size_t>(i)]->rotateY(1.5707964f);
            }
            if (n2 < 5) {
                vec3dArray[static_cast<std::size_t>(i)]->y -= 0.375;
                vec3dArray[static_cast<std::size_t>(i)]->rotateX(1.5707964f);
                if (n2 == 4) {
                    vec3dArray[static_cast<std::size_t>(i)]->rotateY(0.0f);
                }
                if (n2 == 3) {
                    vec3dArray[static_cast<std::size_t>(i)]->rotateY(static_cast<float>(M_PI));
                }
                if (n2 == 2) {
                    vec3dArray[static_cast<std::size_t>(i)]->rotateY(1.5707964f);
                }
                if (n2 == 1) {
                    vec3dArray[static_cast<std::size_t>(i)]->rotateY(-1.5707964f);
                }
                vec3dArray[static_cast<std::size_t>(i)]->x += static_cast<double>(x) + 0.5;
                vec3dArray[static_cast<std::size_t>(i)]->y += static_cast<double>(static_cast<float>(y) + 0.5f);
                vec3dArray[static_cast<std::size_t>(i)]->z += static_cast<double>(z) + 0.5;
                continue;
            }
            vec3dArray[static_cast<std::size_t>(i)]->x += static_cast<double>(x) + 0.5;
            vec3dArray[static_cast<std::size_t>(i)]->y += static_cast<double>(static_cast<float>(y) + 0.125f);
            vec3dArray[static_cast<std::size_t>(i)]->z += static_cast<double>(z) + 0.5;
        }
        net::minecraft::util::math::ClientVec3d* vec3d = nullptr;
        net::minecraft::util::math::ClientVec3d* vec3d2 = nullptr;
        net::minecraft::util::math::ClientVec3d* vec3d3 = nullptr;
        net::minecraft::util::math::ClientVec3d* vec3d4 = nullptr;
        for (int i = 0; i < 6; ++i) {
            if (i == 0) {
                f5 = (float)(n4 + 7) / 256.0f;
                f6 = ((float)(n4 + 9) - 0.01f) / 256.0f;
                f7 = (float)(n5 + 6) / 256.0f;
                f8 = ((float)(n5 + 8) - 0.01f) / 256.0f;
            } else if (i == 2) {
                f5 = (float)(n4 + 7) / 256.0f;
                f6 = ((float)(n4 + 9) - 0.01f) / 256.0f;
                f7 = (float)(n5 + 6) / 256.0f;
                f8 = ((float)(n5 + 16) - 0.01f) / 256.0f;
            }
            if (i == 0) {
                vec3d = vec3dArray[0];
                vec3d2 = vec3dArray[1];
                vec3d3 = vec3dArray[2];
                vec3d4 = vec3dArray[3];
            } else if (i == 1) {
                vec3d = vec3dArray[7];
                vec3d2 = vec3dArray[6];
                vec3d3 = vec3dArray[5];
                vec3d4 = vec3dArray[4];
            } else if (i == 2) {
                vec3d = vec3dArray[1];
                vec3d2 = vec3dArray[0];
                vec3d3 = vec3dArray[4];
                vec3d4 = vec3dArray[5];
            } else if (i == 3) {
                vec3d = vec3dArray[2];
                vec3d2 = vec3dArray[1];
                vec3d3 = vec3dArray[5];
                vec3d4 = vec3dArray[6];
            } else if (i == 4) {
                vec3d = vec3dArray[3];
                vec3d2 = vec3dArray[2];
                vec3d3 = vec3dArray[6];
                vec3d4 = vec3dArray[7];
            } else if (i == 5) {
                vec3d = vec3dArray[0];
                vec3d2 = vec3dArray[3];
                vec3d3 = vec3dArray[7];
                vec3d4 = vec3dArray[4];
            }
            tessellator.vertex(vec3d->x, vec3d->y, vec3d->z, f5, f8);
            tessellator.vertex(vec3d2->x, vec3d2->y, vec3d2->z, f6, f8);
            tessellator.vertex(vec3d3->x, vec3d3->y, vec3d3->z, f6, f7);
            tessellator.vertex(vec3d4->x, vec3d4->y, vec3d4->z, f5, f7);
        }
        return true;
    
}


} // namespace net::minecraft::client::render::block

