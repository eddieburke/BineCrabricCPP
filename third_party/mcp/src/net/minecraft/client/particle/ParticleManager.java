/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.particle;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.Block;
import net.minecraft.client.particle.BlockParticle;
import net.minecraft.client.particle.Particle;
import net.minecraft.client.render.Tessellator;
import net.minecraft.client.texture.TextureManager;
import net.minecraft.entity.Entity;
import net.minecraft.util.math.MathHelper;
import net.minecraft.world.World;
import org.lwjgl.opengl.GL11;

@Environment(value=EnvType.CLIENT)
public class ParticleManager {
    protected World world;
    private List[] particles = new List[4];
    private TextureManager textureManager;
    private Random random = new Random();

    public ParticleManager(World world, TextureManager textureManager) {
        if (world != null) {
            this.world = world;
        }
        this.textureManager = textureManager;
        for (int i = 0; i < 4; ++i) {
            this.particles[i] = new ArrayList();
        }
    }

    public void addParticle(Particle particle) {
        int n = particle.getGroup();
        if (this.particles[n].size() >= 4000) {
            this.particles[n].remove(0);
        }
        this.particles[n].add(particle);
    }

    public void removeDeadParticles() {
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < this.particles[i].size(); ++j) {
                Particle particle = (Particle)this.particles[i].get(j);
                particle.tick();
                if (!particle.dead) continue;
                this.particles[i].remove(j--);
            }
        }
    }

    public void render(Entity entity, float partialTicks) {
        float f = MathHelper.cos(entity.yaw * (float)Math.PI / 180.0f);
        float f2 = MathHelper.sin(entity.yaw * (float)Math.PI / 180.0f);
        float f3 = -f2 * MathHelper.sin(entity.pitch * (float)Math.PI / 180.0f);
        float f4 = f * MathHelper.sin(entity.pitch * (float)Math.PI / 180.0f);
        float f5 = MathHelper.cos(entity.pitch * (float)Math.PI / 180.0f);
        Particle.xOffset = entity.lastTickX + (entity.x - entity.lastTickX) * (double)partialTicks;
        Particle.yOffset = entity.lastTickY + (entity.y - entity.lastTickY) * (double)partialTicks;
        Particle.zOffset = entity.lastTickZ + (entity.z - entity.lastTickZ) * (double)partialTicks;
        for (int i = 0; i < 3; ++i) {
            if (this.particles[i].size() == 0) continue;
            int n = 0;
            if (i == 0) {
                n = this.textureManager.getTextureId("/particles.png");
            }
            if (i == 1) {
                n = this.textureManager.getTextureId("/terrain.png");
            }
            if (i == 2) {
                n = this.textureManager.getTextureId("/gui/items.png");
            }
            GL11.glBindTexture(3553, n);
            Tessellator tessellator = Tessellator.INSTANCE;
            tessellator.startQuads();
            for (int j = 0; j < this.particles[i].size(); ++j) {
                Particle particle = (Particle)this.particles[i].get(j);
                particle.render(tessellator, partialTicks, f, f5, f2, f3, f4);
            }
            tessellator.draw();
        }
    }

    public void renderLit(Entity entity, float partialTicks) {
        int n = 3;
        if (this.particles[n].size() == 0) {
            return;
        }
        Tessellator tessellator = Tessellator.INSTANCE;
        for (int i = 0; i < this.particles[n].size(); ++i) {
            Particle particle = (Particle)this.particles[n].get(i);
            particle.render(tessellator, partialTicks, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        }
    }

    public void setWorld(World world) {
        this.world = world;
        for (int i = 0; i < 4; ++i) {
            this.particles[i].clear();
        }
    }

    public void addBlockBreakParticles(int x, int y, int z, int blockId, int blockMeta) {
        if (blockId == 0) {
            return;
        }
        Block block = Block.BLOCKS[blockId];
        int n = 4;
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                for (int k = 0; k < n; ++k) {
                    double d = (double)x + ((double)i + 0.5) / (double)n;
                    double d2 = (double)y + ((double)j + 0.5) / (double)n;
                    double d3 = (double)z + ((double)k + 0.5) / (double)n;
                    int n2 = this.random.nextInt(6);
                    this.addParticle(new BlockParticle(this.world, d, d2, d3, d - (double)x - 0.5, d2 - (double)y - 0.5, d3 - (double)z - 0.5, block, n2, blockMeta).color(x, y, z));
                }
            }
        }
    }

    public void addBlockBreakingParticles(int x, int y, int z, int side) {
        int n = this.world.getBlockId(x, y, z);
        if (n == 0) {
            return;
        }
        Block block = Block.BLOCKS[n];
        float f = 0.1f;
        double d = (double)x + this.random.nextDouble() * (block.maxX - block.minX - (double)(f * 2.0f)) + (double)f + block.minX;
        double d2 = (double)y + this.random.nextDouble() * (block.maxY - block.minY - (double)(f * 2.0f)) + (double)f + block.minY;
        double d3 = (double)z + this.random.nextDouble() * (block.maxZ - block.minZ - (double)(f * 2.0f)) + (double)f + block.minZ;
        if (side == 0) {
            d2 = (double)y + block.minY - (double)f;
        }
        if (side == 1) {
            d2 = (double)y + block.maxY + (double)f;
        }
        if (side == 2) {
            d3 = (double)z + block.minZ - (double)f;
        }
        if (side == 3) {
            d3 = (double)z + block.maxZ + (double)f;
        }
        if (side == 4) {
            d = (double)x + block.minX - (double)f;
        }
        if (side == 5) {
            d = (double)x + block.maxX + (double)f;
        }
        this.addParticle(new BlockParticle(this.world, d, d2, d3, 0.0, 0.0, 0.0, block, side, this.world.getBlockMeta(x, y, z)).color(x, y, z).multiplyVelocity(0.2f).setScale(0.6f));
    }

    public String toString() {
        return "" + (this.particles[0].size() + this.particles[1].size() + this.particles[2].size());
    }
}

