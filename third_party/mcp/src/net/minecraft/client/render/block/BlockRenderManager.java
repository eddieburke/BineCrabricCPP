/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.render.block;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.BedBlock;
import net.minecraft.block.Block;
import net.minecraft.block.DoorBlock;
import net.minecraft.block.LiquidBlock;
import net.minecraft.block.PistonBlock;
import net.minecraft.block.PistonHeadBlock;
import net.minecraft.block.RailBlock;
import net.minecraft.block.RedstoneWireBlock;
import net.minecraft.block.RepeaterBlock;
import net.minecraft.block.material.Material;
import net.minecraft.client.Minecraft;
import net.minecraft.client.render.GameRenderer;
import net.minecraft.client.render.Tessellator;
import net.minecraft.util.math.Facings;
import net.minecraft.util.math.MathHelper;
import net.minecraft.util.math.Vec3d;
import net.minecraft.world.BlockView;
import net.minecraft.world.World;
import org.lwjgl.opengl.GL11;

@Environment(value=EnvType.CLIENT)
public class BlockRenderManager {
    private BlockView blockView;
    private int textureOverride = -1;
    private boolean flipTextureHorizontally = false;
    private boolean skipFaceCulling = false;
    public static boolean fancyGraphics = true;
    public boolean inventoryColorEnabled = true;
    private int eastFaceRotation = 0;
    private int westFaceRotation = 0;
    private int southFaceRotation = 0;
    private int northFaceRotation = 0;
    private int topFaceRotation = 0;
    private int bottomFaceRotation = 0;
    private boolean useAo;
    private float selfBrightness;
    private float northBrightness;
    private float bottomBrightness;
    private float eastBrightness;
    private float southBrightness;
    private float topBrightness;
    private float westBrightness;
    private float northEastBottomBrightness;
    private float northBottomBrightness;
    private float northWestBottomBrightness;
    private float eastBottomBrightness;
    private float westBottomBrightness;
    private float southEastBottomBrightness;
    private float southBottomBrightness;
    private float southWestBottomBrightness;
    private float northEastTopBrightness;
    private float northTopBrightness;
    private float northWestTopBrightness;
    private float eastTopBrightness;
    private float southEastTopBrightness;
    private float southTopBrightness;
    private float westTopBrightness;
    private float southWestTopBrightness;
    private float northEastBrightness;
    private float southEastBrightness;
    private float northWestBrightness;
    private float southWestBrightness;
    private int useSurroundingBrightness = 1;
    private float firstVertexRed;
    private float secondVertexRed;
    private float thirdVertexRed;
    private float fourthVertexRed;
    private float firstVertexGreen;
    private float secondVertexGreen;
    private float thirdVertexGreen;
    private float fourthVertexGreen;
    private float firstVertexBlue;
    private float secondVertexBlue;
    private float thirdVertexBlue;
    private float fourthVertexBlue;
    private boolean topNorthEdgeTranslucent;
    private boolean topEastEdgeTranslucent;
    private boolean topWestEdgeTranslucent;
    private boolean topSouthEdgeTranslucent;
    private boolean northWestEdgeTranslucent;
    private boolean southEastEdgeTranslucent;
    private boolean southWestEdgeTranslucent;
    private boolean northEastEdgeTranslucent;
    private boolean bottomNorthEdgeTranslucent;
    private boolean bottomEastEdgeTranslucent;
    private boolean bottomWestEdgeTranslucent;
    private boolean bottomSouthEdgeTranslucent;

    public BlockRenderManager(BlockView blockView) {
        this.blockView = blockView;
    }

    public BlockRenderManager() {
    }

    public void renderWithTexture(Block block, int x, int y, int z, int textureOverride) {
        this.textureOverride = textureOverride;
        this.render(block, x, y, z);
        this.textureOverride = -1;
    }

    public void renderWithoutCulling(Block block, int x, int y, int z) {
        this.skipFaceCulling = true;
        this.render(block, x, y, z);
        this.skipFaceCulling = false;
    }

    public boolean render(Block block, int x, int y, int z) {
        int n = block.getRenderType();
        block.updateBoundingBox(this.blockView, x, y, z);
        if (n == 0) {
            return this.renderBlock(block, x, y, z);
        }
        if (n == 4) {
            return this.renderFluid(block, x, y, z);
        }
        if (n == 13) {
            return this.renderCactus(block, x, y, z);
        }
        if (n == 1) {
            return this.renderCross(block, x, y, z);
        }
        if (n == 6) {
            return this.renderCrop(block, x, y, z);
        }
        if (n == 2) {
            return this.renderTorch(block, x, y, z);
        }
        if (n == 3) {
            return this.renderFire(block, x, y, z);
        }
        if (n == 5) {
            return this.renderRedstoneDust(block, x, y, z);
        }
        if (n == 8) {
            return this.renderLadder(block, x, y, z);
        }
        if (n == 7) {
            return this.renderDoor(block, x, y, z);
        }
        if (n == 9) {
            return this.renderRail((RailBlock)block, x, y, z);
        }
        if (n == 10) {
            return this.renderStairs(block, x, y, z);
        }
        if (n == 11) {
            return this.renderFence(block, x, y, z);
        }
        if (n == 12) {
            return this.renderLever(block, x, y, z);
        }
        if (n == 14) {
            return this.renderBed(block, x, y, z);
        }
        if (n == 15) {
            return this.renderRepeater(block, x, y, z);
        }
        if (n == 16) {
            return this.renderPiston(block, x, y, z, false);
        }
        if (n == 17) {
            return this.renderPistonHead(block, x, y, z, true);
        }
        return false;
    }

    private boolean renderBed(Block block, int x, int y, int z) {
        Tessellator tessellator = Tessellator.INSTANCE;
        int n = this.blockView.getBlockMeta(x, y, z);
        int n2 = BedBlock.getDirection(n);
        boolean bl = BedBlock.isHeadOfBed(n);
        float f = 0.5f;
        float f2 = 1.0f;
        float f3 = 0.8f;
        float f4 = 0.6f;
        float f5 = f2;
        float f6 = f2;
        float f7 = f2;
        float f8 = f;
        float f9 = f3;
        float f10 = f4;
        float f11 = f;
        float f12 = f3;
        float f13 = f4;
        float f14 = f;
        float f15 = f3;
        float f16 = f4;
        float f17 = block.getLuminance(this.blockView, x, y, z);
        tessellator.color(f8 * f17, f11 * f17, f14 * f17);
        int n3 = block.getTextureId(this.blockView, x, y, z, 0);
        int n4 = (n3 & 0xF) << 4;
        int n5 = n3 & 0xF0;
        double d = (float)n4 / 256.0f;
        double d2 = ((double)(n4 + 16) - 0.01) / 256.0;
        double d3 = (float)n5 / 256.0f;
        double d4 = ((double)(n5 + 16) - 0.01) / 256.0;
        double d5 = (double)x + block.minX;
        double d6 = (double)x + block.maxX;
        double d7 = (double)y + block.minY + 0.1875;
        double d8 = (double)z + block.minZ;
        double d9 = (double)z + block.maxZ;
        tessellator.vertex(d5, d7, d9, d, d4);
        tessellator.vertex(d5, d7, d8, d, d3);
        tessellator.vertex(d6, d7, d8, d2, d3);
        tessellator.vertex(d6, d7, d9, d2, d4);
        float f18 = block.getLuminance(this.blockView, x, y + 1, z);
        tessellator.color(f5 * f18, f6 * f18, f7 * f18);
        n4 = block.getTextureId(this.blockView, x, y, z, 1);
        n5 = (n4 & 0xF) << 4;
        int n6 = n4 & 0xF0;
        double d10 = (float)n5 / 256.0f;
        double d11 = ((double)(n5 + 16) - 0.01) / 256.0;
        double d12 = (float)n6 / 256.0f;
        double d13 = ((double)(n6 + 16) - 0.01) / 256.0;
        double d14 = d10;
        double d15 = d11;
        double d16 = d12;
        double d17 = d12;
        double d18 = d10;
        double d19 = d11;
        double d20 = d13;
        double d21 = d13;
        if (n2 == 0) {
            d15 = d10;
            d16 = d13;
            d18 = d11;
            d21 = d12;
        } else if (n2 == 2) {
            d14 = d11;
            d17 = d13;
            d19 = d10;
            d20 = d12;
        } else if (n2 == 3) {
            d14 = d11;
            d17 = d13;
            d19 = d10;
            d20 = d12;
            d15 = d10;
            d16 = d13;
            d18 = d11;
            d21 = d12;
        }
        double d22 = (double)x + block.minX;
        double d23 = (double)x + block.maxX;
        double d24 = (double)y + block.maxY;
        double d25 = (double)z + block.minZ;
        double d26 = (double)z + block.maxZ;
        tessellator.vertex(d23, d24, d26, d18, d20);
        tessellator.vertex(d23, d24, d25, d14, d16);
        tessellator.vertex(d22, d24, d25, d15, d17);
        tessellator.vertex(d22, d24, d26, d19, d21);
        int n7 = Facings.TO_DIR[n2];
        if (bl) {
            n7 = Facings.TO_DIR[Facings.OPPOSITE[n2]];
        }
        n4 = 4;
        switch (n2) {
            case 2: {
                break;
            }
            case 0: {
                n4 = 5;
                break;
            }
            case 3: {
                n4 = 2;
                break;
            }
            case 1: {
                n4 = 3;
            }
        }
        if (n7 != 2 && (this.skipFaceCulling || block.isSideVisible(this.blockView, x, y, z - 1, 2))) {
            float f19 = block.getLuminance(this.blockView, x, y, z - 1);
            if (block.minZ > 0.0) {
                f19 = f17;
            }
            tessellator.color(f9 * f19, f12 * f19, f15 * f19);
            this.flipTextureHorizontally = n4 == 2;
            this.renderEastFace(block, x, y, z, block.getTextureId(this.blockView, x, y, z, 2));
        }
        if (n7 != 3 && (this.skipFaceCulling || block.isSideVisible(this.blockView, x, y, z + 1, 3))) {
            float f20 = block.getLuminance(this.blockView, x, y, z + 1);
            if (block.maxZ < 1.0) {
                f20 = f17;
            }
            tessellator.color(f9 * f20, f12 * f20, f15 * f20);
            this.flipTextureHorizontally = n4 == 3;
            this.renderWestFace(block, x, y, z, block.getTextureId(this.blockView, x, y, z, 3));
        }
        if (n7 != 4 && (this.skipFaceCulling || block.isSideVisible(this.blockView, x - 1, y, z, 4))) {
            float f21 = block.getLuminance(this.blockView, x - 1, y, z);
            if (block.minX > 0.0) {
                f21 = f17;
            }
            tessellator.color(f10 * f21, f13 * f21, f16 * f21);
            this.flipTextureHorizontally = n4 == 4;
            this.renderNorthFace(block, x, y, z, block.getTextureId(this.blockView, x, y, z, 4));
        }
        if (n7 != 5 && (this.skipFaceCulling || block.isSideVisible(this.blockView, x + 1, y, z, 5))) {
            float f22 = block.getLuminance(this.blockView, x + 1, y, z);
            if (block.maxX < 1.0) {
                f22 = f17;
            }
            tessellator.color(f10 * f22, f13 * f22, f16 * f22);
            this.flipTextureHorizontally = n4 == 5;
            this.renderSouthFace(block, x, y, z, block.getTextureId(this.blockView, x, y, z, 5));
        }
        this.flipTextureHorizontally = false;
        return true;
    }

    public boolean renderTorch(Block block, int x, int y, int z) {
        int n = this.blockView.getBlockMeta(x, y, z);
        Tessellator tessellator = Tessellator.INSTANCE;
        float f = block.getLuminance(this.blockView, x, y, z);
        if (Block.BLOCKS_LIGHT_LUMINANCE[block.id] > 0) {
            f = 1.0f;
        }
        tessellator.color(f, f, f);
        double d = 0.4f;
        double d2 = 0.5 - d;
        double d3 = 0.2f;
        if (n == 1) {
            this.renderTiltedTorch(block, (double)x - d2, (double)y + d3, z, -d, 0.0);
        } else if (n == 2) {
            this.renderTiltedTorch(block, (double)x + d2, (double)y + d3, z, d, 0.0);
        } else if (n == 3) {
            this.renderTiltedTorch(block, x, (double)y + d3, (double)z - d2, 0.0, -d);
        } else if (n == 4) {
            this.renderTiltedTorch(block, x, (double)y + d3, (double)z + d2, 0.0, d);
        } else {
            this.renderTiltedTorch(block, x, y, z, 0.0, 0.0);
        }
        return true;
    }

    private boolean renderRepeater(Block block, int x, int y, int z) {
        int n = this.blockView.getBlockMeta(x, y, z);
        int n2 = n & 3;
        int n3 = (n & 0xC) >> 2;
        this.renderBlock(block, x, y, z);
        Tessellator tessellator = Tessellator.INSTANCE;
        float f = block.getLuminance(this.blockView, x, y, z);
        if (Block.BLOCKS_LIGHT_LUMINANCE[block.id] > 0) {
            f = (f + 1.0f) * 0.5f;
        }
        tessellator.color(f, f, f);
        double d = -0.1875;
        double d2 = 0.0;
        double d3 = 0.0;
        double d4 = 0.0;
        double d5 = 0.0;
        switch (n2) {
            case 0: {
                d5 = -0.3125;
                d3 = RepeaterBlock.RENDER_OFFSET[n3];
                break;
            }
            case 2: {
                d5 = 0.3125;
                d3 = -RepeaterBlock.RENDER_OFFSET[n3];
                break;
            }
            case 3: {
                d4 = -0.3125;
                d2 = RepeaterBlock.RENDER_OFFSET[n3];
                break;
            }
            case 1: {
                d4 = 0.3125;
                d2 = -RepeaterBlock.RENDER_OFFSET[n3];
            }
        }
        this.renderTiltedTorch(block, (double)x + d2, (double)y + d, (double)z + d3, 0.0, 0.0);
        this.renderTiltedTorch(block, (double)x + d4, (double)y + d, (double)z + d5, 0.0, 0.0);
        int n4 = block.getTexture(1);
        int n5 = (n4 & 0xF) << 4;
        int n6 = n4 & 0xF0;
        double d6 = (float)n5 / 256.0f;
        double d7 = ((float)n5 + 15.99f) / 256.0f;
        double d8 = (float)n6 / 256.0f;
        double d9 = ((float)n6 + 15.99f) / 256.0f;
        float f2 = 0.125f;
        float f3 = x + 1;
        float f4 = x + 1;
        float f5 = x + 0;
        float f6 = x + 0;
        float f7 = z + 0;
        float f8 = z + 1;
        float f9 = z + 1;
        float f10 = z + 0;
        float f11 = (float)y + f2;
        if (n2 == 2) {
            f3 = f4 = (float)(x + 0);
            f5 = f6 = (float)(x + 1);
            f7 = f10 = (float)(z + 1);
            f8 = f9 = (float)(z + 0);
        } else if (n2 == 3) {
            f3 = f6 = (float)(x + 0);
            f4 = f5 = (float)(x + 1);
            f7 = f8 = (float)(z + 0);
            f9 = f10 = (float)(z + 1);
        } else if (n2 == 1) {
            f3 = f6 = (float)(x + 1);
            f4 = f5 = (float)(x + 0);
            f7 = f8 = (float)(z + 1);
            f9 = f10 = (float)(z + 0);
        }
        tessellator.vertex(f6, f11, f10, d6, d8);
        tessellator.vertex(f5, f11, f9, d6, d9);
        tessellator.vertex(f4, f11, f8, d7, d9);
        tessellator.vertex(f3, f11, f7, d7, d8);
        return true;
    }

    public void renderExtendedPiston(Block block, int x, int y, int z) {
        this.skipFaceCulling = true;
        this.renderPiston(block, x, y, z, true);
        this.skipFaceCulling = false;
    }

