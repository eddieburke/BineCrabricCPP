#pragma once
// C++-oriented base for the four animated liquid sprites (lava/water, top/side).
//
// NOT part of the Java beta 1.7.3 class hierarchy: there each of these is an
// independent TextureFX subclass. They are merged here only to share the
// identical 256-cell simulation state (current/next/heat/heatDelta + ticks) and
// the per-cell heat integrator.
//
// Each tick() stays a faithful 1:1 port of its own Java algorithm and is NOT
// merged: the diffusion stencils and colour maps genuinely differ, and the lava
// sprites read heat mid-diffusion while the water sprites use a separate heat
// loop. Sharing tick() would change pixel output, so only state is hoisted.
#include <array>
#include <cstddef>

#include "net/minecraft/client/render/texture/DynamicTexture.hpp"
#include "net/minecraft/client/render/texture/DynamicTextureDetail.hpp"

namespace net::minecraft::client::render::texture {
class LiquidSprite : public DynamicTexture {
   public:
    explicit LiquidSprite(int sprite) : DynamicTexture(sprite) {
    }

   protected:
    // Integrates one heat cell (heat decay + random re-ignition). The caller
    // chooses WHEN this runs relative to reading heat, so the lava (interleaved)
    // and water (separate-loop) orderings — and the single mathRandom() draw per
    // cell — are preserved exactly.
    void integrateHeatCell(int index, float heatRate, float heatDeltaDecay, float spawnValue, double spawnChance) {
        const std::size_t i = static_cast<std::size_t>(index);
        heat[i] += heatDelta[i] * heatRate;
        if (heat[i] < 0.0f) {
            heat[i] = 0.0f;
        }
        heatDelta[i] -= heatDeltaDecay;
        if (detail::mathRandom() < spawnChance) {
            heatDelta[i] = spawnValue;
        }
    }

    std::array<float, 256> current{};
    std::array<float, 256> next{};
    std::array<float, 256> heat{};
    std::array<float, 256> heatDelta{};
    int ticks = 0;
};
}  // namespace net::minecraft::client::render::texture
