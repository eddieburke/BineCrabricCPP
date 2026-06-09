#include "net/minecraft/client/render/entity/UndeadEntityRenderer.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/render/block/BlockRenderManager.hpp"
#include "net/minecraft/client/render/entity/EntityRenderDispatcher.hpp"
#include "net/minecraft/client/render/item/HeldItemRenderer.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"

namespace net::minecraft::client::render::entity {

UndeadEntityRenderer::UndeadEntityRenderer(model::BipedEntityModel* model, float shadowSize)
    : LivingEntityRenderer(model, shadowSize)
    , entityModel_(model)
{
}

void UndeadEntityRenderer::renderMore(const net::minecraft::LivingEntity& entity, float tickDelta)
{
    (void)tickDelta;
    const ItemStack itemStack = entity.getHeldItem();
    if (itemStack.itemId == 0 || entityModel_ == nullptr || dispatcher == nullptr) {
        return;
    }

    gl::GL11::glPushMatrix();
    entityModel_->rightArm.transform(0.0625f);
    gl::GL11::glTranslatef(-0.0625f, 0.4375f, 0.0625f);
    if (itemStack.itemId < 256) {
        net::minecraft::block::Block* block = net::minecraft::block::Block::BLOCKS[static_cast<std::size_t>(itemStack.itemId)];
        if (block != nullptr && block::BlockRenderManager::isSideLit(block->getRenderType())) {
            float scale = 0.5f;
            gl::GL11::glTranslatef(0.0f, 0.1875f, -0.3125f);
            gl::GL11::glRotatef(20.0f, 1.0f, 0.0f, 0.0f);
            gl::GL11::glRotatef(45.0f, 0.0f, 1.0f, 0.0f);
            scale *= 0.75f;
            gl::GL11::glScalef(scale, -scale, scale);
        } else if (Item* item = itemStack.getItem(); item != nullptr && item->isHandheld()) {
            float scale = 0.625f;
            gl::GL11::glTranslatef(0.0f, 0.1875f, 0.0f);
            gl::GL11::glScalef(scale, -scale, scale);
            gl::GL11::glRotatef(-100.0f, 1.0f, 0.0f, 0.0f);
            gl::GL11::glRotatef(45.0f, 0.0f, 1.0f, 0.0f);
        } else {
            const float scale = 0.375f;
            gl::GL11::glTranslatef(0.25f, 0.1875f, -0.1875f);
            gl::GL11::glScalef(scale, scale, scale);
            gl::GL11::glRotatef(60.0f, 0.0f, 0.0f, 1.0f);
            gl::GL11::glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
            gl::GL11::glRotatef(20.0f, 0.0f, 0.0f, 1.0f);
        }
    } else if (Item* item = itemStack.getItem(); item != nullptr && item->isHandheld()) {
        float scale = 0.625f;
        gl::GL11::glTranslatef(0.0f, 0.1875f, 0.0f);
        gl::GL11::glScalef(scale, -scale, scale);
        gl::GL11::glRotatef(-100.0f, 1.0f, 0.0f, 0.0f);
        gl::GL11::glRotatef(45.0f, 0.0f, 1.0f, 0.0f);
    } else {
        const float scale = 0.375f;
        gl::GL11::glTranslatef(0.25f, 0.1875f, -0.1875f);
        gl::GL11::glScalef(scale, scale, scale);
        gl::GL11::glRotatef(60.0f, 0.0f, 0.0f, 1.0f);
        gl::GL11::glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        gl::GL11::glRotatef(20.0f, 0.0f, 0.0f, 1.0f);
    }
    dispatcher->heldItemRenderer()->renderItem(entity, itemStack);
    gl::GL11::glPopMatrix();
}

} // namespace net::minecraft::client::render::entity
