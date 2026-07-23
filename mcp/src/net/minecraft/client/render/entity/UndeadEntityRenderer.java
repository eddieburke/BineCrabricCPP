/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.render.entity;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.Block;
import net.minecraft.client.render.block.BlockRenderManager;
import net.minecraft.client.render.entity.LivingEntityRenderer;
import net.minecraft.client.render.entity.model.BipedEntityModel;
import net.minecraft.entity.LivingEntity;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import org.lwjgl.opengl.GL11;

@Environment(value=EnvType.CLIENT)
public class UndeadEntityRenderer
extends LivingEntityRenderer {
    protected BipedEntityModel entityModel;

    public UndeadEntityRenderer(BipedEntityModel model, float shadowSize) {
        super(model, shadowSize);
        this.entityModel = model;
    }

    protected void renderMore(LivingEntity entity, float tickDelta) {
        ItemStack itemStack = entity.getHeldItem();
        if (itemStack != null) {
            GL11.glPushMatrix();
            this.entityModel.rightArm.transform(0.0625f);
            GL11.glTranslatef(-0.0625f, 0.4375f, 0.0625f);
            if (itemStack.itemId < 256 && BlockRenderManager.isSideLit(Block.BLOCKS[itemStack.itemId].getRenderType())) {
                float f = 0.5f;
                GL11.glTranslatef(0.0f, 0.1875f, -0.3125f);
                GL11.glRotatef(20.0f, 1.0f, 0.0f, 0.0f);
                GL11.glRotatef(45.0f, 0.0f, 1.0f, 0.0f);
                GL11.glScalef(f *= 0.75f, -f, f);
            } else if (Item.ITEMS[itemStack.itemId].isHandheld()) {
                float f = 0.625f;
                GL11.glTranslatef(0.0f, 0.1875f, 0.0f);
                GL11.glScalef(f, -f, f);
                GL11.glRotatef(-100.0f, 1.0f, 0.0f, 0.0f);
                GL11.glRotatef(45.0f, 0.0f, 1.0f, 0.0f);
            } else {
                float f = 0.375f;
                GL11.glTranslatef(0.25f, 0.1875f, -0.1875f);
                GL11.glScalef(f, f, f);
                GL11.glRotatef(60.0f, 0.0f, 0.0f, 1.0f);
                GL11.glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
                GL11.glRotatef(20.0f, 0.0f, 0.0f, 1.0f);
            }
            this.dispatcher.heldItemRenderer.renderItem(entity, itemStack);
            GL11.glPopMatrix();
        }
    }
}

