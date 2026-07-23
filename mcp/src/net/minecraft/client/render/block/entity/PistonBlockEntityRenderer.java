/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.render.block.entity;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.Block;
import net.minecraft.block.PistonBlock;
import net.minecraft.block.entity.PistonBlockEntity;
import net.minecraft.client.Minecraft;
import net.minecraft.client.render.Tessellator;
import net.minecraft.client.render.block.BlockRenderManager;
import net.minecraft.client.render.block.entity.BlockEntityRenderer;
import net.minecraft.client.render.platform.Lighting;
import net.minecraft.world.World;
import org.lwjgl.opengl.GL11;

@Environment(value=EnvType.CLIENT)
public class PistonBlockEntityRenderer
extends BlockEntityRenderer {
    private BlockRenderManager blockRenderManager;

    public void render(PistonBlockEntity arg, double d, double e, double f, float g) {
        Block block = Block.BLOCKS[arg.getPushedBlockId()];
        if (block != null && arg.getProgress(g) < 1.0f) {
            Tessellator tessellator = Tessellator.INSTANCE;
            this.bindTexture("/terrain.png");
            Lighting.turnOff();
            GL11.glBlendFunc(770, 771);
            GL11.glEnable(3042);
            GL11.glDisable(2884);
            if (Minecraft.isAmbientOcclusionEnabled()) {
                GL11.glShadeModel(7425);
            } else {
                GL11.glShadeModel(7424);
            }
            tessellator.startQuads();
            tessellator.translate((double)((float)d - (float)arg.x + arg.getRenderOffsetX(g)), (double)((float)e - (float)arg.y + arg.getRenderOffsetY(g)), (double)((float)f - (float)arg.z + arg.getRenderOffsetZ(g)));
            tessellator.color(1, 1, 1);
            if (block == Block.PISTON_HEAD && arg.getProgress(g) < 0.5f) {
                this.blockRenderManager.renderPistonHeadWithoutCulling(block, arg.x, arg.y, arg.z, false);
            } else if (arg.isSource() && !arg.isExtending()) {
                Block.PISTON_HEAD.setSprite(((PistonBlock)block).getTopTexture());
                this.blockRenderManager.renderPistonHeadWithoutCulling(Block.PISTON_HEAD, arg.x, arg.y, arg.z, arg.getProgress(g) < 0.5f);
                Block.PISTON_HEAD.clearSprite();
                tessellator.translate((double)((float)d - (float)arg.x), (double)((float)e - (float)arg.y), (double)((float)f - (float)arg.z));
                this.blockRenderManager.renderExtendedPiston(block, arg.x, arg.y, arg.z);
            } else {
                this.blockRenderManager.renderWithoutCulling(block, arg.x, arg.y, arg.z);
            }
            tessellator.translate(0.0, 0.0, 0.0);
            tessellator.draw();
            Lighting.turnOn();
        }
    }

    public void setWorld(World world) {
        this.blockRenderManager = new BlockRenderManager(world);
    }
}

