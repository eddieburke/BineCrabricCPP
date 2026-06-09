#pragma once

#include "net/minecraft/client/render/block/BlockRenderManager.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/item/ItemStack.hpp"

namespace net::minecraft::client::font {
class TextRenderer;
}

namespace net::minecraft::client::texture {
class TextureManager;
}

namespace net::minecraft::client::render::item {

class ItemRenderer {
public:
    ItemRenderer() = default;

    void render(const net::minecraft::Entity& entity, const ItemStack& stack)
    {
        (void)entity;
        (void)stack;
    }

    void renderGuiItem(::net::minecraft::client::font::TextRenderer& textRenderer,
        ::net::minecraft::client::texture::TextureManager& textureManager,
        const ItemStack& stack,
        int x,
        int y);

    void renderGuiItemDecoration(::net::minecraft::client::font::TextRenderer& textRenderer,
        ::net::minecraft::client::texture::TextureManager& textureManager,
        const ItemStack& stack,
        int x,
        int y);

    bool useCustomDisplayColor = true;
    block::BlockRenderManager blockRenderManager {};

private:
    // renderGuiItem dispatches to one of these two depending on the stack.
    void renderBlockItemInGui(::net::minecraft::client::texture::TextureManager& textureManager,
        const ItemStack& stack, int x, int y);
    void renderSpriteItemInGui(::net::minecraft::client::texture::TextureManager& textureManager,
        const ItemStack& stack, int x, int y);

    // renderGuiItemDecoration overlays.
    void drawCountLabel(::net::minecraft::client::font::TextRenderer& textRenderer, const ItemStack& stack, int x, int y);
    void drawDurabilityBar(const ItemStack& stack, int x, int y);

    // Applies the stack's display tint (or white) as the current GL color.
    void applyDisplayColor(const ItemStack& stack);

    void fillRect(int x, int y, int width, int height, int color);
    void drawTexture(int x, int y, int u, int v, int width, int height);
};

} // namespace net::minecraft::client::render::item
