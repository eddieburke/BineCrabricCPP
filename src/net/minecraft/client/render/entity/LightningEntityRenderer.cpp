#include "net/minecraft/client/render/entity/LightningEntityRenderer.hpp"

#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/entity/LightningEntity.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"

#include <random>

namespace net::minecraft::client::render::entity {

void LightningEntityRenderer::render(const net::minecraft::Entity& entity, double x, double y, double z, float yaw, float tickDelta)
{
    (void)yaw;
    (void)tickDelta;
    const auto* lightning = dynamic_cast<const net::minecraft::LightningEntity*>(&entity);
    if (lightning == nullptr) {
        return;
    }

    Tessellator& tessellator = Tessellator::INSTANCE;
    gl::GL11::glDisable(3553);
    gl::GL11::glDisable(2896);
    gl::GL11::glEnable(3042);
    gl::GL11::glBlendFunc(770, 1);

    double offsets[8] {};
    double offsets2[8] {};
    double d2 = 0.0;
    double d3 = 0.0;
    std::mt19937_64 rng(static_cast<std::uint64_t>(lightning->seed));
    std::uniform_int_distribution<int> jitter(-5, 5);

    for (int i = 7; i >= 0; --i) {
        offsets[i] = d2;
        offsets2[i] = d3;
        d2 += static_cast<double>(jitter(rng));
        d3 += static_cast<double>(jitter(rng));
    }

    for (int pass = 0; pass < 4; ++pass) {
        std::mt19937_64 rng2(static_cast<std::uint64_t>(lightning->seed));
        std::uniform_int_distribution<int> jitterFine(-15, 15);
        for (int branch = 0; branch < 3; ++branch) {
            int n = 7;
            int n2 = 0;
            if (branch > 0) {
                n = 7 - branch;
            }
            if (branch > 0) {
                n2 = n - 2;
            }
            double d4 = offsets[n] - d2;
            double d5 = offsets2[n] - d3;
            for (int k = n; k >= n2; --k) {
                double d6 = d4;
                double d7 = d5;
                if (branch == 0) {
                    d4 += static_cast<double>(jitter(rng2));
                    d5 += static_cast<double>(jitter(rng2));
                } else {
                    d4 += static_cast<double>(jitterFine(rng2));
                    d5 += static_cast<double>(jitterFine(rng2));
                }
                tessellator.start(5); // GL_TRIANGLE_STRIP
                constexpr float f2 = 0.5f;
                tessellator.color(0.9f * f2, 0.9f * f2, 1.0f * f2, 0.3f);
                double d8 = 0.1 + static_cast<double>(pass) * 0.2;
                if (branch == 0) {
                    d8 *= static_cast<double>(k) * 0.1 + 1.0;
                }
                double d9 = 0.1 + static_cast<double>(pass) * 0.2;
                if (branch == 0) {
                    d9 *= static_cast<double>(k - 1) * 0.1 + 1.0;
                }
                for (int seg = 0; seg < 5; ++seg) {
                    double d10 = x + 0.5 - d8;
                    double d11 = z + 0.5 - d8;
                    if (seg == 1 || seg == 2) {
                        d10 += d8 * 2.0;
                    }
                    if (seg == 2 || seg == 3) {
                        d11 += d8 * 2.0;
                    }
                    double d12 = x + 0.5 - d9;
                    double d13 = z + 0.5 - d9;
                    if (seg == 1 || seg == 2) {
                        d12 += d9 * 2.0;
                    }
                    if (seg == 2 || seg == 3) {
                        d13 += d9 * 2.0;
                    }
                    tessellator.vertex(d12 + d4, y + static_cast<double>(k * 16), d13 + d5);
                    tessellator.vertex(d10 + d6, y + static_cast<double>((k + 1) * 16), d11 + d7);
                }
                tessellator.draw();
            }
        }
    }

    gl::GL11::glDisable(3042);
    gl::GL11::glEnable(2896);
    gl::GL11::glEnable(3553);
}

} // namespace net::minecraft::client::render::entity
