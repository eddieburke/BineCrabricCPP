#pragma once
#include <string>

#include "net/minecraft/block/entity/BlockEntity.hpp"

namespace net::minecraft {
class World;
}

namespace net::minecraft::client::font {
class TextRenderer;
}

namespace net::minecraft::client::render::block::entity {
class BlockEntityRenderDispatcher;

class BlockEntityRenderer {
   public:
    virtual ~BlockEntityRenderer() = default;
    virtual void render(const net::minecraft::block::entity::BlockEntity& blockEntity,
                        double x,
                        double y,
                        double z,
                        float tickDelta) = 0;

    virtual void setDispatcher(BlockEntityRenderDispatcher* dispatcherIn) {
        dispatcher = dispatcherIn;
    }

    virtual void setWorld(net::minecraft::World* world) {
        (void) world;
    }

    [[nodiscard]] font::TextRenderer* getTextRenderer() const;

   protected:
    void bindTexture(const std::string& path);
    BlockEntityRenderDispatcher* dispatcher = nullptr;
};
}  // namespace net::minecraft::client::render::block::entity
