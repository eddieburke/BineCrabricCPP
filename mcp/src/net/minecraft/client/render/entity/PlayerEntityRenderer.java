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
import net.minecraft.client.Minecraft;
import net.minecraft.client.font.TextRenderer;
import net.minecraft.client.render.Tessellator;
import net.minecraft.client.render.block.BlockRenderManager;
import net.minecraft.client.render.entity.LivingEntityRenderer;
import net.minecraft.client.render.entity.model.BipedEntityModel;
import net.minecraft.entity.player.ClientPlayerEntity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.item.ArmorItem;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.util.math.MathHelper;
import org.lwjgl.opengl.GL11;

@Environment(value=EnvType.CLIENT)
public class PlayerEntityRenderer
extends LivingEntityRenderer {
    private BipedEntityModel bipedModel;
    private BipedEntityModel armor1;
    private BipedEntityModel armor2;
    private static final String[] armorTextureNames = new String[]{"cloth", "chain", "iron", "diamond", "gold"};

    public PlayerEntityRenderer() {
        super(new BipedEntityModel(0.0f), 0.5f);
        this.bipedModel = (BipedEntityModel)this.model;
        this.armor1 = new BipedEntityModel(1.0f);
        this.armor2 = new BipedEntityModel(0.5f);
    }

    protected boolean bindTexture(PlayerEntity arg, int i, float f) {
        Item item;
        ItemStack itemStack = arg.inventory.getArmorStack(3 - i);
        if (itemStack != null && (item = itemStack.getItem()) instanceof ArmorItem) {
            ArmorItem armorItem = (ArmorItem)item;
            this.bindTexture("/armor/" + armorTextureNames[armorItem.textureIndex] + "_" + (i == 2 ? 2 : 1) + ".png");
            BipedEntityModel bipedEntityModel = i == 2 ? this.armor2 : this.armor1;
            bipedEntityModel.head.visible = i == 0;
            bipedEntityModel.hat.visible = i == 0;
            bipedEntityModel.body.visible = i == 1 || i == 2;
            bipedEntityModel.rightArm.visible = i == 1;
            bipedEntityModel.leftArm.visible = i == 1;
            bipedEntityModel.rightLeg.visible = i == 2 || i == 3;
            bipedEntityModel.leftLeg.visible = i == 2 || i == 3;
            this.setDecorationModel(bipedEntityModel);
            return true;
        }
        return false;
    }

    public void render(PlayerEntity arg, double d, double e, double f, float g, float h) {
        ItemStack itemStack = arg.inventory.getSelectedItem();
        this.bipedModel.rightArmPose = itemStack != null;
        this.armor2.rightArmPose = this.bipedModel.rightArmPose;
        this.armor1.rightArmPose = this.bipedModel.rightArmPose;
        this.armor2.sneaking = this.bipedModel.sneaking = arg.isSneaking();
        this.armor1.sneaking = this.bipedModel.sneaking;
        double d2 = e - (double)arg.standingEyeHeight;
        if (arg.isSneaking() && !(arg instanceof ClientPlayerEntity)) {
            d2 -= 0.125;
        }
        super.render(arg, d, d2, f, g, h);
        this.bipedModel.sneaking = false;
        this.armor2.sneaking = false;
        this.armor1.sneaking = false;
        this.bipedModel.rightArmPose = false;
        this.armor2.rightArmPose = false;
        this.armor1.rightArmPose = false;
    }

    protected void renderNameTag(PlayerEntity arg, double d, double e, double f) {
        if (Minecraft.isDisplayGui() && arg != this.dispatcher.cameraEntity) {
            float f2;
            float f3 = 1.6f;
            float f4 = 0.016666668f * f3;
            float f5 = arg.getDistance(this.dispatcher.cameraEntity);
            float f6 = f2 = arg.isSneaking() ? 32.0f : 64.0f;
            if (f5 < f2) {
                String string = arg.name;
                if (!arg.isSneaking()) {
                    if (arg.isSleeping()) {
                        this.renderNameTag(arg, string, d, e - 1.5, f, 64);
                    } else {
                        this.renderNameTag(arg, string, d, e, f, 64);
                    }
                } else {
                    TextRenderer textRenderer = this.getTextRenderer();
                    GL11.glPushMatrix();
                    GL11.glTranslatef((float)d + 0.0f, (float)e + 2.3f, (float)f);
                    GL11.glNormal3f(0.0f, 1.0f, 0.0f);
                    GL11.glRotatef(-this.dispatcher.yaw, 0.0f, 1.0f, 0.0f);
                    GL11.glRotatef(this.dispatcher.pitch, 1.0f, 0.0f, 0.0f);
                    GL11.glScalef(-f4, -f4, f4);
                    GL11.glDisable(2896);
                    GL11.glTranslatef(0.0f, 0.25f / f4, 0.0f);
                    GL11.glDepthMask(false);
                    GL11.glEnable(3042);
                    GL11.glBlendFunc(770, 771);
                    Tessellator tessellator = Tessellator.INSTANCE;
                    GL11.glDisable(3553);
                    tessellator.startQuads();
                    int n = textRenderer.getWidth(string) / 2;
                    tessellator.color(0.0f, 0.0f, 0.0f, 0.25f);
                    tessellator.vertex(-n - 1, -1.0, 0.0);
                    tessellator.vertex(-n - 1, 8.0, 0.0);
                    tessellator.vertex(n + 1, 8.0, 0.0);
                    tessellator.vertex(n + 1, -1.0, 0.0);
                    tessellator.draw();
                    GL11.glEnable(3553);
                    GL11.glDepthMask(true);
                    textRenderer.draw(string, -textRenderer.getWidth(string) / 2, 0, 0x20FFFFFF);
                    GL11.glEnable(2896);
                    GL11.glDisable(3042);
                    GL11.glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
                    GL11.glPopMatrix();
                }
            }
        }
    }

    protected void renderMore(PlayerEntity arg, float f) {
        ItemStack itemStack;
        float f2;
        ItemStack itemStack2 = arg.inventory.getArmorStack(3);
        if (itemStack2 != null && itemStack2.getItem().id < 256) {
            GL11.glPushMatrix();
            this.bipedModel.head.transform(0.0625f);
            if (BlockRenderManager.isSideLit(Block.BLOCKS[itemStack2.itemId].getRenderType())) {
                float f3 = 0.625f;
                GL11.glTranslatef(0.0f, -0.25f, 0.0f);
                GL11.glRotatef(180.0f, 0.0f, 1.0f, 0.0f);
                GL11.glScalef(f3, -f3, f3);
            }
            this.dispatcher.heldItemRenderer.renderItem(arg, itemStack2);
            GL11.glPopMatrix();
        }
        if (arg.name.equals("deadmau5") && this.bindDownloadedTexture(arg.skinUrl, null)) {
            for (int i = 0; i < 2; ++i) {
                f2 = arg.prevYaw + (arg.yaw - arg.prevYaw) * f - (arg.lastBodyYaw + (arg.bodyYaw - arg.lastBodyYaw) * f);
                float f4 = arg.prevPitch + (arg.pitch - arg.prevPitch) * f;
                GL11.glPushMatrix();
                GL11.glRotatef(f2, 0.0f, 1.0f, 0.0f);
                GL11.glRotatef(f4, 1.0f, 0.0f, 0.0f);
                GL11.glTranslatef(0.375f * (float)(i * 2 - 1), 0.0f, 0.0f);
                GL11.glTranslatef(0.0f, -0.375f, 0.0f);
                GL11.glRotatef(-f4, 1.0f, 0.0f, 0.0f);
                GL11.glRotatef(-f2, 0.0f, 1.0f, 0.0f);
                float f5 = 1.3333334f;
                GL11.glScalef(f5, f5, f5);
                this.bipedModel.renderEars(0.0625f);
                GL11.glPopMatrix();
            }
        }
        if (this.bindDownloadedTexture(arg.playerCapeUrl, null)) {
            GL11.glPushMatrix();
            GL11.glTranslatef(0.0f, 0.0f, 0.125f);
            double d = arg.prevCapeX + (arg.capeX - arg.prevCapeX) * (double)f - (arg.prevX + (arg.x - arg.prevX) * (double)f);
            double d2 = arg.prevCapeY + (arg.capeY - arg.prevCapeY) * (double)f - (arg.prevY + (arg.y - arg.prevY) * (double)f);
            double d3 = arg.prevCapeZ + (arg.capeZ - arg.prevCapeZ) * (double)f - (arg.prevZ + (arg.z - arg.prevZ) * (double)f);
            float f6 = arg.lastBodyYaw + (arg.bodyYaw - arg.lastBodyYaw) * f;
            double d4 = MathHelper.sin(f6 * (float)Math.PI / 180.0f);
            double d5 = -MathHelper.cos(f6 * (float)Math.PI / 180.0f);
            float f7 = (float)d2 * 10.0f;
            if (f7 < -6.0f) {
                f7 = -6.0f;
            }
            if (f7 > 32.0f) {
                f7 = 32.0f;
            }
            float f8 = (float)(d * d4 + d3 * d5) * 100.0f;
            float f9 = (float)(d * d5 - d3 * d4) * 100.0f;
            if (f8 < 0.0f) {
                f8 = 0.0f;
            }
            float f10 = arg.prevStepBobbingAmount + (arg.stepBobbingAmount - arg.prevStepBobbingAmount) * f;
            f7 += MathHelper.sin((arg.prevHorizontalSpeed + (arg.horizontalSpeed - arg.prevHorizontalSpeed) * f) * 6.0f) * 32.0f * f10;
            if (arg.isSneaking()) {
                f7 += 25.0f;
            }
            GL11.glRotatef(6.0f + f8 / 2.0f + f7, 1.0f, 0.0f, 0.0f);
            GL11.glRotatef(f9 / 2.0f, 0.0f, 0.0f, 1.0f);
            GL11.glRotatef(-f9 / 2.0f, 0.0f, 1.0f, 0.0f);
            GL11.glRotatef(180.0f, 0.0f, 1.0f, 0.0f);
            this.bipedModel.renderCape(0.0625f);
            GL11.glPopMatrix();
        }
        if ((itemStack = arg.inventory.getSelectedItem()) != null) {
            GL11.glPushMatrix();
            this.bipedModel.rightArm.transform(0.0625f);
            GL11.glTranslatef(-0.0625f, 0.4375f, 0.0625f);
            if (arg.fishHook != null) {
                itemStack = new ItemStack(Item.STICK);
            }
            if (itemStack.itemId < 256 && BlockRenderManager.isSideLit(Block.BLOCKS[itemStack.itemId].getRenderType())) {
                f2 = 0.5f;
                GL11.glTranslatef(0.0f, 0.1875f, -0.3125f);
                GL11.glRotatef(20.0f, 1.0f, 0.0f, 0.0f);
                GL11.glRotatef(45.0f, 0.0f, 1.0f, 0.0f);
                GL11.glScalef(f2 *= 0.75f, -f2, f2);
            } else if (Item.ITEMS[itemStack.itemId].isHandheld()) {
                f2 = 0.625f;
                if (Item.ITEMS[itemStack.itemId].isHandheldRod()) {
                    GL11.glRotatef(180.0f, 0.0f, 0.0f, 1.0f);
                    GL11.glTranslatef(0.0f, -0.125f, 0.0f);
                }
                GL11.glTranslatef(0.0f, 0.1875f, 0.0f);
                GL11.glScalef(f2, -f2, f2);
                GL11.glRotatef(-100.0f, 1.0f, 0.0f, 0.0f);
                GL11.glRotatef(45.0f, 0.0f, 1.0f, 0.0f);
            } else {
                f2 = 0.375f;
                GL11.glTranslatef(0.25f, 0.1875f, -0.1875f);
                GL11.glScalef(f2, f2, f2);
                GL11.glRotatef(60.0f, 0.0f, 0.0f, 1.0f);
                GL11.glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
                GL11.glRotatef(20.0f, 0.0f, 0.0f, 1.0f);
            }
            this.dispatcher.heldItemRenderer.renderItem(arg, itemStack);
            GL11.glPopMatrix();
        }
    }

    protected void applyScale(PlayerEntity arg, float f) {
        float f2 = 0.9375f;
        GL11.glScalef(f2, f2, f2);
    }

    public void renderHand() {
        this.bipedModel.handSwingProgress = 0.0f;
        this.bipedModel.setAngles(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0625f);
        this.bipedModel.rightArm.render(0.0625f);
    }

    protected void applyTranslation(PlayerEntity arg, double d, double e, double f) {
        if (arg.isAlive() && arg.isSleeping()) {
            super.applyTranslation(arg, d + (double)arg.sleepOffsetX, e + (double)arg.sleepOffsetY, f + (double)arg.sleepOffsetZ);
        } else {
            super.applyTranslation(arg, d, e, f);
        }
    }

    protected void applyHandSwingRotation(PlayerEntity arg, float f, float g, float h) {
        if (arg.isAlive() && arg.isSleeping()) {
            GL11.glRotatef(arg.getSleepingRotation(), 0.0f, 1.0f, 0.0f);
            GL11.glRotatef(this.getDeathYaw(arg), 0.0f, 0.0f, 1.0f);
            GL11.glRotatef(270.0f, 0.0f, 1.0f, 0.0f);
        } else {
            super.applyHandSwingRotation(arg, f, g, h);
        }
    }
}