    private boolean renderPiston(Block block, int x, int y, int z, boolean extended) {
        int n = this.blockView.getBlockMeta(x, y, z);
        boolean bl = extended || (n & 8) != 0;
        int n2 = PistonBlock.getFacing(n);
        if (bl) {
            switch (n2) {
                case 0: {
                    this.eastFaceRotation = 3;
                    this.westFaceRotation = 3;
                    this.southFaceRotation = 3;
                    this.northFaceRotation = 3;
                    block.setBoundingBox(0.0f, 0.25f, 0.0f, 1.0f, 1.0f, 1.0f);
                    break;
                }
                case 1: {
                    block.setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 0.75f, 1.0f);
                    break;
                }
                case 2: {
                    this.southFaceRotation = 1;
                    this.northFaceRotation = 2;
                    block.setBoundingBox(0.0f, 0.0f, 0.25f, 1.0f, 1.0f, 1.0f);
                    break;
                }
                case 3: {
                    this.southFaceRotation = 2;
                    this.northFaceRotation = 1;
                    this.topFaceRotation = 3;
                    this.bottomFaceRotation = 3;
                    block.setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.75f);
                    break;
                }
                case 4: {
                    this.eastFaceRotation = 1;
                    this.westFaceRotation = 2;
                    this.topFaceRotation = 2;
                    this.bottomFaceRotation = 1;
                    block.setBoundingBox(0.25f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
                    break;
                }
                case 5: {
                    this.eastFaceRotation = 2;
                    this.westFaceRotation = 1;
                    this.topFaceRotation = 1;
                    this.bottomFaceRotation = 2;
                    block.setBoundingBox(0.0f, 0.0f, 0.0f, 0.75f, 1.0f, 1.0f);
                }
            }
            this.renderBlock(block, x, y, z);
            this.eastFaceRotation = 0;
            this.westFaceRotation = 0;
            this.southFaceRotation = 0;
            this.northFaceRotation = 0;
            this.topFaceRotation = 0;
            this.bottomFaceRotation = 0;
            block.setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
        } else {
            switch (n2) {
                case 0: {
                    this.eastFaceRotation = 3;
                    this.westFaceRotation = 3;
                    this.southFaceRotation = 3;
                    this.northFaceRotation = 3;
                    break;
                }
                case 1: {
                    break;
                }
                case 2: {
                    this.southFaceRotation = 1;
                    this.northFaceRotation = 2;
                    break;
                }
                case 3: {
                    this.southFaceRotation = 2;
                    this.northFaceRotation = 1;
                    this.topFaceRotation = 3;
                    this.bottomFaceRotation = 3;
                    break;
                }
                case 4: {
                    this.eastFaceRotation = 1;
                    this.westFaceRotation = 2;
                    this.topFaceRotation = 2;
                    this.bottomFaceRotation = 1;
                    break;
                }
                case 5: {
                    this.eastFaceRotation = 2;
                    this.westFaceRotation = 1;
                    this.topFaceRotation = 1;
                    this.bottomFaceRotation = 2;
                }
            }
            this.renderBlock(block, x, y, z);
            this.eastFaceRotation = 0;
            this.westFaceRotation = 0;
            this.southFaceRotation = 0;
            this.northFaceRotation = 0;
            this.topFaceRotation = 0;
            this.bottomFaceRotation = 0;
        }
        return true;
    }

    private void renderPistonHeadYAxis(double x1, double x2, double y1, double y2, double z1, double z2, float brightness, double shiftU) {
        int n = 108;
        if (this.textureOverride >= 0) {
            n = this.textureOverride;
        }
        int n2 = (n & 0xF) << 4;
        int n3 = n & 0xF0;
        Tessellator tessellator = Tessellator.INSTANCE;
        double d = (float)(n2 + 0) / 256.0f;
        double d2 = (float)(n3 + 0) / 256.0f;
        double d3 = ((double)n2 + shiftU - 0.01) / 256.0;
        double d4 = ((double)((float)n3 + 4.0f) - 0.01) / 256.0;
        tessellator.color(brightness, brightness, brightness);
        tessellator.vertex(x1, y2, z1, d3, d2);
        tessellator.vertex(x1, y1, z1, d, d2);
        tessellator.vertex(x2, y1, z2, d, d4);
        tessellator.vertex(x2, y2, z2, d3, d4);
    }

    private void renderPistonHeadZAxis(double x1, double x2, double y1, double y2, double z1, double z2, float brightness, double shiftU) {
        int n = 108;
        if (this.textureOverride >= 0) {
            n = this.textureOverride;
        }
        int n2 = (n & 0xF) << 4;
        int n3 = n & 0xF0;
        Tessellator tessellator = Tessellator.INSTANCE;
        double d = (float)(n2 + 0) / 256.0f;
        double d2 = (float)(n3 + 0) / 256.0f;
        double d3 = ((double)n2 + shiftU - 0.01) / 256.0;
        double d4 = ((double)((float)n3 + 4.0f) - 0.01) / 256.0;
        tessellator.color(brightness, brightness, brightness);
        tessellator.vertex(x1, y1, z2, d3, d2);
        tessellator.vertex(x1, y1, z1, d, d2);
        tessellator.vertex(x2, y2, z1, d, d4);
        tessellator.vertex(x2, y2, z2, d3, d4);
    }

    private void renderPistonHeadXAxis(double x1, double x2, double y1, double y2, double z1, double z2, float brightness, double shiftU) {
        int n = 108;
        if (this.textureOverride >= 0) {
            n = this.textureOverride;
        }
        int n2 = (n & 0xF) << 4;
        int n3 = n & 0xF0;
        Tessellator tessellator = Tessellator.INSTANCE;
        double d = (float)(n2 + 0) / 256.0f;
        double d2 = (float)(n3 + 0) / 256.0f;
        double d3 = ((double)n2 + shiftU - 0.01) / 256.0;
        double d4 = ((double)((float)n3 + 4.0f) - 0.01) / 256.0;
        tessellator.color(brightness, brightness, brightness);
        tessellator.vertex(x2, y1, z1, d3, d2);
        tessellator.vertex(x1, y1, z1, d, d2);
        tessellator.vertex(x1, y2, z2, d, d4);
        tessellator.vertex(x2, y2, z2, d3, d4);
    }

    public void renderPistonHeadWithoutCulling(Block block, int x, int y, int z, boolean extendedHalfway) {
        this.skipFaceCulling = true;
        this.renderPistonHead(block, x, y, z, extendedHalfway);
        this.skipFaceCulling = false;
    }

    private boolean renderPistonHead(Block block, int x, int y, int z, boolean extendedHalfway) {
        int n = this.blockView.getBlockMeta(x, y, z);
        int n2 = PistonHeadBlock.getFacing(n);
        float f = block.getLuminance(this.blockView, x, y, z);
        float f2 = extendedHalfway ? 1.0f : 0.5f;
        double d = extendedHalfway ? 16.0 : 8.0;
        switch (n2) {
            case 0: {
                this.eastFaceRotation = 3;
                this.westFaceRotation = 3;
                this.southFaceRotation = 3;
                this.northFaceRotation = 3;
                block.setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 0.25f, 1.0f);
                this.renderBlock(block, x, y, z);
                this.renderPistonHeadYAxis((float)x + 0.375f, (float)x + 0.625f, (float)y + 0.25f, (float)y + 0.25f + f2, (float)z + 0.625f, (float)z + 0.625f, f * 0.8f, d);
                this.renderPistonHeadYAxis((float)x + 0.625f, (float)x + 0.375f, (float)y + 0.25f, (float)y + 0.25f + f2, (float)z + 0.375f, (float)z + 0.375f, f * 0.8f, d);
                this.renderPistonHeadYAxis((float)x + 0.375f, (float)x + 0.375f, (float)y + 0.25f, (float)y + 0.25f + f2, (float)z + 0.375f, (float)z + 0.625f, f * 0.6f, d);
                this.renderPistonHeadYAxis((float)x + 0.625f, (float)x + 0.625f, (float)y + 0.25f, (float)y + 0.25f + f2, (float)z + 0.625f, (float)z + 0.375f, f * 0.6f, d);
                break;
            }
            case 1: {
                block.setBoundingBox(0.0f, 0.75f, 0.0f, 1.0f, 1.0f, 1.0f);
                this.renderBlock(block, x, y, z);
                this.renderPistonHeadYAxis((float)x + 0.375f, (float)x + 0.625f, (float)y - 0.25f + 1.0f - f2, (float)y - 0.25f + 1.0f, (float)z + 0.625f, (float)z + 0.625f, f * 0.8f, d);
                this.renderPistonHeadYAxis((float)x + 0.625f, (float)x + 0.375f, (float)y - 0.25f + 1.0f - f2, (float)y - 0.25f + 1.0f, (float)z + 0.375f, (float)z + 0.375f, f * 0.8f, d);
                this.renderPistonHeadYAxis((float)x + 0.375f, (float)x + 0.375f, (float)y - 0.25f + 1.0f - f2, (float)y - 0.25f + 1.0f, (float)z + 0.375f, (float)z + 0.625f, f * 0.6f, d);
                this.renderPistonHeadYAxis((float)x + 0.625f, (float)x + 0.625f, (float)y - 0.25f + 1.0f - f2, (float)y - 0.25f + 1.0f, (float)z + 0.625f, (float)z + 0.375f, f * 0.6f, d);
                break;
            }
            case 2: {
                this.southFaceRotation = 1;
                this.northFaceRotation = 2;
                block.setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.25f);
                this.renderBlock(block, x, y, z);
                this.renderPistonHeadZAxis((float)x + 0.375f, (float)x + 0.375f, (float)y + 0.625f, (float)y + 0.375f, (float)z + 0.25f, (float)z + 0.25f + f2, f * 0.6f, d);
                this.renderPistonHeadZAxis((float)x + 0.625f, (float)x + 0.625f, (float)y + 0.375f, (float)y + 0.625f, (float)z + 0.25f, (float)z + 0.25f + f2, f * 0.6f, d);
                this.renderPistonHeadZAxis((float)x + 0.375f, (float)x + 0.625f, (float)y + 0.375f, (float)y + 0.375f, (float)z + 0.25f, (float)z + 0.25f + f2, f * 0.5f, d);
                this.renderPistonHeadZAxis((float)x + 0.625f, (float)x + 0.375f, (float)y + 0.625f, (float)y + 0.625f, (float)z + 0.25f, (float)z + 0.25f + f2, f, d);
                break;
            }
            case 3: {
                this.southFaceRotation = 2;
                this.northFaceRotation = 1;
                this.topFaceRotation = 3;
                this.bottomFaceRotation = 3;
                block.setBoundingBox(0.0f, 0.0f, 0.75f, 1.0f, 1.0f, 1.0f);
                this.renderBlock(block, x, y, z);
                this.renderPistonHeadZAxis((float)x + 0.375f, (float)x + 0.375f, (float)y + 0.625f, (float)y + 0.375f, (float)z - 0.25f + 1.0f - f2, (float)z - 0.25f + 1.0f, f * 0.6f, d);
                this.renderPistonHeadZAxis((float)x + 0.625f, (float)x + 0.625f, (float)y + 0.375f, (float)y + 0.625f, (float)z - 0.25f + 1.0f - f2, (float)z - 0.25f + 1.0f, f * 0.6f, d);
                this.renderPistonHeadZAxis((float)x + 0.375f, (float)x + 0.625f, (float)y + 0.375f, (float)y + 0.375f, (float)z - 0.25f + 1.0f - f2, (float)z - 0.25f + 1.0f, f * 0.5f, d);
                this.renderPistonHeadZAxis((float)x + 0.625f, (float)x + 0.375f, (float)y + 0.625f, (float)y + 0.625f, (float)z - 0.25f + 1.0f - f2, (float)z - 0.25f + 1.0f, f, d);
                break;
            }
            case 4: {
                this.eastFaceRotation = 1;
                this.westFaceRotation = 2;
                this.topFaceRotation = 2;
                this.bottomFaceRotation = 1;
                block.setBoundingBox(0.0f, 0.0f, 0.0f, 0.25f, 1.0f, 1.0f);
                this.renderBlock(block, x, y, z);
                this.renderPistonHeadXAxis((float)x + 0.25f, (float)x + 0.25f + f2, (float)y + 0.375f, (float)y + 0.375f, (float)z + 0.625f, (float)z + 0.375f, f * 0.5f, d);
                this.renderPistonHeadXAxis((float)x + 0.25f, (float)x + 0.25f + f2, (float)y + 0.625f, (float)y + 0.625f, (float)z + 0.375f, (float)z + 0.625f, f, d);
                this.renderPistonHeadXAxis((float)x + 0.25f, (float)x + 0.25f + f2, (float)y + 0.375f, (float)y + 0.625f, (float)z + 0.375f, (float)z + 0.375f, f * 0.6f, d);
                this.renderPistonHeadXAxis((float)x + 0.25f, (float)x + 0.25f + f2, (float)y + 0.625f, (float)y + 0.375f, (float)z + 0.625f, (float)z + 0.625f, f * 0.6f, d);
                break;
            }
            case 5: {
                this.eastFaceRotation = 2;
                this.westFaceRotation = 1;
                this.topFaceRotation = 1;
                this.bottomFaceRotation = 2;
                block.setBoundingBox(0.75f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
                this.renderBlock(block, x, y, z);
                this.renderPistonHeadXAxis((float)x - 0.25f + 1.0f - f2, (float)x - 0.25f + 1.0f, (float)y + 0.375f, (float)y + 0.375f, (float)z + 0.625f, (float)z + 0.375f, f * 0.5f, d);
                this.renderPistonHeadXAxis((float)x - 0.25f + 1.0f - f2, (float)x - 0.25f + 1.0f, (float)y + 0.625f, (float)y + 0.625f, (float)z + 0.375f, (float)z + 0.625f, f, d);
                this.renderPistonHeadXAxis((float)x - 0.25f + 1.0f - f2, (float)x - 0.25f + 1.0f, (float)y + 0.375f, (float)y + 0.625f, (float)z + 0.375f, (float)z + 0.375f, f * 0.6f, d);
                this.renderPistonHeadXAxis((float)x - 0.25f + 1.0f - f2, (float)x - 0.25f + 1.0f, (float)y + 0.625f, (float)y + 0.375f, (float)z + 0.625f, (float)z + 0.625f, f * 0.6f, d);
            }
        }
        this.eastFaceRotation = 0;
        this.westFaceRotation = 0;
        this.southFaceRotation = 0;
        this.northFaceRotation = 0;
        this.topFaceRotation = 0;
        this.bottomFaceRotation = 0;
        block.setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
        return true;
    }

    public boolean renderLever(Block block, int x, int y, int z) {
        boolean bl;
        int n = this.blockView.getBlockMeta(x, y, z);
        int n2 = n & 7;
        boolean bl2 = (n & 8) > 0;
        Tessellator tessellator = Tessellator.INSTANCE;
        boolean bl3 = bl = this.textureOverride >= 0;
        if (!bl) {
            this.textureOverride = Block.COBBLESTONE.textureId;
        }
        float f = 0.25f;
        float f2 = 0.1875f;
        float f3 = 0.1875f;
        if (n2 == 5) {
            block.setBoundingBox(0.5f - f2, 0.0f, 0.5f - f, 0.5f + f2, f3, 0.5f + f);
        } else if (n2 == 6) {
            block.setBoundingBox(0.5f - f, 0.0f, 0.5f - f2, 0.5f + f, f3, 0.5f + f2);
        } else if (n2 == 4) {
            block.setBoundingBox(0.5f - f2, 0.5f - f, 1.0f - f3, 0.5f + f2, 0.5f + f, 1.0f);
        } else if (n2 == 3) {
            block.setBoundingBox(0.5f - f2, 0.5f - f, 0.0f, 0.5f + f2, 0.5f + f, f3);
        } else if (n2 == 2) {
            block.setBoundingBox(1.0f - f3, 0.5f - f, 0.5f - f2, 1.0f, 0.5f + f, 0.5f + f2);
        } else if (n2 == 1) {
            block.setBoundingBox(0.0f, 0.5f - f, 0.5f - f2, f3, 0.5f + f, 0.5f + f2);
        }
        this.renderBlock(block, x, y, z);
        if (!bl) {
            this.textureOverride = -1;
        }
        float f4 = block.getLuminance(this.blockView, x, y, z);
        if (Block.BLOCKS_LIGHT_LUMINANCE[block.id] > 0) {
            f4 = 1.0f;
        }
        tessellator.color(f4, f4, f4);
        int n3 = block.getTexture(0);
        if (this.textureOverride >= 0) {
            n3 = this.textureOverride;
        }
        int n4 = (n3 & 0xF) << 4;
        int n5 = n3 & 0xF0;
        float f5 = (float)n4 / 256.0f;
        float f6 = ((float)n4 + 15.99f) / 256.0f;
        float f7 = (float)n5 / 256.0f;
        float f8 = ((float)n5 + 15.99f) / 256.0f;
        Vec3d[] vec3dArray = new Vec3d[8];
        float f9 = 0.0625f;
        float f10 = 0.0625f;
        float f11 = 0.625f;
        vec3dArray[0] = Vec3d.create(-f9, 0.0, -f10);
        vec3dArray[1] = Vec3d.create(f9, 0.0, -f10);
        vec3dArray[2] = Vec3d.create(f9, 0.0, f10);
        vec3dArray[3] = Vec3d.create(-f9, 0.0, f10);
        vec3dArray[4] = Vec3d.create(-f9, f11, -f10);
        vec3dArray[5] = Vec3d.create(f9, f11, -f10);
        vec3dArray[6] = Vec3d.create(f9, f11, f10);
        vec3dArray[7] = Vec3d.create(-f9, f11, f10);
        for (int i = 0; i < 8; ++i) {
            if (bl2) {
                vec3dArray[i].z -= 0.0625;
                vec3dArray[i].rotateX(0.69813174f);
            } else {
                vec3dArray[i].z += 0.0625;
                vec3dArray[i].rotateX(-0.69813174f);
            }
            if (n2 == 6) {
                vec3dArray[i].rotateY(1.5707964f);
            }
            if (n2 < 5) {
                vec3dArray[i].y -= 0.375;
                vec3dArray[i].rotateX(1.5707964f);
                if (n2 == 4) {
                    vec3dArray[i].rotateY(0.0f);
                }
                if (n2 == 3) {
                    vec3dArray[i].rotateY((float)Math.PI);
                }
                if (n2 == 2) {
                    vec3dArray[i].rotateY(1.5707964f);
                }
                if (n2 == 1) {
                    vec3dArray[i].rotateY(-1.5707964f);
                }
                vec3dArray[i].x += (double)x + 0.5;
                vec3dArray[i].y += (double)((float)y + 0.5f);
                vec3dArray[i].z += (double)z + 0.5;
                continue;
            }
            vec3dArray[i].x += (double)x + 0.5;
            vec3dArray[i].y += (double)((float)y + 0.125f);
            vec3dArray[i].z += (double)z + 0.5;
        }
        Vec3d vec3d = null;
        Vec3d vec3d2 = null;
        Vec3d vec3d3 = null;
        Vec3d vec3d4 = null;
        for (int i = 0; i < 6; ++i) {
            if (i == 0) {
                f5 = (float)(n4 + 7) / 256.0f;
                f6 = ((float)(n4 + 9) - 0.01f) / 256.0f;
                f7 = (float)(n5 + 6) / 256.0f;
                f8 = ((float)(n5 + 8) - 0.01f) / 256.0f;
            } else if (i == 2) {
                f5 = (float)(n4 + 7) / 256.0f;
                f6 = ((float)(n4 + 9) - 0.01f) / 256.0f;
                f7 = (float)(n5 + 6) / 256.0f;
                f8 = ((float)(n5 + 16) - 0.01f) / 256.0f;
            }
            if (i == 0) {
                vec3d = vec3dArray[0];
                vec3d2 = vec3dArray[1];
                vec3d3 = vec3dArray[2];
                vec3d4 = vec3dArray[3];
            } else if (i == 1) {
                vec3d = vec3dArray[7];
                vec3d2 = vec3dArray[6];
                vec3d3 = vec3dArray[5];
                vec3d4 = vec3dArray[4];
            } else if (i == 2) {
                vec3d = vec3dArray[1];
                vec3d2 = vec3dArray[0];
                vec3d3 = vec3dArray[4];
                vec3d4 = vec3dArray[5];
            } else if (i == 3) {
                vec3d = vec3dArray[2];
                vec3d2 = vec3dArray[1];
                vec3d3 = vec3dArray[5];
                vec3d4 = vec3dArray[6];
            } else if (i == 4) {
                vec3d = vec3dArray[3];
                vec3d2 = vec3dArray[2];
                vec3d3 = vec3dArray[6];
                vec3d4 = vec3dArray[7];
            } else if (i == 5) {
                vec3d = vec3dArray[0];
                vec3d2 = vec3dArray[3];
                vec3d3 = vec3dArray[7];
                vec3d4 = vec3dArray[4];
            }
            tessellator.vertex(vec3d.x, vec3d.y, vec3d.z, f5, f8);
            tessellator.vertex(vec3d2.x, vec3d2.y, vec3d2.z, f6, f8);
            tessellator.vertex(vec3d3.x, vec3d3.y, vec3d3.z, f6, f7);
            tessellator.vertex(vec3d4.x, vec3d4.y, vec3d4.z, f5, f7);
        }
        return true;
    }

    public boolean renderFire(Block block, int x, int y, int z) {
        Tessellator tessellator = Tessellator.INSTANCE;
        int n = block.getTexture(0);
        if (this.textureOverride >= 0) {
            n = this.textureOverride;
        }
        float f = block.getLuminance(this.blockView, x, y, z);
        tessellator.color(f, f, f);
        int n2 = (n & 0xF) << 4;
        int n3 = n & 0xF0;
        double d = (float)n2 / 256.0f;
        double d2 = ((float)n2 + 15.99f) / 256.0f;
        double d3 = (float)n3 / 256.0f;
        double d4 = ((float)n3 + 15.99f) / 256.0f;
        float f2 = 1.4f;
        if (this.blockView.shouldSuffocate(x, y - 1, z) || Block.FIRE.isFlammable(this.blockView, x, y - 1, z)) {
            double d5 = (double)x + 0.5 + 0.2;
            double d6 = (double)x + 0.5 - 0.2;
            double d7 = (double)z + 0.5 + 0.2;
            double d8 = (double)z + 0.5 - 0.2;
            double d9 = (double)x + 0.5 - 0.3;
            double d10 = (double)x + 0.5 + 0.3;
            double d11 = (double)z + 0.5 - 0.3;
            double d12 = (double)z + 0.5 + 0.3;
            tessellator.vertex(d9, (float)y + f2, z + 1, d2, d3);
            tessellator.vertex(d5, y + 0, z + 1, d2, d4);
            tessellator.vertex(d5, y + 0, z + 0, d, d4);
            tessellator.vertex(d9, (float)y + f2, z + 0, d, d3);
            tessellator.vertex(d10, (float)y + f2, z + 0, d2, d3);
            tessellator.vertex(d6, y + 0, z + 0, d2, d4);
            tessellator.vertex(d6, y + 0, z + 1, d, d4);
            tessellator.vertex(d10, (float)y + f2, z + 1, d, d3);
            d = (float)n2 / 256.0f;
            d2 = ((float)n2 + 15.99f) / 256.0f;
            d3 = (float)(n3 + 16) / 256.0f;
            d4 = ((float)n3 + 15.99f + 16.0f) / 256.0f;
            tessellator.vertex(x + 1, (float)y + f2, d12, d2, d3);
            tessellator.vertex(x + 1, y + 0, d8, d2, d4);
            tessellator.vertex(x + 0, y + 0, d8, d, d4);
            tessellator.vertex(x + 0, (float)y + f2, d12, d, d3);
            tessellator.vertex(x + 0, (float)y + f2, d11, d2, d3);
            tessellator.vertex(x + 0, y + 0, d7, d2, d4);
            tessellator.vertex(x + 1, y + 0, d7, d, d4);
            tessellator.vertex(x + 1, (float)y + f2, d11, d, d3);
            d5 = (double)x + 0.5 - 0.5;
            d6 = (double)x + 0.5 + 0.5;
            d7 = (double)z + 0.5 - 0.5;
            d8 = (double)z + 0.5 + 0.5;
            d9 = (double)x + 0.5 - 0.4;
            d10 = (double)x + 0.5 + 0.4;
            d11 = (double)z + 0.5 - 0.4;
            d12 = (double)z + 0.5 + 0.4;
            tessellator.vertex(d9, (float)y + f2, z + 0, d, d3);
            tessellator.vertex(d5, y + 0, z + 0, d, d4);
            tessellator.vertex(d5, y + 0, z + 1, d2, d4);
            tessellator.vertex(d9, (float)y + f2, z + 1, d2, d3);
            tessellator.vertex(d10, (float)y + f2, z + 1, d, d3);
            tessellator.vertex(d6, y + 0, z + 1, d, d4);
            tessellator.vertex(d6, y + 0, z + 0, d2, d4);
            tessellator.vertex(d10, (float)y + f2, z + 0, d2, d3);
            d = (float)n2 / 256.0f;
            d2 = ((float)n2 + 15.99f) / 256.0f;
            d3 = (float)n3 / 256.0f;
            d4 = ((float)n3 + 15.99f) / 256.0f;
            tessellator.vertex(x + 0, (float)y + f2, d12, d, d3);
            tessellator.vertex(x + 0, y + 0, d8, d, d4);
            tessellator.vertex(x + 1, y + 0, d8, d2, d4);
            tessellator.vertex(x + 1, (float)y + f2, d12, d2, d3);
            tessellator.vertex(x + 1, (float)y + f2, d11, d, d3);
            tessellator.vertex(x + 1, y + 0, d7, d, d4);
            tessellator.vertex(x + 0, y + 0, d7, d2, d4);
            tessellator.vertex(x + 0, (float)y + f2, d11, d2, d3);
        } else {
            double d13;
            float f3 = 0.2f;
            float f4 = 0.0625f;
            if ((x + y + z & 1) == 1) {
                d = (float)n2 / 256.0f;
                d2 = ((float)n2 + 15.99f) / 256.0f;
                d3 = (float)(n3 + 16) / 256.0f;
                d4 = ((float)n3 + 15.99f + 16.0f) / 256.0f;
            }
            if ((x / 2 + y / 2 + z / 2 & 1) == 1) {
                d13 = d2;
                d2 = d;
                d = d13;
            }
            if (Block.FIRE.isFlammable(this.blockView, x - 1, y, z)) {
                tessellator.vertex((float)x + f3, (float)y + f2 + f4, z + 1, d2, d3);
                tessellator.vertex(x + 0, (float)(y + 0) + f4, z + 1, d2, d4);
                tessellator.vertex(x + 0, (float)(y + 0) + f4, z + 0, d, d4);
                tessellator.vertex((float)x + f3, (float)y + f2 + f4, z + 0, d, d3);
                tessellator.vertex((float)x + f3, (float)y + f2 + f4, z + 0, d, d3);
                tessellator.vertex(x + 0, (float)(y + 0) + f4, z + 0, d, d4);
                tessellator.vertex(x + 0, (float)(y + 0) + f4, z + 1, d2, d4);
                tessellator.vertex((float)x + f3, (float)y + f2 + f4, z + 1, d2, d3);
            }
            if (Block.FIRE.isFlammable(this.blockView, x + 1, y, z)) {
                tessellator.vertex((float)(x + 1) - f3, (float)y + f2 + f4, z + 0, d, d3);
                tessellator.vertex(x + 1 - 0, (float)(y + 0) + f4, z + 0, d, d4);
                tessellator.vertex(x + 1 - 0, (float)(y + 0) + f4, z + 1, d2, d4);
                tessellator.vertex((float)(x + 1) - f3, (float)y + f2 + f4, z + 1, d2, d3);
                tessellator.vertex((float)(x + 1) - f3, (float)y + f2 + f4, z + 1, d2, d3);
                tessellator.vertex(x + 1 - 0, (float)(y + 0) + f4, z + 1, d2, d4);
                tessellator.vertex(x + 1 - 0, (float)(y + 0) + f4, z + 0, d, d4);
                tessellator.vertex((float)(x + 1) - f3, (float)y + f2 + f4, z + 0, d, d3);
            }
            if (Block.FIRE.isFlammable(this.blockView, x, y, z - 1)) {
                tessellator.vertex(x + 0, (float)y + f2 + f4, (float)z + f3, d2, d3);
                tessellator.vertex(x + 0, (float)(y + 0) + f4, z + 0, d2, d4);
                tessellator.vertex(x + 1, (float)(y + 0) + f4, z + 0, d, d4);
                tessellator.vertex(x + 1, (float)y + f2 + f4, (float)z + f3, d, d3);
                tessellator.vertex(x + 1, (float)y + f2 + f4, (float)z + f3, d, d3);
                tessellator.vertex(x + 1, (float)(y + 0) + f4, z + 0, d, d4);
                tessellator.vertex(x + 0, (float)(y + 0) + f4, z + 0, d2, d4);
                tessellator.vertex(x + 0, (float)y + f2 + f4, (float)z + f3, d2, d3);
            }
            if (Block.FIRE.isFlammable(this.blockView, x, y, z + 1)) {
                tessellator.vertex(x + 1, (float)y + f2 + f4, (float)(z + 1) - f3, d, d3);
                tessellator.vertex(x + 1, (float)(y + 0) + f4, z + 1 - 0, d, d4);
                tessellator.vertex(x + 0, (float)(y + 0) + f4, z + 1 - 0, d2, d4);
                tessellator.vertex(x + 0, (float)y + f2 + f4, (float)(z + 1) - f3, d2, d3);
                tessellator.vertex(x + 0, (float)y + f2 + f4, (float)(z + 1) - f3, d2, d3);
                tessellator.vertex(x + 0, (float)(y + 0) + f4, z + 1 - 0, d2, d4);
                tessellator.vertex(x + 1, (float)(y + 0) + f4, z + 1 - 0, d, d4);
                tessellator.vertex(x + 1, (float)y + f2 + f4, (float)(z + 1) - f3, d, d3);
            }
            if (Block.FIRE.isFlammable(this.blockView, x, y + 1, z)) {
                d13 = (double)x + 0.5 + 0.5;
                double d14 = (double)x + 0.5 - 0.5;
                double d15 = (double)z + 0.5 + 0.5;
                double d16 = (double)z + 0.5 - 0.5;
                double d17 = (double)x + 0.5 - 0.5;
                double d18 = (double)x + 0.5 + 0.5;
                double d19 = (double)z + 0.5 - 0.5;
                double d20 = (double)z + 0.5 + 0.5;
                d = (float)n2 / 256.0f;
                d2 = ((float)n2 + 15.99f) / 256.0f;
                d3 = (float)n3 / 256.0f;
                d4 = ((float)n3 + 15.99f) / 256.0f;
                f2 = -0.2f;
                if ((x + ++y + z & 1) == 0) {
                    tessellator.vertex(d17, (float)y + f2, z + 0, d2, d3);
                    tessellator.vertex(d13, y + 0, z + 0, d2, d4);
                    tessellator.vertex(d13, y + 0, z + 1, d, d4);
                    tessellator.vertex(d17, (float)y + f2, z + 1, d, d3);
                    d = (float)n2 / 256.0f;
                    d2 = ((float)n2 + 15.99f) / 256.0f;
                    d3 = (float)(n3 + 16) / 256.0f;
                    d4 = ((float)n3 + 15.99f + 16.0f) / 256.0f;
                    tessellator.vertex(d18, (float)y + f2, z + 1, d2, d3);
                    tessellator.vertex(d14, y + 0, z + 1, d2, d4);
                    tessellator.vertex(d14, y + 0, z + 0, d, d4);
                    tessellator.vertex(d18, (float)y + f2, z + 0, d, d3);
                } else {
                    tessellator.vertex(x + 0, (float)y + f2, d20, d2, d3);
                    tessellator.vertex(x + 0, y + 0, d16, d2, d4);
                    tessellator.vertex(x + 1, y + 0, d16, d, d4);
                    tessellator.vertex(x + 1, (float)y + f2, d20, d, d3);
                    d = (float)n2 / 256.0f;
                    d2 = ((float)n2 + 15.99f) / 256.0f;
                    d3 = (float)(n3 + 16) / 256.0f;
                    d4 = ((float)n3 + 15.99f + 16.0f) / 256.0f;
                    tessellator.vertex(x + 1, (float)y + f2, d19, d2, d3);
                    tessellator.vertex(x + 1, y + 0, d15, d2, d4);
                    tessellator.vertex(x + 0, y + 0, d15, d, d4);
                    tessellator.vertex(x + 0, (float)y + f2, d19, d, d3);
                }
            }
        }
        return true;
    }

    public boolean renderRedstoneDust(Block block, int x, int y, int z) {
        boolean bl;
        Tessellator tessellator = Tessellator.INSTANCE;
        int n = this.blockView.getBlockMeta(x, y, z);
        int n2 = block.getTexture(1, n);
        if (this.textureOverride >= 0) {
            n2 = this.textureOverride;
        }
        float f = block.getLuminance(this.blockView, x, y, z);
        float f2 = (float)n / 15.0f;
        float f3 = f2 * 0.6f + 0.4f;
        if (n == 0) {
            f3 = 0.3f;
        }
        float f4 = f2 * f2 * 0.7f - 0.5f;
        float f5 = f2 * f2 * 0.6f - 0.7f;
        if (f4 < 0.0f) {
            f4 = 0.0f;
        }
        if (f5 < 0.0f) {
            f5 = 0.0f;
        }
        tessellator.color(f * f3, f * f4, f * f5);
        int n3 = (n2 & 0xF) << 4;
        int n4 = n2 & 0xF0;
        double d = (float)n3 / 256.0f;
        double d2 = ((float)n3 + 15.99f) / 256.0f;
        double d3 = (float)n4 / 256.0f;
        double d4 = ((float)n4 + 15.99f) / 256.0f;
        boolean bl2 = RedstoneWireBlock.shouldConnectTo(this.blockView, x - 1, y, z, 1) || !this.blockView.shouldSuffocate(x - 1, y, z) && RedstoneWireBlock.shouldConnectTo(this.blockView, x - 1, y - 1, z, -1);
        boolean bl3 = RedstoneWireBlock.shouldConnectTo(this.blockView, x + 1, y, z, 3) || !this.blockView.shouldSuffocate(x + 1, y, z) && RedstoneWireBlock.shouldConnectTo(this.blockView, x + 1, y - 1, z, -1);
        boolean bl4 = RedstoneWireBlock.shouldConnectTo(this.blockView, x, y, z - 1, 2) || !this.blockView.shouldSuffocate(x, y, z - 1) && RedstoneWireBlock.shouldConnectTo(this.blockView, x, y - 1, z - 1, -1);
        boolean bl5 = bl = RedstoneWireBlock.shouldConnectTo(this.blockView, x, y, z + 1, 0) || !this.blockView.shouldSuffocate(x, y, z + 1) && RedstoneWireBlock.shouldConnectTo(this.blockView, x, y - 1, z + 1, -1);
        if (!this.blockView.shouldSuffocate(x, y + 1, z)) {
            if (this.blockView.shouldSuffocate(x - 1, y, z) && RedstoneWireBlock.shouldConnectTo(this.blockView, x - 1, y + 1, z, -1)) {
                bl2 = true;
            }
            if (this.blockView.shouldSuffocate(x + 1, y, z) && RedstoneWireBlock.shouldConnectTo(this.blockView, x + 1, y + 1, z, -1)) {
                bl3 = true;
            }
            if (this.blockView.shouldSuffocate(x, y, z - 1) && RedstoneWireBlock.shouldConnectTo(this.blockView, x, y + 1, z - 1, -1)) {
                bl4 = true;
            }
            if (this.blockView.shouldSuffocate(x, y, z + 1) && RedstoneWireBlock.shouldConnectTo(this.blockView, x, y + 1, z + 1, -1)) {
                bl = true;
            }
        }
        float f6 = x + 0;
        float f7 = x + 1;
        float f8 = z + 0;
        float f9 = z + 1;
        int n5 = 0;
        if ((bl2 || bl3) && !bl4 && !bl) {
            n5 = 1;
        }
        if ((bl4 || bl) && !bl3 && !bl2) {
            n5 = 2;
        }
        if (n5 != 0) {
            d = (float)(n3 + 16) / 256.0f;
            d2 = ((float)(n3 + 16) + 15.99f) / 256.0f;
            d3 = (float)n4 / 256.0f;
            d4 = ((float)n4 + 15.99f) / 256.0f;
        }
        if (n5 == 0) {
            if (bl3 || bl4 || bl || bl2) {
                if (!bl2) {
                    f6 += 0.3125f;
                }
                if (!bl2) {
                    d += 0.01953125;
                }
                if (!bl3) {
                    f7 -= 0.3125f;
                }
                if (!bl3) {
                    d2 -= 0.01953125;
                }
                if (!bl4) {
                    f8 += 0.3125f;
                }
                if (!bl4) {
                    d3 += 0.01953125;
                }
                if (!bl) {
                    f9 -= 0.3125f;
                }
                if (!bl) {
                    d4 -= 0.01953125;
                }
            }
            tessellator.vertex(f7, (float)y + 0.015625f, f9, d2, d4);
            tessellator.vertex(f7, (float)y + 0.015625f, f8, d2, d3);
            tessellator.vertex(f6, (float)y + 0.015625f, f8, d, d3);
            tessellator.vertex(f6, (float)y + 0.015625f, f9, d, d4);
            tessellator.color(f, f, f);
            tessellator.vertex(f7, (float)y + 0.015625f, f9, d2, d4 + 0.0625);
            tessellator.vertex(f7, (float)y + 0.015625f, f8, d2, d3 + 0.0625);
            tessellator.vertex(f6, (float)y + 0.015625f, f8, d, d3 + 0.0625);
            tessellator.vertex(f6, (float)y + 0.015625f, f9, d, d4 + 0.0625);
        } else if (n5 == 1) {
            tessellator.vertex(f7, (float)y + 0.015625f, f9, d2, d4);
            tessellator.vertex(f7, (float)y + 0.015625f, f8, d2, d3);
            tessellator.vertex(f6, (float)y + 0.015625f, f8, d, d3);
            tessellator.vertex(f6, (float)y + 0.015625f, f9, d, d4);
            tessellator.color(f, f, f);
            tessellator.vertex(f7, (float)y + 0.015625f, f9, d2, d4 + 0.0625);
            tessellator.vertex(f7, (float)y + 0.015625f, f8, d2, d3 + 0.0625);
            tessellator.vertex(f6, (float)y + 0.015625f, f8, d, d3 + 0.0625);
            tessellator.vertex(f6, (float)y + 0.015625f, f9, d, d4 + 0.0625);
        } else if (n5 == 2) {
            tessellator.vertex(f7, (float)y + 0.015625f, f9, d2, d4);
            tessellator.vertex(f7, (float)y + 0.015625f, f8, d, d4);
            tessellator.vertex(f6, (float)y + 0.015625f, f8, d, d3);
            tessellator.vertex(f6, (float)y + 0.015625f, f9, d2, d3);
            tessellator.color(f, f, f);
            tessellator.vertex(f7, (float)y + 0.015625f, f9, d2, d4 + 0.0625);
            tessellator.vertex(f7, (float)y + 0.015625f, f8, d, d4 + 0.0625);
            tessellator.vertex(f6, (float)y + 0.015625f, f8, d, d3 + 0.0625);
            tessellator.vertex(f6, (float)y + 0.015625f, f9, d2, d3 + 0.0625);
        }
        if (!this.blockView.shouldSuffocate(x, y + 1, z)) {
            d = (float)(n3 + 16) / 256.0f;
            d2 = ((float)(n3 + 16) + 15.99f) / 256.0f;
            d3 = (float)n4 / 256.0f;
            d4 = ((float)n4 + 15.99f) / 256.0f;
            if (this.blockView.shouldSuffocate(x - 1, y, z) && this.blockView.getBlockId(x - 1, y + 1, z) == Block.REDSTONE_WIRE.id) {
                tessellator.color(f * f3, f * f4, f * f5);
                tessellator.vertex((float)x + 0.015625f, (float)(y + 1) + 0.021875f, z + 1, d2, d3);
                tessellator.vertex((float)x + 0.015625f, y + 0, z + 1, d, d3);
                tessellator.vertex((float)x + 0.015625f, y + 0, z + 0, d, d4);
                tessellator.vertex((float)x + 0.015625f, (float)(y + 1) + 0.021875f, z + 0, d2, d4);
                tessellator.color(f, f, f);
                tessellator.vertex((float)x + 0.015625f, (float)(y + 1) + 0.021875f, z + 1, d2, d3 + 0.0625);
                tessellator.vertex((float)x + 0.015625f, y + 0, z + 1, d, d3 + 0.0625);
                tessellator.vertex((float)x + 0.015625f, y + 0, z + 0, d, d4 + 0.0625);
                tessellator.vertex((float)x + 0.015625f, (float)(y + 1) + 0.021875f, z + 0, d2, d4 + 0.0625);
            }
            if (this.blockView.shouldSuffocate(x + 1, y, z) && this.blockView.getBlockId(x + 1, y + 1, z) == Block.REDSTONE_WIRE.id) {
                tessellator.color(f * f3, f * f4, f * f5);
                tessellator.vertex((float)(x + 1) - 0.015625f, y + 0, z + 1, d, d4);
                tessellator.vertex((float)(x + 1) - 0.015625f, (float)(y + 1) + 0.021875f, z + 1, d2, d4);
                tessellator.vertex((float)(x + 1) - 0.015625f, (float)(y + 1) + 0.021875f, z + 0, d2, d3);
                tessellator.vertex((float)(x + 1) - 0.015625f, y + 0, z + 0, d, d3);
                tessellator.color(f, f, f);
                tessellator.vertex((float)(x + 1) - 0.015625f, y + 0, z + 1, d, d4 + 0.0625);
                tessellator.vertex((float)(x + 1) - 0.015625f, (float)(y + 1) + 0.021875f, z + 1, d2, d4 + 0.0625);
                tessellator.vertex((float)(x + 1) - 0.015625f, (float)(y + 1) + 0.021875f, z + 0, d2, d3 + 0.0625);
                tessellator.vertex((float)(x + 1) - 0.015625f, y + 0, z + 0, d, d3 + 0.0625);
            }
            if (this.blockView.shouldSuffocate(x, y, z - 1) && this.blockView.getBlockId(x, y + 1, z - 1) == Block.REDSTONE_WIRE.id) {
                tessellator.color(f * f3, f * f4, f * f5);
                tessellator.vertex(x + 1, y + 0, (float)z + 0.015625f, d, d4);
                tessellator.vertex(x + 1, (float)(y + 1) + 0.021875f, (float)z + 0.015625f, d2, d4);
                tessellator.vertex(x + 0, (float)(y + 1) + 0.021875f, (float)z + 0.015625f, d2, d3);
                tessellator.vertex(x + 0, y + 0, (float)z + 0.015625f, d, d3);
                tessellator.color(f, f, f);
                tessellator.vertex(x + 1, y + 0, (float)z + 0.015625f, d, d4 + 0.0625);
                tessellator.vertex(x + 1, (float)(y + 1) + 0.021875f, (float)z + 0.015625f, d2, d4 + 0.0625);
                tessellator.vertex(x + 0, (float)(y + 1) + 0.021875f, (float)z + 0.015625f, d2, d3 + 0.0625);
                tessellator.vertex(x + 0, y + 0, (float)z + 0.015625f, d, d3 + 0.0625);
            }
            if (this.blockView.shouldSuffocate(x, y, z + 1) && this.blockView.getBlockId(x, y + 1, z + 1) == Block.REDSTONE_WIRE.id) {
                tessellator.color(f * f3, f * f4, f * f5);
                tessellator.vertex(x + 1, (float)(y + 1) + 0.021875f, (float)(z + 1) - 0.015625f, d2, d3);
                tessellator.vertex(x + 1, y + 0, (float)(z + 1) - 0.015625f, d, d3);
                tessellator.vertex(x + 0, y + 0, (float)(z + 1) - 0.015625f, d, d4);
                tessellator.vertex(x + 0, (float)(y + 1) + 0.021875f, (float)(z + 1) - 0.015625f, d2, d4);
                tessellator.color(f, f, f);
                tessellator.vertex(x + 1, (float)(y + 1) + 0.021875f, (float)(z + 1) - 0.015625f, d2, d3 + 0.0625);
                tessellator.vertex(x + 1, y + 0, (float)(z + 1) - 0.015625f, d, d3 + 0.0625);
                tessellator.vertex(x + 0, y + 0, (float)(z + 1) - 0.015625f, d, d4 + 0.0625);
                tessellator.vertex(x + 0, (float)(y + 1) + 0.021875f, (float)(z + 1) - 0.015625f, d2, d4 + 0.0625);
            }
        }
        return true;
    }

    public boolean renderRail(RailBlock rail, int x, int y, int z) {
        Tessellator tessellator = Tessellator.INSTANCE;
        int n = this.blockView.getBlockMeta(x, y, z);
        int n2 = rail.getTexture(0, n);
        if (this.textureOverride >= 0) {
            n2 = this.textureOverride;
        }
        if (rail.isAlwaysStraight()) {
            n &= 7;
        }
        float f = rail.getLuminance(this.blockView, x, y, z);
        tessellator.color(f, f, f);
        int n3 = (n2 & 0xF) << 4;
        int n4 = n2 & 0xF0;
        double d = (float)n3 / 256.0f;
        double d2 = ((float)n3 + 15.99f) / 256.0f;
        double d3 = (float)n4 / 256.0f;
        double d4 = ((float)n4 + 15.99f) / 256.0f;
        float f2 = 0.0625f;
        float f3 = x + 1;
        float f4 = x + 1;
        float f5 = x + 0;
        float f6 = x + 0;
        float f7 = z + 0;
        float f8 = z + 1;
        float f9 = z + 1;
        float f10 = z + 0;
        float f11 = (float)y + f2;
        float f12 = (float)y + f2;
        float f13 = (float)y + f2;
        float f14 = (float)y + f2;
        if (n == 1 || n == 2 || n == 3 || n == 7) {
            f3 = f6 = (float)(x + 1);
            f4 = f5 = (float)(x + 0);
            f7 = f8 = (float)(z + 1);
            f9 = f10 = (float)(z + 0);
        } else if (n == 8) {
            f3 = f4 = (float)(x + 0);
            f5 = f6 = (float)(x + 1);
            f7 = f10 = (float)(z + 1);
            f8 = f9 = (float)(z + 0);
        } else if (n == 9) {
            f3 = f6 = (float)(x + 0);
            f4 = f5 = (float)(x + 1);
            f7 = f8 = (float)(z + 0);
            f9 = f10 = (float)(z + 1);
        }
        if (n == 2 || n == 4) {
            f11 += 1.0f;
            f14 += 1.0f;
        } else if (n == 3 || n == 5) {
            f12 += 1.0f;
            f13 += 1.0f;
        }
        tessellator.vertex(f3, f11, f7, d2, d3);
        tessellator.vertex(f4, f12, f8, d2, d4);
        tessellator.vertex(f5, f13, f9, d, d4);
        tessellator.vertex(f6, f14, f10, d, d3);
        tessellator.vertex(f6, f14, f10, d, d3);
        tessellator.vertex(f5, f13, f9, d, d4);
        tessellator.vertex(f4, f12, f8, d2, d4);
        tessellator.vertex(f3, f11, f7, d2, d3);
        return true;
    }

    public boolean renderLadder(Block block, int x, int y, int z) {
        Tessellator tessellator = Tessellator.INSTANCE;
        int n = block.getTexture(0);
        if (this.textureOverride >= 0) {
            n = this.textureOverride;
        }
        float f = block.getLuminance(this.blockView, x, y, z);
        tessellator.color(f, f, f);
        int n2 = (n & 0xF) << 4;
        int n3 = n & 0xF0;
        double d = (float)n2 / 256.0f;
        double d2 = ((float)n2 + 15.99f) / 256.0f;
        double d3 = (float)n3 / 256.0f;
        double d4 = ((float)n3 + 15.99f) / 256.0f;
        int n4 = this.blockView.getBlockMeta(x, y, z);
        float f2 = 0.0f;
        float f3 = 0.05f;
        if (n4 == 5) {
            tessellator.vertex((float)x + f3, (float)(y + 1) + f2, (float)(z + 1) + f2, d, d3);
            tessellator.vertex((float)x + f3, (float)(y + 0) - f2, (float)(z + 1) + f2, d, d4);
            tessellator.vertex((float)x + f3, (float)(y + 0) - f2, (float)(z + 0) - f2, d2, d4);
            tessellator.vertex((float)x + f3, (float)(y + 1) + f2, (float)(z + 0) - f2, d2, d3);
        }
        if (n4 == 4) {
            tessellator.vertex((float)(x + 1) - f3, (float)(y + 0) - f2, (float)(z + 1) + f2, d2, d4);
            tessellator.vertex((float)(x + 1) - f3, (float)(y + 1) + f2, (float)(z + 1) + f2, d2, d3);
            tessellator.vertex((float)(x + 1) - f3, (float)(y + 1) + f2, (float)(z + 0) - f2, d, d3);
            tessellator.vertex((float)(x + 1) - f3, (float)(y + 0) - f2, (float)(z + 0) - f2, d, d4);
        }
        if (n4 == 3) {
            tessellator.vertex((float)(x + 1) + f2, (float)(y + 0) - f2, (float)z + f3, d2, d4);
            tessellator.vertex((float)(x + 1) + f2, (float)(y + 1) + f2, (float)z + f3, d2, d3);
            tessellator.vertex((float)(x + 0) - f2, (float)(y + 1) + f2, (float)z + f3, d, d3);
            tessellator.vertex((float)(x + 0) - f2, (float)(y + 0) - f2, (float)z + f3, d, d4);
        }
        if (n4 == 2) {
            tessellator.vertex((float)(x + 1) + f2, (float)(y + 1) + f2, (float)(z + 1) - f3, d, d3);
            tessellator.vertex((float)(x + 1) + f2, (float)(y + 0) - f2, (float)(z + 1) - f3, d, d4);
            tessellator.vertex((float)(x + 0) - f2, (float)(y + 0) - f2, (float)(z + 1) - f3, d2, d4);
            tessellator.vertex((float)(x + 0) - f2, (float)(y + 1) + f2, (float)(z + 1) - f3, d2, d3);
        }
        return true;
    }

    public boolean renderCross(Block block, int x, int y, int z) {
        Tessellator tessellator = Tessellator.INSTANCE;
        float f = block.getLuminance(this.blockView, x, y, z);
        int n = block.getColorMultiplier(this.blockView, x, y, z);
        float f2 = (float)(n >> 16 & 0xFF) / 255.0f;
        float f3 = (float)(n >> 8 & 0xFF) / 255.0f;
        float f4 = (float)(n & 0xFF) / 255.0f;
        if (GameRenderer.anaglyph3d) {
            float f5 = (f2 * 30.0f + f3 * 59.0f + f4 * 11.0f) / 100.0f;
            float f6 = (f2 * 30.0f + f3 * 70.0f) / 100.0f;
            float f7 = (f2 * 30.0f + f4 * 70.0f) / 100.0f;
            f2 = f5;
            f3 = f6;
            f4 = f7;
        }
        tessellator.color(f * f2, f * f3, f * f4);
        double d = x;
        double d2 = y;
        double d3 = z;
        if (block == Block.GRASS) {
            long l = (long)(x * 3129871) ^ (long)z * 116129781L ^ (long)y;
            l = l * l * 42317861L + l * 11L;
            d += ((double)((float)(l >> 16 & 0xFL) / 15.0f) - 0.5) * 0.5;
            d2 += ((double)((float)(l >> 20 & 0xFL) / 15.0f) - 1.0) * 0.2;
            d3 += ((double)((float)(l >> 24 & 0xFL) / 15.0f) - 0.5) * 0.5;
        }
        this.renderCross(block, this.blockView.getBlockMeta(x, y, z), d, d2, d3);
        return true;
    }

    public boolean renderCrop(Block block, int x, int y, int z) {
        Tessellator tessellator = Tessellator.INSTANCE;
        float f = block.getLuminance(this.blockView, x, y, z);
        tessellator.color(f, f, f);
        this.renderCrop(block, this.blockView.getBlockMeta(x, y, z), x, (float)y - 0.0625f, z);
        return true;
    }

    public void renderTiltedTorch(Block block, double x, double y, double z, double xTilt, double zTilt) {
        Tessellator tessellator = Tessellator.INSTANCE;
        int n = block.getTexture(0);
        if (this.textureOverride >= 0) {
            n = this.textureOverride;
        }
        int n2 = (n & 0xF) << 4;
        int n3 = n & 0xF0;
        float f = (float)n2 / 256.0f;
        float f2 = ((float)n2 + 15.99f) / 256.0f;
        float f3 = (float)n3 / 256.0f;
        float f4 = ((float)n3 + 15.99f) / 256.0f;
        double d = (double)f + 0.02734375;
        double d2 = (double)f3 + 0.0234375;
        double d3 = (double)f + 0.03515625;
        double d4 = (double)f3 + 0.03125;
        double d5 = (x += 0.5) - 0.5;
        double d6 = x + 0.5;
        double d7 = (z += 0.5) - 0.5;
        double d8 = z + 0.5;
        double d9 = 0.0625;
        double d10 = 0.625;
        tessellator.vertex(x + xTilt * (1.0 - d10) - d9, y + d10, z + zTilt * (1.0 - d10) - d9, d, d2);
        tessellator.vertex(x + xTilt * (1.0 - d10) - d9, y + d10, z + zTilt * (1.0 - d10) + d9, d, d4);
        tessellator.vertex(x + xTilt * (1.0 - d10) + d9, y + d10, z + zTilt * (1.0 - d10) + d9, d3, d4);
        tessellator.vertex(x + xTilt * (1.0 - d10) + d9, y + d10, z + zTilt * (1.0 - d10) - d9, d3, d2);
        tessellator.vertex(x - d9, y + 1.0, d7, f, f3);
        tessellator.vertex(x - d9 + xTilt, y + 0.0, d7 + zTilt, f, f4);
        tessellator.vertex(x - d9 + xTilt, y + 0.0, d8 + zTilt, f2, f4);
        tessellator.vertex(x - d9, y + 1.0, d8, f2, f3);
        tessellator.vertex(x + d9, y + 1.0, d8, f, f3);
        tessellator.vertex(x + xTilt + d9, y + 0.0, d8 + zTilt, f, f4);
        tessellator.vertex(x + xTilt + d9, y + 0.0, d7 + zTilt, f2, f4);
        tessellator.vertex(x + d9, y + 1.0, d7, f2, f3);
        tessellator.vertex(d5, y + 1.0, z + d9, f, f3);
        tessellator.vertex(d5 + xTilt, y + 0.0, z + d9 + zTilt, f, f4);
        tessellator.vertex(d6 + xTilt, y + 0.0, z + d9 + zTilt, f2, f4);
        tessellator.vertex(d6, y + 1.0, z + d9, f2, f3);
        tessellator.vertex(d6, y + 1.0, z - d9, f, f3);
        tessellator.vertex(d6 + xTilt, y + 0.0, z - d9 + zTilt, f, f4);
        tessellator.vertex(d5 + xTilt, y + 0.0, z - d9 + zTilt, f2, f4);
        tessellator.vertex(d5, y + 1.0, z - d9, f2, f3);
    }

    public void renderCross(Block block, int metadata, double x, double y, double z) {
        Tessellator tessellator = Tessellator.INSTANCE;
        int n = block.getTexture(0, metadata);
        if (this.textureOverride >= 0) {
            n = this.textureOverride;
        }
        int n2 = (n & 0xF) << 4;
        int n3 = n & 0xF0;
        double d = (float)n2 / 256.0f;
        double d2 = ((float)n2 + 15.99f) / 256.0f;
        double d3 = (float)n3 / 256.0f;
        double d4 = ((float)n3 + 15.99f) / 256.0f;
        double d5 = x + 0.5 - (double)0.45f;
        double d6 = x + 0.5 + (double)0.45f;
        double d7 = z + 0.5 - (double)0.45f;
        double d8 = z + 0.5 + (double)0.45f;
        tessellator.vertex(d5, y + 1.0, d7, d, d3);
        tessellator.vertex(d5, y + 0.0, d7, d, d4);
        tessellator.vertex(d6, y + 0.0, d8, d2, d4);
        tessellator.vertex(d6, y + 1.0, d8, d2, d3);
        tessellator.vertex(d6, y + 1.0, d8, d, d3);
        tessellator.vertex(d6, y + 0.0, d8, d, d4);
        tessellator.vertex(d5, y + 0.0, d7, d2, d4);
        tessellator.vertex(d5, y + 1.0, d7, d2, d3);
        tessellator.vertex(d5, y + 1.0, d8, d, d3);
        tessellator.vertex(d5, y + 0.0, d8, d, d4);
        tessellator.vertex(d6, y + 0.0, d7, d2, d4);
        tessellator.vertex(d6, y + 1.0, d7, d2, d3);
        tessellator.vertex(d6, y + 1.0, d7, d, d3);
        tessellator.vertex(d6, y + 0.0, d7, d, d4);
        tessellator.vertex(d5, y + 0.0, d8, d2, d4);
        tessellator.vertex(d5, y + 1.0, d8, d2, d3);
    }

    public void renderCrop(Block block, int metadata, double x, double y, double z) {
        Tessellator tessellator = Tessellator.INSTANCE;
        int n = block.getTexture(0, metadata);
        if (this.textureOverride >= 0) {
            n = this.textureOverride;
        }
        int n2 = (n & 0xF) << 4;
        int n3 = n & 0xF0;
        double d = (float)n2 / 256.0f;
        double d2 = ((float)n2 + 15.99f) / 256.0f;
        double d3 = (float)n3 / 256.0f;
        double d4 = ((float)n3 + 15.99f) / 256.0f;
        double d5 = x + 0.5 - 0.25;
        double d6 = x + 0.5 + 0.25;
        double d7 = z + 0.5 - 0.5;
        double d8 = z + 0.5 + 0.5;
        tessellator.vertex(d5, y + 1.0, d7, d, d3);
        tessellator.vertex(d5, y + 0.0, d7, d, d4);
        tessellator.vertex(d5, y + 0.0, d8, d2, d4);
        tessellator.vertex(d5, y + 1.0, d8, d2, d3);
        tessellator.vertex(d5, y + 1.0, d8, d, d3);
        tessellator.vertex(d5, y + 0.0, d8, d, d4);
        tessellator.vertex(d5, y + 0.0, d7, d2, d4);
        tessellator.vertex(d5, y + 1.0, d7, d2, d3);
        tessellator.vertex(d6, y + 1.0, d8, d, d3);
        tessellator.vertex(d6, y + 0.0, d8, d, d4);
        tessellator.vertex(d6, y + 0.0, d7, d2, d4);
        tessellator.vertex(d6, y + 1.0, d7, d2, d3);
        tessellator.vertex(d6, y + 1.0, d7, d, d3);
        tessellator.vertex(d6, y + 0.0, d7, d, d4);
        tessellator.vertex(d6, y + 0.0, d8, d2, d4);
        tessellator.vertex(d6, y + 1.0, d8, d2, d3);
        d5 = x + 0.5 - 0.5;
        d6 = x + 0.5 + 0.5;
        d7 = z + 0.5 - 0.25;
        d8 = z + 0.5 + 0.25;
        tessellator.vertex(d5, y + 1.0, d7, d, d3);
        tessellator.vertex(d5, y + 0.0, d7, d, d4);
        tessellator.vertex(d6, y + 0.0, d7, d2, d4);
        tessellator.vertex(d6, y + 1.0, d7, d2, d3);
        tessellator.vertex(d6, y + 1.0, d7, d, d3);
        tessellator.vertex(d6, y + 0.0, d7, d, d4);
        tessellator.vertex(d5, y + 0.0, d7, d2, d4);
        tessellator.vertex(d5, y + 1.0, d7, d2, d3);
        tessellator.vertex(d6, y + 1.0, d8, d, d3);
        tessellator.vertex(d6, y + 0.0, d8, d, d4);
        tessellator.vertex(d5, y + 0.0, d8, d2, d4);
        tessellator.vertex(d5, y + 1.0, d8, d2, d3);
        tessellator.vertex(d5, y + 1.0, d8, d, d3);
        tessellator.vertex(d5, y + 0.0, d8, d, d4);
        tessellator.vertex(d6, y + 0.0, d8, d2, d4);
        tessellator.vertex(d6, y + 1.0, d8, d2, d3);
    }

    public boolean renderFluid(Block block, int x, int y, int z) {
        float f;
        float f2;
        float f3;
        int n;
        int n2;
        Tessellator tessellator = Tessellator.INSTANCE;
        int n3 = block.getColorMultiplier(this.blockView, x, y, z);
        float f4 = (float)(n3 >> 16 & 0xFF) / 255.0f;
        float f5 = (float)(n3 >> 8 & 0xFF) / 255.0f;
        float f6 = (float)(n3 & 0xFF) / 255.0f;
        boolean bl = block.isSideVisible(this.blockView, x, y + 1, z, 1);
        boolean bl2 = block.isSideVisible(this.blockView, x, y - 1, z, 0);
        boolean[] blArray = new boolean[]{block.isSideVisible(this.blockView, x, y, z - 1, 2), block.isSideVisible(this.blockView, x, y, z + 1, 3), block.isSideVisible(this.blockView, x - 1, y, z, 4), block.isSideVisible(this.blockView, x + 1, y, z, 5)};
        if (!(bl || bl2 || blArray[0] || blArray[1] || blArray[2] || blArray[3])) {
            return false;
        }
        boolean bl3 = false;
        float f7 = 0.5f;
        float f8 = 1.0f;
        float f9 = 0.8f;
        float f10 = 0.6f;
        double d = 0.0;
        double d2 = 1.0;
        Material material = block.material;
        int n4 = this.blockView.getBlockMeta(x, y, z);
        float f11 = this.getFluidHeight(x, y, z, material);
        float f12 = this.getFluidHeight(x, y, z + 1, material);
        float f13 = this.getFluidHeight(x + 1, y, z + 1, material);
        float f14 = this.getFluidHeight(x + 1, y, z, material);
        if (this.skipFaceCulling || bl) {
            bl3 = true;
            int n5 = block.getTexture(1, n4);
            float f15 = (float)LiquidBlock.getFlowingAngle(this.blockView, x, y, z, material);
            if (f15 > -999.0f) {
                n5 = block.getTexture(2, n4);
            }
            n2 = (n5 & 0xF) << 4;
            n = n5 & 0xF0;
            double d3 = ((double)n2 + 8.0) / 256.0;
            double d4 = ((double)n + 8.0) / 256.0;
            if (f15 < -999.0f) {
                f15 = 0.0f;
            } else {
                d3 = (float)(n2 + 16) / 256.0f;
                d4 = (float)(n + 16) / 256.0f;
            }
            f3 = MathHelper.sin(f15) * 8.0f / 256.0f;
            f2 = MathHelper.cos(f15) * 8.0f / 256.0f;
            f = block.getLuminance(this.blockView, x, y, z);
            tessellator.color(f8 * f * f4, f8 * f * f5, f8 * f * f6);
            tessellator.vertex(x + 0, (float)y + f11, z + 0, d3 - (double)f2 - (double)f3, d4 - (double)f2 + (double)f3);
            tessellator.vertex(x + 0, (float)y + f12, z + 1, d3 - (double)f2 + (double)f3, d4 + (double)f2 + (double)f3);
            tessellator.vertex(x + 1, (float)y + f13, z + 1, d3 + (double)f2 + (double)f3, d4 + (double)f2 - (double)f3);
            tessellator.vertex(x + 1, (float)y + f14, z + 0, d3 + (double)f2 - (double)f3, d4 - (double)f2 - (double)f3);
        }
        if (this.skipFaceCulling || bl2) {
            float f16 = block.getLuminance(this.blockView, x, y - 1, z);
            tessellator.color(f7 * f16, f7 * f16, f7 * f16);
            this.renderBottomFace(block, x, y, z, block.getTexture(0));
            bl3 = true;
        }
        for (int i = 0; i < 4; ++i) {
            float f17;
            float f18;
            float f19;
            int n6 = x;
            n2 = y;
            n = z;
            if (i == 0) {
                --n;
            }
            if (i == 1) {
                ++n;
            }
            if (i == 2) {
                --n6;
            }
            if (i == 3) {
                ++n6;
            }
            int n7 = block.getTexture(i + 2, n4);
            int n8 = (n7 & 0xF) << 4;
            int n9 = n7 & 0xF0;
            if (!this.skipFaceCulling && !blArray[i]) continue;
            if (i == 0) {
                f19 = f11;
                f3 = f14;
                f2 = x;
                f18 = x + 1;
                f = z;
                f17 = z;
            } else if (i == 1) {
                f19 = f13;
                f3 = f12;
                f2 = x + 1;
                f18 = x;
                f = z + 1;
                f17 = z + 1;
            } else if (i == 2) {
                f19 = f12;
                f3 = f11;
                f2 = x;
                f18 = x;
                f = z + 1;
                f17 = z;
            } else {
                f19 = f14;
                f3 = f13;
                f2 = x + 1;
                f18 = x + 1;
                f = z;
                f17 = z + 1;
            }
            bl3 = true;
            double d5 = (float)(n8 + 0) / 256.0f;
            double d6 = ((double)(n8 + 16) - 0.01) / 256.0;
            double d7 = ((float)n9 + (1.0f - f19) * 16.0f) / 256.0f;
            double d8 = ((float)n9 + (1.0f - f3) * 16.0f) / 256.0f;
            double d9 = ((double)(n9 + 16) - 0.01) / 256.0;
            float f20 = block.getLuminance(this.blockView, n6, n2, n);
            f20 = i < 2 ? (f20 *= f9) : (f20 *= f10);
            tessellator.color(f8 * f20 * f4, f8 * f20 * f5, f8 * f20 * f6);
            tessellator.vertex(f2, (float)y + f19, f, d5, d7);
            tessellator.vertex(f18, (float)y + f3, f17, d6, d8);
            tessellator.vertex(f18, y + 0, f17, d6, d9);
            tessellator.vertex(f2, y + 0, f, d5, d9);
        }
        block.minY = d;
        block.maxY = d2;
        return bl3;
    }

    private float getFluidHeight(int x, int y, int z, Material material) {
        int n = 0;
        float f = 0.0f;
        for (int i = 0; i < 4; ++i) {
            int n2 = x - (i & 1);
            int n3 = y;
            int n4 = z - (i >> 1 & 1);
            if (this.blockView.getMaterial(n2, n3 + 1, n4) == material) {
                return 1.0f;
            }
            Material material2 = this.blockView.getMaterial(n2, n3, n4);
            if (material2 == material) {
                int n5 = this.blockView.getBlockMeta(n2, n3, n4);
                if (n5 >= 8 || n5 == 0) {
                    f += LiquidBlock.getFluidHeightFromMeta(n5) * 10.0f;
                    n += 10;
                }
                f += LiquidBlock.getFluidHeightFromMeta(n5);
                ++n;
                continue;
            }
            if (material2.isSolid()) continue;
            f += 1.0f;
            ++n;
        }
        return 1.0f - f / (float)n;
    }

    public void renderFallingBlockEntity(Block block, World world, int x, int y, int z) {
        float f = 0.5f;
        float f2 = 1.0f;
        float f3 = 0.8f;
        float f4 = 0.6f;
        Tessellator tessellator = Tessellator.INSTANCE;
        tessellator.startQuads();
        float f5 = block.getLuminance(world, x, y, z);
        float f6 = block.getLuminance(world, x, y - 1, z);
        if (f6 < f5) {
            f6 = f5;
        }
        tessellator.color(f * f6, f * f6, f * f6);
        this.renderBottomFace(block, -0.5, -0.5, -0.5, block.getTexture(0));
        f6 = block.getLuminance(world, x, y + 1, z);
        if (f6 < f5) {
            f6 = f5;
        }
        tessellator.color(f2 * f6, f2 * f6, f2 * f6);
        this.renderTopFace(block, -0.5, -0.5, -0.5, block.getTexture(1));
        f6 = block.getLuminance(world, x, y, z - 1);
        if (f6 < f5) {
            f6 = f5;
        }
        tessellator.color(f3 * f6, f3 * f6, f3 * f6);
        this.renderEastFace(block, -0.5, -0.5, -0.5, block.getTexture(2));
        f6 = block.getLuminance(world, x, y, z + 1);
        if (f6 < f5) {
            f6 = f5;
        }
        tessellator.color(f3 * f6, f3 * f6, f3 * f6);
        this.renderWestFace(block, -0.5, -0.5, -0.5, block.getTexture(3));
        f6 = block.getLuminance(world, x - 1, y, z);
        if (f6 < f5) {
            f6 = f5;
        }
        tessellator.color(f4 * f6, f4 * f6, f4 * f6);
        this.renderNorthFace(block, -0.5, -0.5, -0.5, block.getTexture(4));
        f6 = block.getLuminance(world, x + 1, y, z);
        if (f6 < f5) {
            f6 = f5;
        }
        tessellator.color(f4 * f6, f4 * f6, f4 * f6);
        this.renderSouthFace(block, -0.5, -0.5, -0.5, block.getTexture(5));
        tessellator.draw();
    }

    public boolean renderBlock(Block block, int x, int y, int z) {
        int n = block.getColorMultiplier(this.blockView, x, y, z);
        float f = (float)(n >> 16 & 0xFF) / 255.0f;
        float f2 = (float)(n >> 8 & 0xFF) / 255.0f;
        float f3 = (float)(n & 0xFF) / 255.0f;
        if (GameRenderer.anaglyph3d) {
            float f4 = (f * 30.0f + f2 * 59.0f + f3 * 11.0f) / 100.0f;
            float f5 = (f * 30.0f + f2 * 70.0f) / 100.0f;
            float f6 = (f * 30.0f + f3 * 70.0f) / 100.0f;
            f = f4;
            f2 = f5;
            f3 = f6;
        }
        if (Minecraft.isAmbientOcclusionEnabled()) {
            return this.renderSmooth(block, x, y, z, f, f2, f3);
        }
        return this.renderFlat(block, x, y, z, f, f2, f3);
    }

    public boolean renderSmooth(Block block, int x, int y, int z, float red, float green, float blue) {
        int n;
        this.useAo = true;
        boolean bl = false;
        float f = this.selfBrightness;
        float f2 = this.selfBrightness;
        float f3 = this.selfBrightness;
        float f4 = this.selfBrightness;
        boolean bl2 = true;
        boolean bl3 = true;
        boolean bl4 = true;
        boolean bl5 = true;
        boolean bl6 = true;
        boolean bl7 = true;
        this.selfBrightness = block.getLuminance(this.blockView, x, y, z);
        this.northBrightness = block.getLuminance(this.blockView, x - 1, y, z);
        this.bottomBrightness = block.getLuminance(this.blockView, x, y - 1, z);
        this.eastBrightness = block.getLuminance(this.blockView, x, y, z - 1);
        this.southBrightness = block.getLuminance(this.blockView, x + 1, y, z);
        this.topBrightness = block.getLuminance(this.blockView, x, y + 1, z);
        this.westBrightness = block.getLuminance(this.blockView, x, y, z + 1);
        this.topEastEdgeTranslucent = Block.BLOCKS_ALLOW_VISION[this.blockView.getBlockId(x + 1, y + 1, z)];
        this.bottomEastEdgeTranslucent = Block.BLOCKS_ALLOW_VISION[this.blockView.getBlockId(x + 1, y - 1, z)];
        this.southEastEdgeTranslucent = Block.BLOCKS_ALLOW_VISION[this.blockView.getBlockId(x + 1, y, z + 1)];
        this.northEastEdgeTranslucent = Block.BLOCKS_ALLOW_VISION[this.blockView.getBlockId(x + 1, y, z - 1)];
        this.topWestEdgeTranslucent = Block.BLOCKS_ALLOW_VISION[this.blockView.getBlockId(x - 1, y + 1, z)];
        this.bottomWestEdgeTranslucent = Block.BLOCKS_ALLOW_VISION[this.blockView.getBlockId(x - 1, y - 1, z)];
        this.northWestEdgeTranslucent = Block.BLOCKS_ALLOW_VISION[this.blockView.getBlockId(x - 1, y, z - 1)];
        this.southWestEdgeTranslucent = Block.BLOCKS_ALLOW_VISION[this.blockView.getBlockId(x - 1, y, z + 1)];
        this.topSouthEdgeTranslucent = Block.BLOCKS_ALLOW_VISION[this.blockView.getBlockId(x, y + 1, z + 1)];
        this.topNorthEdgeTranslucent = Block.BLOCKS_ALLOW_VISION[this.blockView.getBlockId(x, y + 1, z - 1)];
        this.bottomSouthEdgeTranslucent = Block.BLOCKS_ALLOW_VISION[this.blockView.getBlockId(x, y - 1, z + 1)];
        this.bottomNorthEdgeTranslucent = Block.BLOCKS_ALLOW_VISION[this.blockView.getBlockId(x, y - 1, z - 1)];
        if (block.textureId == 3) {
            bl7 = false;
            bl6 = false;
            bl5 = false;
            bl4 = false;
            bl2 = false;
        }
        if (this.textureOverride >= 0) {
            bl7 = false;
            bl6 = false;
            bl5 = false;
            bl4 = false;
            bl2 = false;
        }
        if (this.skipFaceCulling || block.isSideVisible(this.blockView, x, y - 1, z, 0)) {
            if (this.useSurroundingBrightness > 0) {
                this.northBottomBrightness = block.getLuminance(this.blockView, x - 1, --y, z);
                this.eastBottomBrightness = block.getLuminance(this.blockView, x, y, z - 1);
                this.westBottomBrightness = block.getLuminance(this.blockView, x, y, z + 1);
                this.southBottomBrightness = block.getLuminance(this.blockView, x + 1, y, z);
                this.northEastBottomBrightness = this.bottomNorthEdgeTranslucent || this.bottomWestEdgeTranslucent ? block.getLuminance(this.blockView, x - 1, y, z - 1) : this.northBottomBrightness;
                this.northWestBottomBrightness = this.bottomSouthEdgeTranslucent || this.bottomWestEdgeTranslucent ? block.getLuminance(this.blockView, x - 1, y, z + 1) : this.northBottomBrightness;
                this.southEastBottomBrightness = this.bottomNorthEdgeTranslucent || this.bottomEastEdgeTranslucent ? block.getLuminance(this.blockView, x + 1, y, z - 1) : this.southBottomBrightness;
                this.southWestBottomBrightness = this.bottomSouthEdgeTranslucent || this.bottomEastEdgeTranslucent ? block.getLuminance(this.blockView, x + 1, y, z + 1) : this.southBottomBrightness;
                ++y;
                f = (this.northWestBottomBrightness + this.northBottomBrightness + this.westBottomBrightness + this.bottomBrightness) / 4.0f;
                f4 = (this.westBottomBrightness + this.bottomBrightness + this.southWestBottomBrightness + this.southBottomBrightness) / 4.0f;
                f3 = (this.bottomBrightness + this.eastBottomBrightness + this.southBottomBrightness + this.southEastBottomBrightness) / 4.0f;
                f2 = (this.northBottomBrightness + this.northEastBottomBrightness + this.bottomBrightness + this.eastBottomBrightness) / 4.0f;
            } else {
                f3 = f4 = this.bottomBrightness;
                f2 = f4;
                f = f4;
            }
            this.thirdVertexRed = this.fourthVertexRed = (bl2 ? red : 1.0f) * 0.5f;
            this.secondVertexRed = this.fourthVertexRed;
            this.firstVertexRed = this.fourthVertexRed;
            this.thirdVertexGreen = this.fourthVertexGreen = (bl2 ? green : 1.0f) * 0.5f;
            this.secondVertexGreen = this.fourthVertexGreen;
            this.firstVertexGreen = this.fourthVertexGreen;
            this.thirdVertexBlue = this.fourthVertexBlue = (bl2 ? blue : 1.0f) * 0.5f;
            this.secondVertexBlue = this.fourthVertexBlue;
            this.firstVertexBlue = this.fourthVertexBlue;
            this.firstVertexRed *= f;
            this.firstVertexGreen *= f;
            this.firstVertexBlue *= f;
            this.secondVertexRed *= f2;
            this.secondVertexGreen *= f2;
            this.secondVertexBlue *= f2;
            this.thirdVertexRed *= f3;
            this.thirdVertexGreen *= f3;
            this.thirdVertexBlue *= f3;
            this.fourthVertexRed *= f4;
            this.fourthVertexGreen *= f4;
            this.fourthVertexBlue *= f4;
            this.renderBottomFace(block, x, y, z, block.getTextureId(this.blockView, x, y, z, 0));
            bl = true;
        }
        if (this.skipFaceCulling || block.isSideVisible(this.blockView, x, y + 1, z, 1)) {
            if (this.useSurroundingBrightness > 0) {
                this.northTopBrightness = block.getLuminance(this.blockView, x - 1, ++y, z);
                this.southTopBrightness = block.getLuminance(this.blockView, x + 1, y, z);
                this.eastTopBrightness = block.getLuminance(this.blockView, x, y, z - 1);
                this.westTopBrightness = block.getLuminance(this.blockView, x, y, z + 1);
                this.northEastTopBrightness = this.topNorthEdgeTranslucent || this.topWestEdgeTranslucent ? block.getLuminance(this.blockView, x - 1, y, z - 1) : this.northTopBrightness;
                this.southEastTopBrightness = this.topNorthEdgeTranslucent || this.topEastEdgeTranslucent ? block.getLuminance(this.blockView, x + 1, y, z - 1) : this.southTopBrightness;
                this.northWestTopBrightness = this.topSouthEdgeTranslucent || this.topWestEdgeTranslucent ? block.getLuminance(this.blockView, x - 1, y, z + 1) : this.northTopBrightness;
                this.southWestTopBrightness = this.topSouthEdgeTranslucent || this.topEastEdgeTranslucent ? block.getLuminance(this.blockView, x + 1, y, z + 1) : this.southTopBrightness;
                --y;
                f4 = (this.northWestTopBrightness + this.northTopBrightness + this.westTopBrightness + this.topBrightness) / 4.0f;
                f = (this.westTopBrightness + this.topBrightness + this.southWestTopBrightness + this.southTopBrightness) / 4.0f;
                f2 = (this.topBrightness + this.eastTopBrightness + this.southTopBrightness + this.southEastTopBrightness) / 4.0f;
                f3 = (this.northTopBrightness + this.northEastTopBrightness + this.topBrightness + this.eastTopBrightness) / 4.0f;
            } else {
                f3 = f4 = this.topBrightness;
                f2 = f4;
                f = f4;
            }
            this.fourthVertexRed = bl3 ? red : 1.0f;
            this.thirdVertexRed = this.fourthVertexRed;
            this.secondVertexRed = this.fourthVertexRed;
            this.firstVertexRed = this.fourthVertexRed;
            this.fourthVertexGreen = bl3 ? green : 1.0f;
            this.thirdVertexGreen = this.fourthVertexGreen;
            this.secondVertexGreen = this.fourthVertexGreen;
            this.firstVertexGreen = this.fourthVertexGreen;
            this.fourthVertexBlue = bl3 ? blue : 1.0f;
            this.thirdVertexBlue = this.fourthVertexBlue;
            this.secondVertexBlue = this.fourthVertexBlue;
            this.firstVertexBlue = this.fourthVertexBlue;
            this.firstVertexRed *= f;
            this.firstVertexGreen *= f;
            this.firstVertexBlue *= f;
            this.secondVertexRed *= f2;
            this.secondVertexGreen *= f2;
            this.secondVertexBlue *= f2;
            this.thirdVertexRed *= f3;
            this.thirdVertexGreen *= f3;
            this.thirdVertexBlue *= f3;
            this.fourthVertexRed *= f4;
            this.fourthVertexGreen *= f4;
            this.fourthVertexBlue *= f4;
            this.renderTopFace(block, x, y, z, block.getTextureId(this.blockView, x, y, z, 1));
            bl = true;
        }
        if (this.skipFaceCulling || block.isSideVisible(this.blockView, x, y, z - 1, 2)) {
            if (this.useSurroundingBrightness > 0) {
                this.northEastBrightness = block.getLuminance(this.blockView, x - 1, y, --z);
                this.eastBottomBrightness = block.getLuminance(this.blockView, x, y - 1, z);
                this.eastTopBrightness = block.getLuminance(this.blockView, x, y + 1, z);
                this.southEastBrightness = block.getLuminance(this.blockView, x + 1, y, z);
                this.northEastBottomBrightness = this.northWestEdgeTranslucent || this.bottomNorthEdgeTranslucent ? block.getLuminance(this.blockView, x - 1, y - 1, z) : this.northEastBrightness;
                this.northEastTopBrightness = this.northWestEdgeTranslucent || this.topNorthEdgeTranslucent ? block.getLuminance(this.blockView, x - 1, y + 1, z) : this.northEastBrightness;
                this.southEastBottomBrightness = this.northEastEdgeTranslucent || this.bottomNorthEdgeTranslucent ? block.getLuminance(this.blockView, x + 1, y - 1, z) : this.southEastBrightness;
                this.southEastTopBrightness = this.northEastEdgeTranslucent || this.topNorthEdgeTranslucent ? block.getLuminance(this.blockView, x + 1, y + 1, z) : this.southEastBrightness;
                ++z;
                f = (this.northEastBrightness + this.northEastTopBrightness + this.eastBrightness + this.eastTopBrightness) / 4.0f;
                f2 = (this.eastBrightness + this.eastTopBrightness + this.southEastBrightness + this.southEastTopBrightness) / 4.0f;
                f3 = (this.eastBottomBrightness + this.eastBrightness + this.southEastBottomBrightness + this.southEastBrightness) / 4.0f;
                f4 = (this.northEastBottomBrightness + this.northEastBrightness + this.eastBottomBrightness + this.eastBrightness) / 4.0f;
            } else {
                f3 = f4 = this.eastBrightness;
                f2 = f4;
                f = f4;
            }
            this.thirdVertexRed = this.fourthVertexRed = (bl4 ? red : 1.0f) * 0.8f;
            this.secondVertexRed = this.fourthVertexRed;
            this.firstVertexRed = this.fourthVertexRed;
            this.thirdVertexGreen = this.fourthVertexGreen = (bl4 ? green : 1.0f) * 0.8f;
            this.secondVertexGreen = this.fourthVertexGreen;
            this.firstVertexGreen = this.fourthVertexGreen;
            this.thirdVertexBlue = this.fourthVertexBlue = (bl4 ? blue : 1.0f) * 0.8f;
            this.secondVertexBlue = this.fourthVertexBlue;
            this.firstVertexBlue = this.fourthVertexBlue;
            this.firstVertexRed *= f;
            this.firstVertexGreen *= f;
            this.firstVertexBlue *= f;
            this.secondVertexRed *= f2;
            this.secondVertexGreen *= f2;
            this.secondVertexBlue *= f2;
            this.thirdVertexRed *= f3;
            this.thirdVertexGreen *= f3;
            this.thirdVertexBlue *= f3;
            this.fourthVertexRed *= f4;
            this.fourthVertexGreen *= f4;
            this.fourthVertexBlue *= f4;
            n = block.getTextureId(this.blockView, x, y, z, 2);
            this.renderEastFace(block, x, y, z, n);
            if (fancyGraphics && n == 3 && this.textureOverride < 0) {
                this.firstVertexRed *= red;
                this.secondVertexRed *= red;
                this.thirdVertexRed *= red;
                this.fourthVertexRed *= red;
                this.firstVertexGreen *= green;
                this.secondVertexGreen *= green;
                this.thirdVertexGreen *= green;
                this.fourthVertexGreen *= green;
                this.firstVertexBlue *= blue;
                this.secondVertexBlue *= blue;
                this.thirdVertexBlue *= blue;
                this.fourthVertexBlue *= blue;
                this.renderEastFace(block, x, y, z, 38);
            }
            bl = true;
        }
        if (this.skipFaceCulling || block.isSideVisible(this.blockView, x, y, z + 1, 3)) {
            if (this.useSurroundingBrightness > 0) {
                this.northWestBrightness = block.getLuminance(this.blockView, x - 1, y, ++z);
                this.southWestBrightness = block.getLuminance(this.blockView, x + 1, y, z);
                this.westBottomBrightness = block.getLuminance(this.blockView, x, y - 1, z);
                this.westTopBrightness = block.getLuminance(this.blockView, x, y + 1, z);
                this.northWestBottomBrightness = this.southWestEdgeTranslucent || this.bottomSouthEdgeTranslucent ? block.getLuminance(this.blockView, x - 1, y - 1, z) : this.northWestBrightness;
                this.northWestTopBrightness = this.southWestEdgeTranslucent || this.topSouthEdgeTranslucent ? block.getLuminance(this.blockView, x - 1, y + 1, z) : this.northWestBrightness;
                this.southWestBottomBrightness = this.southEastEdgeTranslucent || this.bottomSouthEdgeTranslucent ? block.getLuminance(this.blockView, x + 1, y - 1, z) : this.southWestBrightness;
                this.southWestTopBrightness = this.southEastEdgeTranslucent || this.topSouthEdgeTranslucent ? block.getLuminance(this.blockView, x + 1, y + 1, z) : this.southWestBrightness;
                --z;
                f = (this.northWestBrightness + this.northWestTopBrightness + this.westBrightness + this.westTopBrightness) / 4.0f;
                f4 = (this.westBrightness + this.westTopBrightness + this.southWestBrightness + this.southWestTopBrightness) / 4.0f;
                f3 = (this.westBottomBrightness + this.westBrightness + this.southWestBottomBrightness + this.southWestBrightness) / 4.0f;
                f2 = (this.northWestBottomBrightness + this.northWestBrightness + this.westBottomBrightness + this.westBrightness) / 4.0f;
            } else {
                f3 = f4 = this.westBrightness;
                f2 = f4;
                f = f4;
            }
            this.thirdVertexRed = this.fourthVertexRed = (bl5 ? red : 1.0f) * 0.8f;
            this.secondVertexRed = this.fourthVertexRed;
            this.firstVertexRed = this.fourthVertexRed;
            this.thirdVertexGreen = this.fourthVertexGreen = (bl5 ? green : 1.0f) * 0.8f;
            this.secondVertexGreen = this.fourthVertexGreen;
            this.firstVertexGreen = this.fourthVertexGreen;
            this.thirdVertexBlue = this.fourthVertexBlue = (bl5 ? blue : 1.0f) * 0.8f;
            this.secondVertexBlue = this.fourthVertexBlue;
            this.firstVertexBlue = this.fourthVertexBlue;
            this.firstVertexRed *= f;
            this.firstVertexGreen *= f;
            this.firstVertexBlue *= f;
            this.secondVertexRed *= f2;
            this.secondVertexGreen *= f2;
            this.secondVertexBlue *= f2;
            this.thirdVertexRed *= f3;
            this.thirdVertexGreen *= f3;
            this.thirdVertexBlue *= f3;
            this.fourthVertexRed *= f4;
            this.fourthVertexGreen *= f4;
            this.fourthVertexBlue *= f4;
            n = block.getTextureId(this.blockView, x, y, z, 3);
            this.renderWestFace(block, x, y, z, block.getTextureId(this.blockView, x, y, z, 3));
            if (fancyGraphics && n == 3 && this.textureOverride < 0) {
                this.firstVertexRed *= red;
                this.secondVertexRed *= red;
                this.thirdVertexRed *= red;
                this.fourthVertexRed *= red;
                this.firstVertexGreen *= green;
                this.secondVertexGreen *= green;
                this.thirdVertexGreen *= green;
                this.fourthVertexGreen *= green;
                this.firstVertexBlue *= blue;
                this.secondVertexBlue *= blue;
                this.thirdVertexBlue *= blue;
                this.fourthVertexBlue *= blue;
                this.renderWestFace(block, x, y, z, 38);
            }
            bl = true;
        }
        if (this.skipFaceCulling || block.isSideVisible(this.blockView, x - 1, y, z, 4)) {
            if (this.useSurroundingBrightness > 0) {
                this.northBottomBrightness = block.getLuminance(this.blockView, --x, y - 1, z);
                this.northEastBrightness = block.getLuminance(this.blockView, x, y, z - 1);
                this.northWestBrightness = block.getLuminance(this.blockView, x, y, z + 1);
                this.northTopBrightness = block.getLuminance(this.blockView, x, y + 1, z);
                this.northEastBottomBrightness = this.northWestEdgeTranslucent || this.bottomWestEdgeTranslucent ? block.getLuminance(this.blockView, x, y - 1, z - 1) : this.northEastBrightness;
                this.northWestBottomBrightness = this.southWestEdgeTranslucent || this.bottomWestEdgeTranslucent ? block.getLuminance(this.blockView, x, y - 1, z + 1) : this.northWestBrightness;
                this.northEastTopBrightness = this.northWestEdgeTranslucent || this.topWestEdgeTranslucent ? block.getLuminance(this.blockView, x, y + 1, z - 1) : this.northEastBrightness;
                this.northWestTopBrightness = this.southWestEdgeTranslucent || this.topWestEdgeTranslucent ? block.getLuminance(this.blockView, x, y + 1, z + 1) : this.northWestBrightness;
                ++x;
                f4 = (this.northBottomBrightness + this.northWestBottomBrightness + this.northBrightness + this.northWestBrightness) / 4.0f;
                f = (this.northBrightness + this.northWestBrightness + this.northTopBrightness + this.northWestTopBrightness) / 4.0f;
                f2 = (this.northEastBrightness + this.northBrightness + this.northEastTopBrightness + this.northTopBrightness) / 4.0f;
                f3 = (this.northEastBottomBrightness + this.northBottomBrightness + this.northEastBrightness + this.northBrightness) / 4.0f;
            } else {
                f3 = f4 = this.northBrightness;
                f2 = f4;
                f = f4;
            }
            this.thirdVertexRed = this.fourthVertexRed = (bl6 ? red : 1.0f) * 0.6f;
            this.secondVertexRed = this.fourthVertexRed;
            this.firstVertexRed = this.fourthVertexRed;
            this.thirdVertexGreen = this.fourthVertexGreen = (bl6 ? green : 1.0f) * 0.6f;
            this.secondVertexGreen = this.fourthVertexGreen;
            this.firstVertexGreen = this.fourthVertexGreen;
            this.thirdVertexBlue = this.fourthVertexBlue = (bl6 ? blue : 1.0f) * 0.6f;
            this.secondVertexBlue = this.fourthVertexBlue;
            this.firstVertexBlue = this.fourthVertexBlue;
            this.firstVertexRed *= f;
            this.firstVertexGreen *= f;
            this.firstVertexBlue *= f;
            this.secondVertexRed *= f2;
            this.secondVertexGreen *= f2;
            this.secondVertexBlue *= f2;
            this.thirdVertexRed *= f3;
            this.thirdVertexGreen *= f3;
            this.thirdVertexBlue *= f3;
            this.fourthVertexRed *= f4;
            this.fourthVertexGreen *= f4;
            this.fourthVertexBlue *= f4;
            n = block.getTextureId(this.blockView, x, y, z, 4);
            this.renderNorthFace(block, x, y, z, n);
            if (fancyGraphics && n == 3 && this.textureOverride < 0) {
                this.firstVertexRed *= red;
                this.secondVertexRed *= red;
                this.thirdVertexRed *= red;
                this.fourthVertexRed *= red;
                this.firstVertexGreen *= green;
                this.secondVertexGreen *= green;
                this.thirdVertexGreen *= green;
                this.fourthVertexGreen *= green;
                this.firstVertexBlue *= blue;
                this.secondVertexBlue *= blue;
                this.thirdVertexBlue *= blue;
                this.fourthVertexBlue *= blue;
                this.renderNorthFace(block, x, y, z, 38);
            }
            bl = true;
        }
        if (this.skipFaceCulling || block.isSideVisible(this.blockView, x + 1, y, z, 5)) {
            if (this.useSurroundingBrightness > 0) {
                this.southBottomBrightness = block.getLuminance(this.blockView, ++x, y - 1, z);
                this.southEastBrightness = block.getLuminance(this.blockView, x, y, z - 1);
                this.southWestBrightness = block.getLuminance(this.blockView, x, y, z + 1);
                this.southTopBrightness = block.getLuminance(this.blockView, x, y + 1, z);
                this.southEastBottomBrightness = this.bottomEastEdgeTranslucent || this.northEastEdgeTranslucent ? block.getLuminance(this.blockView, x, y - 1, z - 1) : this.southEastBrightness;
                this.southWestBottomBrightness = this.bottomEastEdgeTranslucent || this.southEastEdgeTranslucent ? block.getLuminance(this.blockView, x, y - 1, z + 1) : this.southWestBrightness;
                this.southEastTopBrightness = this.topEastEdgeTranslucent || this.northEastEdgeTranslucent ? block.getLuminance(this.blockView, x, y + 1, z - 1) : this.southEastBrightness;
                this.southWestTopBrightness = this.topEastEdgeTranslucent || this.southEastEdgeTranslucent ? block.getLuminance(this.blockView, x, y + 1, z + 1) : this.southWestBrightness;
                --x;
                f = (this.southBottomBrightness + this.southWestBottomBrightness + this.southBrightness + this.southWestBrightness) / 4.0f;
                f4 = (this.southBrightness + this.southWestBrightness + this.southTopBrightness + this.southWestTopBrightness) / 4.0f;
                f3 = (this.southEastBrightness + this.southBrightness + this.southEastTopBrightness + this.southTopBrightness) / 4.0f;
                f2 = (this.southEastBottomBrightness + this.southBottomBrightness + this.southEastBrightness + this.southBrightness) / 4.0f;
            } else {
                f3 = f4 = this.southBrightness;
                f2 = f4;
                f = f4;
            }
            this.thirdVertexRed = this.fourthVertexRed = (bl7 ? red : 1.0f) * 0.6f;
            this.secondVertexRed = this.fourthVertexRed;
            this.firstVertexRed = this.fourthVertexRed;
            this.thirdVertexGreen = this.fourthVertexGreen = (bl7 ? green : 1.0f) * 0.6f;
            this.secondVertexGreen = this.fourthVertexGreen;
            this.firstVertexGreen = this.fourthVertexGreen;
            this.thirdVertexBlue = this.fourthVertexBlue = (bl7 ? blue : 1.0f) * 0.6f;
            this.secondVertexBlue = this.fourthVertexBlue;
            this.firstVertexBlue = this.fourthVertexBlue;
            this.firstVertexRed *= f;
            this.firstVertexGreen *= f;
            this.firstVertexBlue *= f;
            this.secondVertexRed *= f2;
            this.secondVertexGreen *= f2;
            this.secondVertexBlue *= f2;
            this.thirdVertexRed *= f3;
            this.thirdVertexGreen *= f3;
            this.thirdVertexBlue *= f3;
            this.fourthVertexRed *= f4;
            this.fourthVertexGreen *= f4;
            this.fourthVertexBlue *= f4;
            n = block.getTextureId(this.blockView, x, y, z, 5);
            this.renderSouthFace(block, x, y, z, n);
            if (fancyGraphics && n == 3 && this.textureOverride < 0) {
                this.firstVertexRed *= red;
                this.secondVertexRed *= red;
                this.thirdVertexRed *= red;
                this.fourthVertexRed *= red;
                this.firstVertexGreen *= green;
                this.secondVertexGreen *= green;
                this.thirdVertexGreen *= green;
                this.fourthVertexGreen *= green;
                this.firstVertexBlue *= blue;
                this.secondVertexBlue *= blue;
                this.thirdVertexBlue *= blue;
                this.fourthVertexBlue *= blue;
                this.renderSouthFace(block, x, y, z, 38);
            }
            bl = true;
        }
        this.useAo = false;
        return bl;
    }

    public boolean renderFlat(Block block, int x, int y, int z, float red, float green, float blue) {
        int n;
        float f;
        this.useAo = false;
        Tessellator tessellator = Tessellator.INSTANCE;
        boolean bl = false;
        float f2 = 0.5f;
        float f3 = 1.0f;
        float f4 = 0.8f;
        float f5 = 0.6f;
        float f6 = f3 * red;
        float f7 = f3 * green;
        float f8 = f3 * blue;
        float f9 = f2;
        float f10 = f4;
        float f11 = f5;
        float f12 = f2;
        float f13 = f4;
        float f14 = f5;
        float f15 = f2;
        float f16 = f4;
        float f17 = f5;
        if (block != Block.GRASS_BLOCK) {
            f9 *= red;
            f10 *= red;
            f11 *= red;
            f12 *= green;
            f13 *= green;
            f14 *= green;
            f15 *= blue;
            f16 *= blue;
            f17 *= blue;
        }
        float f18 = block.getLuminance(this.blockView, x, y, z);
        if (this.skipFaceCulling || block.isSideVisible(this.blockView, x, y - 1, z, 0)) {
            f = block.getLuminance(this.blockView, x, y - 1, z);
            tessellator.color(f9 * f, f12 * f, f15 * f);
            this.renderBottomFace(block, x, y, z, block.getTextureId(this.blockView, x, y, z, 0));
            bl = true;
        }
        if (this.skipFaceCulling || block.isSideVisible(this.blockView, x, y + 1, z, 1)) {
            f = block.getLuminance(this.blockView, x, y + 1, z);
            if (block.maxY != 1.0 && !block.material.isFluid()) {
                f = f18;
            }
            tessellator.color(f6 * f, f7 * f, f8 * f);
            this.renderTopFace(block, x, y, z, block.getTextureId(this.blockView, x, y, z, 1));
            bl = true;
        }
        if (this.skipFaceCulling || block.isSideVisible(this.blockView, x, y, z - 1, 2)) {
            f = block.getLuminance(this.blockView, x, y, z - 1);
            if (block.minZ > 0.0) {
                f = f18;
            }
            tessellator.color(f10 * f, f13 * f, f16 * f);
            n = block.getTextureId(this.blockView, x, y, z, 2);
            this.renderEastFace(block, x, y, z, n);
            if (fancyGraphics && n == 3 && this.textureOverride < 0) {
                tessellator.color(f10 * f * red, f13 * f * green, f16 * f * blue);
                this.renderEastFace(block, x, y, z, 38);
            }
            bl = true;
        }
        if (this.skipFaceCulling || block.isSideVisible(this.blockView, x, y, z + 1, 3)) {
            f = block.getLuminance(this.blockView, x, y, z + 1);
            if (block.maxZ < 1.0) {
                f = f18;
            }
            tessellator.color(f10 * f, f13 * f, f16 * f);
            n = block.getTextureId(this.blockView, x, y, z, 3);
            this.renderWestFace(block, x, y, z, n);
            if (fancyGraphics && n == 3 && this.textureOverride < 0) {
                tessellator.color(f10 * f * red, f13 * f * green, f16 * f * blue);
                this.renderWestFace(block, x, y, z, 38);
            }
            bl = true;
        }
        if (this.skipFaceCulling || block.isSideVisible(this.blockView, x - 1, y, z, 4)) {
            f = block.getLuminance(this.blockView, x - 1, y, z);
            if (block.minX > 0.0) {
                f = f18;
            }
            tessellator.color(f11 * f, f14 * f, f17 * f);
            n = block.getTextureId(this.blockView, x, y, z, 4);
            this.renderNorthFace(block, x, y, z, n);
            if (fancyGraphics && n == 3 && this.textureOverride < 0) {
                tessellator.color(f11 * f * red, f14 * f * green, f17 * f * blue);
                this.renderNorthFace(block, x, y, z, 38);
            }
            bl = true;
        }
        if (this.skipFaceCulling || block.isSideVisible(this.blockView, x + 1, y, z, 5)) {
            f = block.getLuminance(this.blockView, x + 1, y, z);
            if (block.maxX < 1.0) {
                f = f18;
            }
            tessellator.color(f11 * f, f14 * f, f17 * f);
            n = block.getTextureId(this.blockView, x, y, z, 5);
            this.renderSouthFace(block, x, y, z, n);
            if (fancyGraphics && n == 3 && this.textureOverride < 0) {
                tessellator.color(f11 * f * red, f14 * f * green, f17 * f * blue);
                this.renderSouthFace(block, x, y, z, 38);
            }
            bl = true;
        }
        return bl;
    }

    public boolean renderCactus(Block block, int x, int y, int z) {
        int n = block.getColorMultiplier(this.blockView, x, y, z);
        float f = (float)(n >> 16 & 0xFF) / 255.0f;
        float f2 = (float)(n >> 8 & 0xFF) / 255.0f;
        float f3 = (float)(n & 0xFF) / 255.0f;
        if (GameRenderer.anaglyph3d) {
            float f4 = (f * 30.0f + f2 * 59.0f + f3 * 11.0f) / 100.0f;
            float f5 = (f * 30.0f + f2 * 70.0f) / 100.0f;
            float f6 = (f * 30.0f + f3 * 70.0f) / 100.0f;
            f = f4;
            f2 = f5;
            f3 = f6;
        }
        return this.renderCactus(block, x, y, z, f, f2, f3);
    }

    public boolean renderCactus(Block block, int x, int y, int z, float red, float green, float blue) {
        float f;
        Tessellator tessellator = Tessellator.INSTANCE;
        boolean bl = false;
        float f2 = 0.5f;
        float f3 = 1.0f;
        float f4 = 0.8f;
        float f5 = 0.6f;
        float f6 = f2 * red;
        float f7 = f3 * red;
        float f8 = f4 * red;
        float f9 = f5 * red;
        float f10 = f2 * green;
        float f11 = f3 * green;
        float f12 = f4 * green;
        float f13 = f5 * green;
        float f14 = f2 * blue;
        float f15 = f3 * blue;
        float f16 = f4 * blue;
        float f17 = f5 * blue;
        float f18 = 0.0625f;
        float f19 = block.getLuminance(this.blockView, x, y, z);
        if (this.skipFaceCulling || block.isSideVisible(this.blockView, x, y - 1, z, 0)) {
            f = block.getLuminance(this.blockView, x, y - 1, z);
            tessellator.color(f6 * f, f10 * f, f14 * f);
            this.renderBottomFace(block, x, y, z, block.getTextureId(this.blockView, x, y, z, 0));
            bl = true;
        }
        if (this.skipFaceCulling || block.isSideVisible(this.blockView, x, y + 1, z, 1)) {
            f = block.getLuminance(this.blockView, x, y + 1, z);
            if (block.maxY != 1.0 && !block.material.isFluid()) {
                f = f19;
            }
            tessellator.color(f7 * f, f11 * f, f15 * f);
            this.renderTopFace(block, x, y, z, block.getTextureId(this.blockView, x, y, z, 1));
            bl = true;
        }
        if (this.skipFaceCulling || block.isSideVisible(this.blockView, x, y, z - 1, 2)) {
            f = block.getLuminance(this.blockView, x, y, z - 1);
            if (block.minZ > 0.0) {
                f = f19;
            }
            tessellator.color(f8 * f, f12 * f, f16 * f);
            tessellator.translate(0.0f, 0.0f, f18);
            this.renderEastFace(block, x, y, z, block.getTextureId(this.blockView, x, y, z, 2));
            tessellator.translate(0.0f, 0.0f, -f18);
            bl = true;
        }
        if (this.skipFaceCulling || block.isSideVisible(this.blockView, x, y, z + 1, 3)) {
            f = block.getLuminance(this.blockView, x, y, z + 1);
            if (block.maxZ < 1.0) {
                f = f19;
            }
            tessellator.color(f8 * f, f12 * f, f16 * f);
            tessellator.translate(0.0f, 0.0f, -f18);
            this.renderWestFace(block, x, y, z, block.getTextureId(this.blockView, x, y, z, 3));
            tessellator.translate(0.0f, 0.0f, f18);
            bl = true;
        }
        if (this.skipFaceCulling || block.isSideVisible(this.blockView, x - 1, y, z, 4)) {
            f = block.getLuminance(this.blockView, x - 1, y, z);
            if (block.minX > 0.0) {
                f = f19;
            }
            tessellator.color(f9 * f, f13 * f, f17 * f);
            tessellator.translate(f18, 0.0f, 0.0f);
            this.renderNorthFace(block, x, y, z, block.getTextureId(this.blockView, x, y, z, 4));
            tessellator.translate(-f18, 0.0f, 0.0f);
            bl = true;
        }
        if (this.skipFaceCulling || block.isSideVisible(this.blockView, x + 1, y, z, 5)) {
            f = block.getLuminance(this.blockView, x + 1, y, z);
            if (block.maxX < 1.0) {
                f = f19;
            }
            tessellator.color(f9 * f, f13 * f, f17 * f);
            tessellator.translate(-f18, 0.0f, 0.0f);
            this.renderSouthFace(block, x, y, z, block.getTextureId(this.blockView, x, y, z, 5));
            tessellator.translate(f18, 0.0f, 0.0f);
            bl = true;
        }
        return bl;
    }

    public boolean renderFence(Block block, int x, int y, int z) {
        float f;
        boolean bl;
        boolean bl2 = false;
        float f2 = 0.375f;
        float f3 = 0.625f;
        block.setBoundingBox(f2, 0.0f, f2, f3, 1.0f, f3);
        this.renderBlock(block, x, y, z);
        bl2 = true;
        boolean bl3 = false;
        boolean bl4 = false;
        if (this.blockView.getBlockId(x - 1, y, z) == block.id || this.blockView.getBlockId(x + 1, y, z) == block.id) {
            bl3 = true;
        }
        if (this.blockView.getBlockId(x, y, z - 1) == block.id || this.blockView.getBlockId(x, y, z + 1) == block.id) {
            bl4 = true;
        }
        boolean bl5 = this.blockView.getBlockId(x - 1, y, z) == block.id;
        boolean bl6 = this.blockView.getBlockId(x + 1, y, z) == block.id;
        boolean bl7 = this.blockView.getBlockId(x, y, z - 1) == block.id;
        boolean bl8 = bl = this.blockView.getBlockId(x, y, z + 1) == block.id;
        if (!bl3 && !bl4) {
            bl3 = true;
        }
        f2 = 0.4375f;
        f3 = 0.5625f;
        float f4 = 0.75f;
        float f5 = 0.9375f;
        float f6 = bl5 ? 0.0f : f2;
        float f7 = bl6 ? 1.0f : f3;
        float f8 = bl7 ? 0.0f : f2;
        float f9 = f = bl ? 1.0f : f3;
        if (bl3) {
            block.setBoundingBox(f6, f4, f2, f7, f5, f3);
            this.renderBlock(block, x, y, z);
            bl2 = true;
        }
        if (bl4) {
            block.setBoundingBox(f2, f4, f8, f3, f5, f);
            this.renderBlock(block, x, y, z);
            bl2 = true;
        }
        f4 = 0.375f;
        f5 = 0.5625f;
        if (bl3) {
            block.setBoundingBox(f6, f4, f2, f7, f5, f3);
            this.renderBlock(block, x, y, z);
            bl2 = true;
        }
        if (bl4) {
            block.setBoundingBox(f2, f4, f8, f3, f5, f);
            this.renderBlock(block, x, y, z);
            bl2 = true;
        }
        block.setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
        return bl2;
    }

    public boolean renderStairs(Block block, int x, int y, int z) {
        boolean bl = false;
        int n = this.blockView.getBlockMeta(x, y, z);
        if (n == 0) {
            block.setBoundingBox(0.0f, 0.0f, 0.0f, 0.5f, 0.5f, 1.0f);
            this.renderBlock(block, x, y, z);
            block.setBoundingBox(0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
            this.renderBlock(block, x, y, z);
            bl = true;
        } else if (n == 1) {
            block.setBoundingBox(0.0f, 0.0f, 0.0f, 0.5f, 1.0f, 1.0f);
            this.renderBlock(block, x, y, z);
            block.setBoundingBox(0.5f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f);
            this.renderBlock(block, x, y, z);
            bl = true;
        } else if (n == 2) {
            block.setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 0.5f);
            this.renderBlock(block, x, y, z);
            block.setBoundingBox(0.0f, 0.0f, 0.5f, 1.0f, 1.0f, 1.0f);
            this.renderBlock(block, x, y, z);
            bl = true;
        } else if (n == 3) {
            block.setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.5f);
            this.renderBlock(block, x, y, z);
            block.setBoundingBox(0.0f, 0.0f, 0.5f, 1.0f, 0.5f, 1.0f);
            this.renderBlock(block, x, y, z);
            bl = true;
        }
        block.setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
        return bl;
    }

    public boolean renderDoor(Block block, int x, int y, int z) {
        Tessellator tessellator = Tessellator.INSTANCE;
        DoorBlock doorBlock = (DoorBlock)block;
        boolean bl = false;
        float f = 0.5f;
        float f2 = 1.0f;
        float f3 = 0.8f;
        float f4 = 0.6f;
        float f5 = block.getLuminance(this.blockView, x, y, z);
        float f6 = block.getLuminance(this.blockView, x, y - 1, z);
        if (doorBlock.minY > 0.0) {
            f6 = f5;
        }
        if (Block.BLOCKS_LIGHT_LUMINANCE[block.id] > 0) {
            f6 = 1.0f;
        }
        tessellator.color(f * f6, f * f6, f * f6);
        this.renderBottomFace(block, x, y, z, block.getTextureId(this.blockView, x, y, z, 0));
        bl = true;
        f6 = block.getLuminance(this.blockView, x, y + 1, z);
        if (doorBlock.maxY < 1.0) {
            f6 = f5;
        }
        if (Block.BLOCKS_LIGHT_LUMINANCE[block.id] > 0) {
            f6 = 1.0f;
        }
        tessellator.color(f2 * f6, f2 * f6, f2 * f6);
        this.renderTopFace(block, x, y, z, block.getTextureId(this.blockView, x, y, z, 1));
        bl = true;
        f6 = block.getLuminance(this.blockView, x, y, z - 1);
        if (doorBlock.minZ > 0.0) {
            f6 = f5;
        }
        if (Block.BLOCKS_LIGHT_LUMINANCE[block.id] > 0) {
            f6 = 1.0f;
        }
        tessellator.color(f3 * f6, f3 * f6, f3 * f6);
        int n = block.getTextureId(this.blockView, x, y, z, 2);
        if (n < 0) {
            this.flipTextureHorizontally = true;
            n = -n;
        }
        this.renderEastFace(block, x, y, z, n);
        bl = true;
        this.flipTextureHorizontally = false;
        f6 = block.getLuminance(this.blockView, x, y, z + 1);
        if (doorBlock.maxZ < 1.0) {
            f6 = f5;
        }
        if (Block.BLOCKS_LIGHT_LUMINANCE[block.id] > 0) {
            f6 = 1.0f;
        }
        tessellator.color(f3 * f6, f3 * f6, f3 * f6);
        n = block.getTextureId(this.blockView, x, y, z, 3);
        if (n < 0) {
            this.flipTextureHorizontally = true;
            n = -n;
        }
        this.renderWestFace(block, x, y, z, n);
        bl = true;
        this.flipTextureHorizontally = false;
        f6 = block.getLuminance(this.blockView, x - 1, y, z);
        if (doorBlock.minX > 0.0) {
            f6 = f5;
        }
        if (Block.BLOCKS_LIGHT_LUMINANCE[block.id] > 0) {
            f6 = 1.0f;
        }
        tessellator.color(f4 * f6, f4 * f6, f4 * f6);
        n = block.getTextureId(this.blockView, x, y, z, 4);
        if (n < 0) {
            this.flipTextureHorizontally = true;
            n = -n;
        }
        this.renderNorthFace(block, x, y, z, n);
        bl = true;
        this.flipTextureHorizontally = false;
        f6 = block.getLuminance(this.blockView, x + 1, y, z);
        if (doorBlock.maxX < 1.0) {
            f6 = f5;
        }
        if (Block.BLOCKS_LIGHT_LUMINANCE[block.id] > 0) {
            f6 = 1.0f;
        }
        tessellator.color(f4 * f6, f4 * f6, f4 * f6);
        n = block.getTextureId(this.blockView, x, y, z, 5);
        if (n < 0) {
            this.flipTextureHorizontally = true;
            n = -n;
        }
        this.renderSouthFace(block, x, y, z, n);
        bl = true;
        this.flipTextureHorizontally = false;
        return bl;
    }

    public void renderBottomFace(Block block, double x, double y, double z, int texture) {
        Tessellator tessellator = Tessellator.INSTANCE;
        if (this.textureOverride >= 0) {
            texture = this.textureOverride;
        }
        int n = (texture & 0xF) << 4;
        int n2 = texture & 0xF0;
        double d = ((double)n + block.minX * 16.0) / 256.0;
        double d2 = ((double)n + block.maxX * 16.0 - 0.01) / 256.0;
        double d3 = ((double)n2 + block.minZ * 16.0) / 256.0;
        double d4 = ((double)n2 + block.maxZ * 16.0 - 0.01) / 256.0;
        if (block.minX < 0.0 || block.maxX > 1.0) {
            d = ((float)n + 0.0f) / 256.0f;
            d2 = ((float)n + 15.99f) / 256.0f;
        }
        if (block.minZ < 0.0 || block.maxZ > 1.0) {
            d3 = ((float)n2 + 0.0f) / 256.0f;
            d4 = ((float)n2 + 15.99f) / 256.0f;
        }
        double d5 = d2;
        double d6 = d;
        double d7 = d3;
        double d8 = d4;
        if (this.bottomFaceRotation == 2) {
            d = ((double)n + block.minZ * 16.0) / 256.0;
            d3 = ((double)(n2 + 16) - block.maxX * 16.0) / 256.0;
            d2 = ((double)n + block.maxZ * 16.0) / 256.0;
            d4 = ((double)(n2 + 16) - block.minX * 16.0) / 256.0;
            d5 = d2;
            d6 = d;
            d7 = d3;
            d8 = d4;
            d5 = d;
            d6 = d2;
            d3 = d4;
            d4 = d7;
        } else if (this.bottomFaceRotation == 1) {
            d = ((double)(n + 16) - block.maxZ * 16.0) / 256.0;
            d3 = ((double)n2 + block.minX * 16.0) / 256.0;
            d2 = ((double)(n + 16) - block.minZ * 16.0) / 256.0;
            d4 = ((double)n2 + block.maxX * 16.0) / 256.0;
            d5 = d2;
            d6 = d;
            d7 = d3;
            d8 = d4;
            d = d5;
            d2 = d6;
            d7 = d4;
            d8 = d3;
        } else if (this.bottomFaceRotation == 3) {
            d = ((double)(n + 16) - block.minX * 16.0) / 256.0;
            d2 = ((double)(n + 16) - block.maxX * 16.0 - 0.01) / 256.0;
            d3 = ((double)(n2 + 16) - block.minZ * 16.0) / 256.0;
            d4 = ((double)(n2 + 16) - block.maxZ * 16.0 - 0.01) / 256.0;
            d5 = d2;
            d6 = d;
            d7 = d3;
            d8 = d4;
        }
        double d9 = x + block.minX;
        double d10 = x + block.maxX;
        double d11 = y + block.minY;
        double d12 = z + block.minZ;
        double d13 = z + block.maxZ;
        if (this.useAo) {
            tessellator.color(this.firstVertexRed, this.firstVertexGreen, this.firstVertexBlue);
            tessellator.vertex(d9, d11, d13, d6, d8);
            tessellator.color(this.secondVertexRed, this.secondVertexGreen, this.secondVertexBlue);
            tessellator.vertex(d9, d11, d12, d, d3);
            tessellator.color(this.thirdVertexRed, this.thirdVertexGreen, this.thirdVertexBlue);
            tessellator.vertex(d10, d11, d12, d5, d7);
            tessellator.color(this.fourthVertexRed, this.fourthVertexGreen, this.fourthVertexBlue);
            tessellator.vertex(d10, d11, d13, d2, d4);
        } else {
            tessellator.vertex(d9, d11, d13, d6, d8);
            tessellator.vertex(d9, d11, d12, d, d3);
            tessellator.vertex(d10, d11, d12, d5, d7);
            tessellator.vertex(d10, d11, d13, d2, d4);
        }
    }

    public void renderTopFace(Block block, double x, double y, double z, int texture) {
        Tessellator tessellator = Tessellator.INSTANCE;
        if (this.textureOverride >= 0) {
            texture = this.textureOverride;
        }
        int n = (texture & 0xF) << 4;
        int n2 = texture & 0xF0;
        double d = ((double)n + block.minX * 16.0) / 256.0;
        double d2 = ((double)n + block.maxX * 16.0 - 0.01) / 256.0;
        double d3 = ((double)n2 + block.minZ * 16.0) / 256.0;
        double d4 = ((double)n2 + block.maxZ * 16.0 - 0.01) / 256.0;
        if (block.minX < 0.0 || block.maxX > 1.0) {
            d = ((float)n + 0.0f) / 256.0f;
            d2 = ((float)n + 15.99f) / 256.0f;
        }
        if (block.minZ < 0.0 || block.maxZ > 1.0) {
            d3 = ((float)n2 + 0.0f) / 256.0f;
            d4 = ((float)n2 + 15.99f) / 256.0f;
        }
        double d5 = d2;
        double d6 = d;
        double d7 = d3;
        double d8 = d4;
        if (this.topFaceRotation == 1) {
            d = ((double)n + block.minZ * 16.0) / 256.0;
            d3 = ((double)(n2 + 16) - block.maxX * 16.0) / 256.0;
            d2 = ((double)n + block.maxZ * 16.0) / 256.0;
            d4 = ((double)(n2 + 16) - block.minX * 16.0) / 256.0;
            d5 = d2;
            d6 = d;
            d7 = d3;
            d8 = d4;
            d5 = d;
            d6 = d2;
            d3 = d4;
            d4 = d7;
        } else if (this.topFaceRotation == 2) {
            d = ((double)(n + 16) - block.maxZ * 16.0) / 256.0;
            d3 = ((double)n2 + block.minX * 16.0) / 256.0;
            d2 = ((double)(n + 16) - block.minZ * 16.0) / 256.0;
            d4 = ((double)n2 + block.maxX * 16.0) / 256.0;
            d5 = d2;
            d6 = d;
            d7 = d3;
            d8 = d4;
            d = d5;
            d2 = d6;
            d7 = d4;
            d8 = d3;
        } else if (this.topFaceRotation == 3) {
            d = ((double)(n + 16) - block.minX * 16.0) / 256.0;
            d2 = ((double)(n + 16) - block.maxX * 16.0 - 0.01) / 256.0;
            d3 = ((double)(n2 + 16) - block.minZ * 16.0) / 256.0;
            d4 = ((double)(n2 + 16) - block.maxZ * 16.0 - 0.01) / 256.0;
            d5 = d2;
            d6 = d;
            d7 = d3;
            d8 = d4;
        }
        double d9 = x + block.minX;
        double d10 = x + block.maxX;
        double d11 = y + block.maxY;
        double d12 = z + block.minZ;
        double d13 = z + block.maxZ;
        if (this.useAo) {
            tessellator.color(this.firstVertexRed, this.firstVertexGreen, this.firstVertexBlue);
            tessellator.vertex(d10, d11, d13, d2, d4);
            tessellator.color(this.secondVertexRed, this.secondVertexGreen, this.secondVertexBlue);
            tessellator.vertex(d10, d11, d12, d5, d7);
            tessellator.color(this.thirdVertexRed, this.thirdVertexGreen, this.thirdVertexBlue);
            tessellator.vertex(d9, d11, d12, d, d3);
            tessellator.color(this.fourthVertexRed, this.fourthVertexGreen, this.fourthVertexBlue);
            tessellator.vertex(d9, d11, d13, d6, d8);
        } else {
            tessellator.vertex(d10, d11, d13, d2, d4);
            tessellator.vertex(d10, d11, d12, d5, d7);
            tessellator.vertex(d9, d11, d12, d, d3);
            tessellator.vertex(d9, d11, d13, d6, d8);
        }
    }

    public void renderEastFace(Block block, double x, double y, double z, int texture) {
        double d;
        Tessellator tessellator = Tessellator.INSTANCE;
        if (this.textureOverride >= 0) {
            texture = this.textureOverride;
        }
        int n = (texture & 0xF) << 4;
        int n2 = texture & 0xF0;
        double d2 = ((double)n + block.minX * 16.0) / 256.0;
        double d3 = ((double)n + block.maxX * 16.0 - 0.01) / 256.0;
        double d4 = ((double)(n2 + 16) - block.maxY * 16.0) / 256.0;
        double d5 = ((double)(n2 + 16) - block.minY * 16.0 - 0.01) / 256.0;
        if (this.flipTextureHorizontally) {
            d = d2;
            d2 = d3;
            d3 = d;
        }
        if (block.minX < 0.0 || block.maxX > 1.0) {
            d2 = ((float)n + 0.0f) / 256.0f;
            d3 = ((float)n + 15.99f) / 256.0f;
        }
        if (block.minY < 0.0 || block.maxY > 1.0) {
            d4 = ((float)n2 + 0.0f) / 256.0f;
            d5 = ((float)n2 + 15.99f) / 256.0f;
        }
        d = d3;
        double d6 = d2;
        double d7 = d4;
        double d8 = d5;
        if (this.eastFaceRotation == 2) {
            d2 = ((double)n + block.minY * 16.0) / 256.0;
            d4 = ((double)(n2 + 16) - block.minX * 16.0) / 256.0;
            d3 = ((double)n + block.maxY * 16.0) / 256.0;
            d5 = ((double)(n2 + 16) - block.maxX * 16.0) / 256.0;
            d = d3;
            d6 = d2;
            d7 = d4;
            d8 = d5;
            d = d2;
            d6 = d3;
            d4 = d5;
            d5 = d7;
        } else if (this.eastFaceRotation == 1) {
            d2 = ((double)(n + 16) - block.maxY * 16.0) / 256.0;
            d4 = ((double)n2 + block.maxX * 16.0) / 256.0;
            d3 = ((double)(n + 16) - block.minY * 16.0) / 256.0;
            d5 = ((double)n2 + block.minX * 16.0) / 256.0;
            d = d3;
            d6 = d2;
            d7 = d4;
            d8 = d5;
            d2 = d;
            d3 = d6;
            d7 = d5;
            d8 = d4;
        } else if (this.eastFaceRotation == 3) {
            d2 = ((double)(n + 16) - block.minX * 16.0) / 256.0;
            d3 = ((double)(n + 16) - block.maxX * 16.0 - 0.01) / 256.0;
            d4 = ((double)n2 + block.maxY * 16.0) / 256.0;
            d5 = ((double)n2 + block.minY * 16.0 - 0.01) / 256.0;
            d = d3;
            d6 = d2;
            d7 = d4;
            d8 = d5;
        }
        double d9 = x + block.minX;
        double d10 = x + block.maxX;
        double d11 = y + block.minY;
        double d12 = y + block.maxY;
        double d13 = z + block.minZ;
        if (this.useAo) {
            tessellator.color(this.firstVertexRed, this.firstVertexGreen, this.firstVertexBlue);
            tessellator.vertex(d9, d12, d13, d, d7);
            tessellator.color(this.secondVertexRed, this.secondVertexGreen, this.secondVertexBlue);
            tessellator.vertex(d10, d12, d13, d2, d4);
            tessellator.color(this.thirdVertexRed, this.thirdVertexGreen, this.thirdVertexBlue);
            tessellator.vertex(d10, d11, d13, d6, d8);
            tessellator.color(this.fourthVertexRed, this.fourthVertexGreen, this.fourthVertexBlue);
            tessellator.vertex(d9, d11, d13, d3, d5);
        } else {
            tessellator.vertex(d9, d12, d13, d, d7);
            tessellator.vertex(d10, d12, d13, d2, d4);
            tessellator.vertex(d10, d11, d13, d6, d8);
            tessellator.vertex(d9, d11, d13, d3, d5);
        }
    }

    public void renderWestFace(Block block, double x, double y, double z, int texture) {
        double d;
        Tessellator tessellator = Tessellator.INSTANCE;
        if (this.textureOverride >= 0) {
            texture = this.textureOverride;
        }
        int n = (texture & 0xF) << 4;
        int n2 = texture & 0xF0;
        double d2 = ((double)n + block.minX * 16.0) / 256.0;
        double d3 = ((double)n + block.maxX * 16.0 - 0.01) / 256.0;
        double d4 = ((double)(n2 + 16) - block.maxY * 16.0) / 256.0;
        double d5 = ((double)(n2 + 16) - block.minY * 16.0 - 0.01) / 256.0;
        if (this.flipTextureHorizontally) {
            d = d2;
            d2 = d3;
            d3 = d;
        }
        if (block.minX < 0.0 || block.maxX > 1.0) {
            d2 = ((float)n + 0.0f) / 256.0f;
            d3 = ((float)n + 15.99f) / 256.0f;
        }
        if (block.minY < 0.0 || block.maxY > 1.0) {
            d4 = ((float)n2 + 0.0f) / 256.0f;
            d5 = ((float)n2 + 15.99f) / 256.0f;
        }
        d = d3;
        double d6 = d2;
        double d7 = d4;
        double d8 = d5;
        if (this.westFaceRotation == 1) {
            d2 = ((double)n + block.minY * 16.0) / 256.0;
            d5 = ((double)(n2 + 16) - block.minX * 16.0) / 256.0;
            d3 = ((double)n + block.maxY * 16.0) / 256.0;
            d4 = ((double)(n2 + 16) - block.maxX * 16.0) / 256.0;
            d = d3;
            d6 = d2;
            d7 = d4;
            d8 = d5;
            d = d2;
            d6 = d3;
            d4 = d5;
            d5 = d7;
        } else if (this.westFaceRotation == 2) {
            d2 = ((double)(n + 16) - block.maxY * 16.0) / 256.0;
            d4 = ((double)n2 + block.minX * 16.0) / 256.0;
            d3 = ((double)(n + 16) - block.minY * 16.0) / 256.0;
            d5 = ((double)n2 + block.maxX * 16.0) / 256.0;
            d = d3;
            d6 = d2;
            d7 = d4;
            d8 = d5;
            d2 = d;
            d3 = d6;
            d7 = d5;
            d8 = d4;
        } else if (this.westFaceRotation == 3) {
            d2 = ((double)(n + 16) - block.minX * 16.0) / 256.0;
            d3 = ((double)(n + 16) - block.maxX * 16.0 - 0.01) / 256.0;
            d4 = ((double)n2 + block.maxY * 16.0) / 256.0;
            d5 = ((double)n2 + block.minY * 16.0 - 0.01) / 256.0;
            d = d3;
            d6 = d2;
            d7 = d4;
            d8 = d5;
        }
        double d9 = x + block.minX;
        double d10 = x + block.maxX;
        double d11 = y + block.minY;
        double d12 = y + block.maxY;
        double d13 = z + block.maxZ;
        if (this.useAo) {
            tessellator.color(this.firstVertexRed, this.firstVertexGreen, this.firstVertexBlue);
            tessellator.vertex(d9, d12, d13, d2, d4);
            tessellator.color(this.secondVertexRed, this.secondVertexGreen, this.secondVertexBlue);
            tessellator.vertex(d9, d11, d13, d6, d8);
            tessellator.color(this.thirdVertexRed, this.thirdVertexGreen, this.thirdVertexBlue);
            tessellator.vertex(d10, d11, d13, d3, d5);
            tessellator.color(this.fourthVertexRed, this.fourthVertexGreen, this.fourthVertexBlue);
            tessellator.vertex(d10, d12, d13, d, d7);
        } else {
            tessellator.vertex(d9, d12, d13, d2, d4);
            tessellator.vertex(d9, d11, d13, d6, d8);
            tessellator.vertex(d10, d11, d13, d3, d5);
            tessellator.vertex(d10, d12, d13, d, d7);
        }
    }

    public void renderNorthFace(Block block, double x, double y, double z, int texture) {
        double d;
        Tessellator tessellator = Tessellator.INSTANCE;
        if (this.textureOverride >= 0) {
            texture = this.textureOverride;
        }
        int n = (texture & 0xF) << 4;
        int n2 = texture & 0xF0;
        double d2 = ((double)n + block.minZ * 16.0) / 256.0;
        double d3 = ((double)n + block.maxZ * 16.0 - 0.01) / 256.0;
        double d4 = ((double)(n2 + 16) - block.maxY * 16.0) / 256.0;
        double d5 = ((double)(n2 + 16) - block.minY * 16.0 - 0.01) / 256.0;
        if (this.flipTextureHorizontally) {
            d = d2;
            d2 = d3;
            d3 = d;
        }
        if (block.minZ < 0.0 || block.maxZ > 1.0) {
            d2 = ((float)n + 0.0f) / 256.0f;
            d3 = ((float)n + 15.99f) / 256.0f;
        }
        if (block.minY < 0.0 || block.maxY > 1.0) {
            d4 = ((float)n2 + 0.0f) / 256.0f;
            d5 = ((float)n2 + 15.99f) / 256.0f;
        }
        d = d3;
        double d6 = d2;
        double d7 = d4;
        double d8 = d5;
        if (this.northFaceRotation == 1) {
            d2 = ((double)n + block.minY * 16.0) / 256.0;
            d4 = ((double)(n2 + 16) - block.maxZ * 16.0) / 256.0;
            d3 = ((double)n + block.maxY * 16.0) / 256.0;
            d5 = ((double)(n2 + 16) - block.minZ * 16.0) / 256.0;
            d = d3;
            d6 = d2;
            d7 = d4;
            d8 = d5;
            d = d2;
            d6 = d3;
            d4 = d5;
            d5 = d7;
        } else if (this.northFaceRotation == 2) {
            d2 = ((double)(n + 16) - block.maxY * 16.0) / 256.0;
            d4 = ((double)n2 + block.minZ * 16.0) / 256.0;
            d3 = ((double)(n + 16) - block.minY * 16.0) / 256.0;
            d5 = ((double)n2 + block.maxZ * 16.0) / 256.0;
            d = d3;
            d6 = d2;
            d7 = d4;
            d8 = d5;
            d2 = d;
            d3 = d6;
            d7 = d5;
            d8 = d4;
        } else if (this.northFaceRotation == 3) {
            d2 = ((double)(n + 16) - block.minZ * 16.0) / 256.0;
            d3 = ((double)(n + 16) - block.maxZ * 16.0 - 0.01) / 256.0;
            d4 = ((double)n2 + block.maxY * 16.0) / 256.0;
            d5 = ((double)n2 + block.minY * 16.0 - 0.01) / 256.0;
            d = d3;
            d6 = d2;
            d7 = d4;
            d8 = d5;
        }
        double d9 = x + block.minX;
        double d10 = y + block.minY;
        double d11 = y + block.maxY;
        double d12 = z + block.minZ;
        double d13 = z + block.maxZ;
        if (this.useAo) {
            tessellator.color(this.firstVertexRed, this.firstVertexGreen, this.firstVertexBlue);
            tessellator.vertex(d9, d11, d13, d, d7);
            tessellator.color(this.secondVertexRed, this.secondVertexGreen, this.secondVertexBlue);
            tessellator.vertex(d9, d11, d12, d2, d4);
            tessellator.color(this.thirdVertexRed, this.thirdVertexGreen, this.thirdVertexBlue);
            tessellator.vertex(d9, d10, d12, d6, d8);
            tessellator.color(this.fourthVertexRed, this.fourthVertexGreen, this.fourthVertexBlue);
            tessellator.vertex(d9, d10, d13, d3, d5);
        } else {
            tessellator.vertex(d9, d11, d13, d, d7);
            tessellator.vertex(d9, d11, d12, d2, d4);
            tessellator.vertex(d9, d10, d12, d6, d8);
            tessellator.vertex(d9, d10, d13, d3, d5);
        }
    }

    public void renderSouthFace(Block block, double x, double y, double z, int texture) {
        double d;
        Tessellator tessellator = Tessellator.INSTANCE;
        if (this.textureOverride >= 0) {
            texture = this.textureOverride;
        }
        int n = (texture & 0xF) << 4;
        int n2 = texture & 0xF0;
        double d2 = ((double)n + block.minZ * 16.0) / 256.0;
        double d3 = ((double)n + block.maxZ * 16.0 - 0.01) / 256.0;
        double d4 = ((double)(n2 + 16) - block.maxY * 16.0) / 256.0;
        double d5 = ((double)(n2 + 16) - block.minY * 16.0 - 0.01) / 256.0;
        if (this.flipTextureHorizontally) {
            d = d2;
            d2 = d3;
            d3 = d;
        }
        if (block.minZ < 0.0 || block.maxZ > 1.0) {
            d2 = ((float)n + 0.0f) / 256.0f;
            d3 = ((float)n + 15.99f) / 256.0f;
        }
        if (block.minY < 0.0 || block.maxY > 1.0) {
            d4 = ((float)n2 + 0.0f) / 256.0f;
            d5 = ((float)n2 + 15.99f) / 256.0f;
        }
        d = d3;
        double d6 = d2;
        double d7 = d4;
        double d8 = d5;
        if (this.southFaceRotation == 2) {
            d2 = ((double)n + block.minY * 16.0) / 256.0;
            d4 = ((double)(n2 + 16) - block.minZ * 16.0) / 256.0;
            d3 = ((double)n + block.maxY * 16.0) / 256.0;
            d5 = ((double)(n2 + 16) - block.maxZ * 16.0) / 256.0;
            d = d3;
            d6 = d2;
            d7 = d4;
            d8 = d5;
            d = d2;
            d6 = d3;
            d4 = d5;
            d5 = d7;
        } else if (this.southFaceRotation == 1) {
            d2 = ((double)(n + 16) - block.maxY * 16.0) / 256.0;
            d4 = ((double)n2 + block.maxZ * 16.0) / 256.0;
            d3 = ((double)(n + 16) - block.minY * 16.0) / 256.0;
            d5 = ((double)n2 + block.minZ * 16.0) / 256.0;
            d = d3;
            d6 = d2;
            d7 = d4;
            d8 = d5;
            d2 = d;
            d3 = d6;
            d7 = d5;
            d8 = d4;
        } else if (this.southFaceRotation == 3) {
            d2 = ((double)(n + 16) - block.minZ * 16.0) / 256.0;
            d3 = ((double)(n + 16) - block.maxZ * 16.0 - 0.01) / 256.0;
            d4 = ((double)n2 + block.maxY * 16.0) / 256.0;
            d5 = ((double)n2 + block.minY * 16.0 - 0.01) / 256.0;
            d = d3;
            d6 = d2;
            d7 = d4;
            d8 = d5;
        }
        double d9 = x + block.maxX;
        double d10 = y + block.minY;
        double d11 = y + block.maxY;
        double d12 = z + block.minZ;
        double d13 = z + block.maxZ;
        if (this.useAo) {
            tessellator.color(this.firstVertexRed, this.firstVertexGreen, this.firstVertexBlue);
            tessellator.vertex(d9, d10, d13, d6, d8);
            tessellator.color(this.secondVertexRed, this.secondVertexGreen, this.secondVertexBlue);
            tessellator.vertex(d9, d10, d12, d3, d5);
            tessellator.color(this.thirdVertexRed, this.thirdVertexGreen, this.thirdVertexBlue);
            tessellator.vertex(d9, d11, d12, d, d7);
            tessellator.color(this.fourthVertexRed, this.fourthVertexGreen, this.fourthVertexBlue);
            tessellator.vertex(d9, d11, d13, d2, d4);
        } else {
            tessellator.vertex(d9, d10, d13, d6, d8);
            tessellator.vertex(d9, d10, d12, d3, d5);
            tessellator.vertex(d9, d11, d12, d, d7);
            tessellator.vertex(d9, d11, d13, d2, d4);
        }
    }

    public void render(Block block, int metadata, float brightness) {
        float f;
        float f2;
        int n;
        Tessellator tessellator = Tessellator.INSTANCE;
        this.useAo = false;
        if (this.inventoryColorEnabled) {
            n = block.getColor(metadata);
            f2 = (float)(n >> 16 & 0xFF) / 255.0f;
            f = (float)(n >> 8 & 0xFF) / 255.0f;
            float f3 = (float)(n & 0xFF) / 255.0f;
            GL11.glColor4f(f2 * brightness, f * brightness, f3 * brightness, 1.0f);
        }
        if ((n = block.getRenderType()) == 0 || n == 16) {
            if (n == 16) {
                metadata = 1;
            }
            block.setupRenderBoundingBox();
            GL11.glTranslatef(-0.5f, -0.5f, -0.5f);
            tessellator.startQuads();
            tessellator.normal(0.0f, -1.0f, 0.0f);
            this.renderBottomFace(block, 0.0, 0.0, 0.0, block.getTexture(0, metadata));
            tessellator.draw();
            tessellator.startQuads();
            tessellator.normal(0.0f, 1.0f, 0.0f);
            this.renderTopFace(block, 0.0, 0.0, 0.0, block.getTexture(1, metadata));
            tessellator.draw();
            tessellator.startQuads();
            tessellator.normal(0.0f, 0.0f, -1.0f);
            this.renderEastFace(block, 0.0, 0.0, 0.0, block.getTexture(2, metadata));
            tessellator.draw();
            tessellator.startQuads();
            tessellator.normal(0.0f, 0.0f, 1.0f);
            this.renderWestFace(block, 0.0, 0.0, 0.0, block.getTexture(3, metadata));
            tessellator.draw();
            tessellator.startQuads();
            tessellator.normal(-1.0f, 0.0f, 0.0f);
            this.renderNorthFace(block, 0.0, 0.0, 0.0, block.getTexture(4, metadata));
            tessellator.draw();
            tessellator.startQuads();
            tessellator.normal(1.0f, 0.0f, 0.0f);
            this.renderSouthFace(block, 0.0, 0.0, 0.0, block.getTexture(5, metadata));
            tessellator.draw();
            GL11.glTranslatef(0.5f, 0.5f, 0.5f);
        } else if (n == 1) {
            tessellator.startQuads();
            tessellator.normal(0.0f, -1.0f, 0.0f);
            this.renderCross(block, metadata, -0.5, -0.5, -0.5);
            tessellator.draw();
        } else if (n == 13) {
            block.setupRenderBoundingBox();
            GL11.glTranslatef(-0.5f, -0.5f, -0.5f);
            f2 = 0.0625f;
            tessellator.startQuads();
            tessellator.normal(0.0f, -1.0f, 0.0f);
            this.renderBottomFace(block, 0.0, 0.0, 0.0, block.getTexture(0));
            tessellator.draw();
            tessellator.startQuads();
            tessellator.normal(0.0f, 1.0f, 0.0f);
            this.renderTopFace(block, 0.0, 0.0, 0.0, block.getTexture(1));
            tessellator.draw();
            tessellator.startQuads();
            tessellator.normal(0.0f, 0.0f, -1.0f);
            tessellator.translate(0.0f, 0.0f, f2);
            this.renderEastFace(block, 0.0, 0.0, 0.0, block.getTexture(2));
            tessellator.translate(0.0f, 0.0f, -f2);
            tessellator.draw();
            tessellator.startQuads();
            tessellator.normal(0.0f, 0.0f, 1.0f);
            tessellator.translate(0.0f, 0.0f, -f2);
            this.renderWestFace(block, 0.0, 0.0, 0.0, block.getTexture(3));
            tessellator.translate(0.0f, 0.0f, f2);
            tessellator.draw();
            tessellator.startQuads();
            tessellator.normal(-1.0f, 0.0f, 0.0f);
            tessellator.translate(f2, 0.0f, 0.0f);
            this.renderNorthFace(block, 0.0, 0.0, 0.0, block.getTexture(4));
            tessellator.translate(-f2, 0.0f, 0.0f);
            tessellator.draw();
            tessellator.startQuads();
            tessellator.normal(1.0f, 0.0f, 0.0f);
            tessellator.translate(-f2, 0.0f, 0.0f);
            this.renderSouthFace(block, 0.0, 0.0, 0.0, block.getTexture(5));
            tessellator.translate(f2, 0.0f, 0.0f);
            tessellator.draw();
            GL11.glTranslatef(0.5f, 0.5f, 0.5f);
        } else if (n == 6) {
            tessellator.startQuads();
            tessellator.normal(0.0f, -1.0f, 0.0f);
            this.renderCrop(block, metadata, -0.5, -0.5, -0.5);
            tessellator.draw();
        } else if (n == 2) {
            tessellator.startQuads();
            tessellator.normal(0.0f, -1.0f, 0.0f);
            this.renderTiltedTorch(block, -0.5, -0.5, -0.5, 0.0, 0.0);
            tessellator.draw();
        } else if (n == 10) {
            for (int i = 0; i < 2; ++i) {
                if (i == 0) {
                    block.setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.5f);
                }
                if (i == 1) {
                    block.setBoundingBox(0.0f, 0.0f, 0.5f, 1.0f, 0.5f, 1.0f);
                }
                GL11.glTranslatef(-0.5f, -0.5f, -0.5f);
                tessellator.startQuads();
                tessellator.normal(0.0f, -1.0f, 0.0f);
                this.renderBottomFace(block, 0.0, 0.0, 0.0, block.getTexture(0));
                tessellator.draw();
                tessellator.startQuads();
                tessellator.normal(0.0f, 1.0f, 0.0f);
                this.renderTopFace(block, 0.0, 0.0, 0.0, block.getTexture(1));
                tessellator.draw();
                tessellator.startQuads();
                tessellator.normal(0.0f, 0.0f, -1.0f);
                this.renderEastFace(block, 0.0, 0.0, 0.0, block.getTexture(2));
                tessellator.draw();
                tessellator.startQuads();
                tessellator.normal(0.0f, 0.0f, 1.0f);
                this.renderWestFace(block, 0.0, 0.0, 0.0, block.getTexture(3));
                tessellator.draw();
                tessellator.startQuads();
                tessellator.normal(-1.0f, 0.0f, 0.0f);
                this.renderNorthFace(block, 0.0, 0.0, 0.0, block.getTexture(4));
                tessellator.draw();
                tessellator.startQuads();
                tessellator.normal(1.0f, 0.0f, 0.0f);
                this.renderSouthFace(block, 0.0, 0.0, 0.0, block.getTexture(5));
                tessellator.draw();
                GL11.glTranslatef(0.5f, 0.5f, 0.5f);
            }
        } else if (n == 11) {
            for (int i = 0; i < 4; ++i) {
                f = 0.125f;
                if (i == 0) {
                    block.setBoundingBox(0.5f - f, 0.0f, 0.0f, 0.5f + f, 1.0f, f * 2.0f);
                }
                if (i == 1) {
                    block.setBoundingBox(0.5f - f, 0.0f, 1.0f - f * 2.0f, 0.5f + f, 1.0f, 1.0f);
                }
                f = 0.0625f;
                if (i == 2) {
                    block.setBoundingBox(0.5f - f, 1.0f - f * 3.0f, -f * 2.0f, 0.5f + f, 1.0f - f, 1.0f + f * 2.0f);
                }
                if (i == 3) {
                    block.setBoundingBox(0.5f - f, 0.5f - f * 3.0f, -f * 2.0f, 0.5f + f, 0.5f - f, 1.0f + f * 2.0f);
                }
                GL11.glTranslatef(-0.5f, -0.5f, -0.5f);
                tessellator.startQuads();
                tessellator.normal(0.0f, -1.0f, 0.0f);
                this.renderBottomFace(block, 0.0, 0.0, 0.0, block.getTexture(0));
                tessellator.draw();
                tessellator.startQuads();
                tessellator.normal(0.0f, 1.0f, 0.0f);
                this.renderTopFace(block, 0.0, 0.0, 0.0, block.getTexture(1));
                tessellator.draw();
                tessellator.startQuads();
                tessellator.normal(0.0f, 0.0f, -1.0f);
                this.renderEastFace(block, 0.0, 0.0, 0.0, block.getTexture(2));
                tessellator.draw();
                tessellator.startQuads();
                tessellator.normal(0.0f, 0.0f, 1.0f);
                this.renderWestFace(block, 0.0, 0.0, 0.0, block.getTexture(3));
                tessellator.draw();
                tessellator.startQuads();
                tessellator.normal(-1.0f, 0.0f, 0.0f);
                this.renderNorthFace(block, 0.0, 0.0, 0.0, block.getTexture(4));
                tessellator.draw();
                tessellator.startQuads();
                tessellator.normal(1.0f, 0.0f, 0.0f);
                this.renderSouthFace(block, 0.0, 0.0, 0.0, block.getTexture(5));
                tessellator.draw();
                GL11.glTranslatef(0.5f, 0.5f, 0.5f);
            }
            block.setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
        }
    }

    public static boolean isSideLit(int renderType) {
        if (renderType == 0) {
            return true;
        }
        if (renderType == 13) {
            return true;
        }
        if (renderType == 10) {
            return true;
        }
        if (renderType == 11) {
            return true;
        }
        return renderType == 16;
    }
}

