#pragma once

#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/gui/DrawContext.hpp"
#include "net/minecraft/client/gui/GuiParticle.hpp"

#include <memory>
#include <vector>

namespace net::minecraft::client::gui {

class ParticlesGui : public DrawContext {
public:
    explicit ParticlesGui(client::Minecraft* minecraft)
        : minecraft_(minecraft)
    {
    }

    void add(std::unique_ptr<GuiParticle> particle)
    {
        if (particle != nullptr) {
            particles_.push_back(std::move(particle));
        }
    }

    void tick()
    {
        for (std::size_t i = 0; i < particles_.size();) {
            GuiParticle& particle = *particles_[i];
            particle.tickColor();
            particle.tickPosition(this);
            if (particle.removed) {
                particles_.erase(particles_.begin() + static_cast<std::ptrdiff_t>(i));
            } else {
                ++i;
            }
        }
    }

    void render(float tickDelta)
    {
        if (minecraft_ == nullptr) {
            return;
        }
        minecraft_->textureManager.bindTexture(minecraft_->textureManager.getTextureId("/gui/particles.png"));
        for (const auto& particlePtr : particles_) {
            const GuiParticle& particle = *particlePtr;
            const int drawX = static_cast<int>(particle.lastX + (particle.x - particle.lastX) * tickDelta - 4.0);
            const int drawY = static_cast<int>(particle.lastY + (particle.y - particle.lastY) * tickDelta - 4.0);
            const float alpha = static_cast<float>(particle.lastA + (particle.a - particle.lastA) * tickDelta);
            const float red = static_cast<float>(particle.lastR + (particle.r - particle.lastR) * tickDelta);
            const float green = static_cast<float>(particle.lastG + (particle.g - particle.lastG) * tickDelta);
            const float blue = static_cast<float>(particle.lastB + (particle.b - particle.lastB) * tickDelta);
            gl::GL11::glColor4f(red, green, blue, alpha);
            drawTexture(drawX, drawY, 40, 0, 8, 8);
        }
    }

    [[nodiscard]] const std::vector<std::unique_ptr<GuiParticle>>& particles() const noexcept
    {
        return particles_;
    }

private:
    client::Minecraft* minecraft_ = nullptr;
    std::vector<std::unique_ptr<GuiParticle>> particles_;
};

} // namespace net::minecraft::client::gui
