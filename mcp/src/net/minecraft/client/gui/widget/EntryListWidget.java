/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.gui.widget;

import java.util.List;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.Minecraft;
import net.minecraft.client.gui.widget.ButtonWidget;
import net.minecraft.client.render.Tessellator;
import org.lwjgl.input.Mouse;
import org.lwjgl.opengl.GL11;

@Environment(value=EnvType.CLIENT)
public abstract class EntryListWidget {
    private final Minecraft minecraft;
    private final int width;
    private final int height;
    protected final int top;
    protected final int bottom;
    private final int right;
    private final int left;
    protected final int itemHeight;
    private int scrollUpButtonId;
    private int scrollDownButtonId;
    private float mostYStart = -2.0f;
    private float scrollSpeedMultiplier;
    private float scrollAmount;
    private int pos = -1;
    private long time = 0L;
    private boolean renderSelectionHighlight = true;
    private boolean renderHeader;
    private int headerHeight;

    public EntryListWidget(Minecraft minecraft, int width, int height, int top, int bottom, int itemHeight) {
        this.minecraft = minecraft;
        this.width = width;
        this.height = height;
        this.top = top;
        this.bottom = bottom;
        this.itemHeight = itemHeight;
        this.left = 0;
        this.right = width;
    }

    public void setRenderSelectionHighlight(boolean renderSelectionHighlight) {
        this.renderSelectionHighlight = renderSelectionHighlight;
    }

    protected void setHeader(boolean renderHeader, int headerHeight) {
        this.renderHeader = renderHeader;
        this.headerHeight = headerHeight;
        if (!renderHeader) {
            this.headerHeight = 0;
        }
    }

    protected abstract int getEntryCount();

    protected abstract void entryClicked(int var1, boolean var2);

    protected abstract boolean isSelectedEntry(int var1);

    protected int getEntriesHeight() {
        return this.getEntryCount() * this.itemHeight + this.headerHeight;
    }

    protected abstract void renderBackground();

    protected abstract void renderEntry(int var1, int var2, int var3, int var4, Tessellator var5);

    protected void renderHeader(int x, int y, Tessellator tessellator) {
    }

    protected void headerClicked(int x, int y) {
    }

    protected void renderDecorations(int mouseX, int mouseY) {
    }

    public int getEntryAt(int x, int y) {
        int n = this.width / 2 - 110;
        int n2 = this.width / 2 + 110;
        int n3 = y - this.top - this.headerHeight + (int)this.scrollAmount - 4;
        int n4 = n3 / this.itemHeight;
        if (x >= n && x <= n2 && n4 >= 0 && n3 >= 0 && n4 < this.getEntryCount()) {
            return n4;
        }
        return -1;
    }

    public void registerButtons(List buttons, int scrollUp, int scrollDown) {
        this.scrollUpButtonId = scrollUp;
        this.scrollDownButtonId = scrollDown;
    }

    private void clampScrolling() {
        int n = this.getEntriesHeight() - (this.bottom - this.top - 4);
        if (n < 0) {
            n /= 2;
        }
        if (this.scrollAmount < 0.0f) {
            this.scrollAmount = 0.0f;
        }
        if (this.scrollAmount > (float)n) {
            this.scrollAmount = n;
        }
    }

    public void buttonClicked(ButtonWidget button) {
        if (!button.active) {
            return;
        }
        if (button.id == this.scrollUpButtonId) {
            this.scrollAmount -= (float)(this.itemHeight * 2 / 3);
            this.mostYStart = -2.0f;
            this.clampScrolling();
        } else if (button.id == this.scrollDownButtonId) {
            this.scrollAmount += (float)(this.itemHeight * 2 / 3);
            this.mostYStart = -2.0f;
            this.clampScrolling();
        }
    }

    public void render(int mouseX, int mouseY, float f) {
        int n;
        int n2;
        int n3;
        int n4;
        int n5;
        int n6;
        this.renderBackground();
        int n7 = this.getEntryCount();
        int n8 = this.width / 2 + 124;
        int n9 = n8 + 6;
        if (Mouse.isButtonDown(0)) {
            if (this.mostYStart == -1.0f) {
                boolean bl = true;
                if (mouseY >= this.top && mouseY <= this.bottom) {
                    int n10 = this.width / 2 - 110;
                    n6 = this.width / 2 + 110;
                    n5 = mouseY - this.top - this.headerHeight + (int)this.scrollAmount - 4;
                    n4 = n5 / this.itemHeight;
                    if (mouseX >= n10 && mouseX <= n6 && n4 >= 0 && n5 >= 0 && n4 < n7) {
                        n3 = n4 == this.pos && System.currentTimeMillis() - this.time < 250L ? 1 : 0;
                        this.entryClicked(n4, n3 != 0);
                        this.pos = n4;
                        this.time = System.currentTimeMillis();
                    } else if (mouseX >= n10 && mouseX <= n6 && n5 < 0) {
                        this.headerClicked(mouseX - n10, mouseY - this.top + (int)this.scrollAmount - 4);
                        bl = false;
                    }
                    if (mouseX >= n8 && mouseX <= n9) {
                        this.scrollSpeedMultiplier = -1.0f;
                        n3 = this.getEntriesHeight() - (this.bottom - this.top - 4);
                        if (n3 < 1) {
                            n3 = 1;
                        }
                        if ((n2 = (int)((float)((this.bottom - this.top) * (this.bottom - this.top)) / (float)this.getEntriesHeight())) < 32) {
                            n2 = 32;
                        }
                        if (n2 > this.bottom - this.top - 8) {
                            n2 = this.bottom - this.top - 8;
                        }
                        this.scrollSpeedMultiplier /= (float)(this.bottom - this.top - n2) / (float)n3;
                    } else {
                        this.scrollSpeedMultiplier = 1.0f;
                    }
                    this.mostYStart = bl ? (float)mouseY : -2.0f;
                } else {
                    this.mostYStart = -2.0f;
                }
            } else if (this.mostYStart >= 0.0f) {
                this.scrollAmount -= ((float)mouseY - this.mostYStart) * this.scrollSpeedMultiplier;
                this.mostYStart = mouseY;
            }
        } else {
            this.mostYStart = -1.0f;
        }
        this.clampScrolling();
        GL11.glDisable(2896);
        GL11.glDisable(2912);
        Tessellator tessellator = Tessellator.INSTANCE;
        GL11.glBindTexture(3553, this.minecraft.textureManager.getTextureId("/gui/background.png"));
        GL11.glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        float f2 = 32.0f;
        tessellator.startQuads();
        tessellator.color(0x202020);
        tessellator.vertex(this.left, this.bottom, 0.0, (float)this.left / f2, (float)(this.bottom + (int)this.scrollAmount) / f2);
        tessellator.vertex(this.right, this.bottom, 0.0, (float)this.right / f2, (float)(this.bottom + (int)this.scrollAmount) / f2);
        tessellator.vertex(this.right, this.top, 0.0, (float)this.right / f2, (float)(this.top + (int)this.scrollAmount) / f2);
        tessellator.vertex(this.left, this.top, 0.0, (float)this.left / f2, (float)(this.top + (int)this.scrollAmount) / f2);
        tessellator.draw();
        n6 = this.width / 2 - 92 - 16;
        n5 = this.top + 4 - (int)this.scrollAmount;
        if (this.renderHeader) {
            this.renderHeader(n6, n5, tessellator);
        }
        for (n4 = 0; n4 < n7; ++n4) {
            n3 = n5 + n4 * this.itemHeight + this.headerHeight;
            n2 = this.itemHeight - 4;
            if (n3 > this.bottom || n3 + n2 < this.top) continue;
            if (this.renderSelectionHighlight && this.isSelectedEntry(n4)) {
                n = this.width / 2 - 110;
                int n11 = this.width / 2 + 110;
                GL11.glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
                GL11.glDisable(3553);
                tessellator.startQuads();
                tessellator.color(0x808080);
                tessellator.vertex(n, n3 + n2 + 2, 0.0, 0.0, 1.0);
                tessellator.vertex(n11, n3 + n2 + 2, 0.0, 1.0, 1.0);
                tessellator.vertex(n11, n3 - 2, 0.0, 1.0, 0.0);
                tessellator.vertex(n, n3 - 2, 0.0, 0.0, 0.0);
                tessellator.color(0);
                tessellator.vertex(n + 1, n3 + n2 + 1, 0.0, 0.0, 1.0);
                tessellator.vertex(n11 - 1, n3 + n2 + 1, 0.0, 1.0, 1.0);
                tessellator.vertex(n11 - 1, n3 - 1, 0.0, 1.0, 0.0);
                tessellator.vertex(n + 1, n3 - 1, 0.0, 0.0, 0.0);
                tessellator.draw();
                GL11.glEnable(3553);
            }
            this.renderEntry(n4, n6, n3, n2, tessellator);
        }
        GL11.glDisable(2929);
        n4 = 4;
        this.renderBackground(0, this.top, 255, 255);
        this.renderBackground(this.bottom, this.height, 255, 255);
        GL11.glEnable(3042);
        GL11.glBlendFunc(770, 771);
        GL11.glDisable(3008);
        GL11.glShadeModel(7425);
        GL11.glDisable(3553);
        tessellator.startQuads();
        tessellator.color(0, 0);
        tessellator.vertex(this.left, this.top + n4, 0.0, 0.0, 1.0);
        tessellator.vertex(this.right, this.top + n4, 0.0, 1.0, 1.0);
        tessellator.color(0, 255);
        tessellator.vertex(this.right, this.top, 0.0, 1.0, 0.0);
        tessellator.vertex(this.left, this.top, 0.0, 0.0, 0.0);
        tessellator.draw();
        tessellator.startQuads();
        tessellator.color(0, 255);
        tessellator.vertex(this.left, this.bottom, 0.0, 0.0, 1.0);
        tessellator.vertex(this.right, this.bottom, 0.0, 1.0, 1.0);
        tessellator.color(0, 0);
        tessellator.vertex(this.right, this.bottom - n4, 0.0, 1.0, 0.0);
        tessellator.vertex(this.left, this.bottom - n4, 0.0, 0.0, 0.0);
        tessellator.draw();
        n3 = this.getEntriesHeight() - (this.bottom - this.top - 4);
        if (n3 > 0) {
            n2 = (this.bottom - this.top) * (this.bottom - this.top) / this.getEntriesHeight();
            if (n2 < 32) {
                n2 = 32;
            }
            if (n2 > this.bottom - this.top - 8) {
                n2 = this.bottom - this.top - 8;
            }
            if ((n = (int)this.scrollAmount * (this.bottom - this.top - n2) / n3 + this.top) < this.top) {
                n = this.top;
            }
            tessellator.startQuads();
            tessellator.color(0, 255);
            tessellator.vertex(n8, this.bottom, 0.0, 0.0, 1.0);
            tessellator.vertex(n9, this.bottom, 0.0, 1.0, 1.0);
            tessellator.vertex(n9, this.top, 0.0, 1.0, 0.0);
            tessellator.vertex(n8, this.top, 0.0, 0.0, 0.0);
            tessellator.draw();
            tessellator.startQuads();
            tessellator.color(0x808080, 255);
            tessellator.vertex(n8, n + n2, 0.0, 0.0, 1.0);
            tessellator.vertex(n9, n + n2, 0.0, 1.0, 1.0);
            tessellator.vertex(n9, n, 0.0, 1.0, 0.0);
            tessellator.vertex(n8, n, 0.0, 0.0, 0.0);
            tessellator.draw();
            tessellator.startQuads();
            tessellator.color(0xC0C0C0, 255);
            tessellator.vertex(n8, n + n2 - 1, 0.0, 0.0, 1.0);
            tessellator.vertex(n9 - 1, n + n2 - 1, 0.0, 1.0, 1.0);
            tessellator.vertex(n9 - 1, n, 0.0, 1.0, 0.0);
            tessellator.vertex(n8, n, 0.0, 0.0, 0.0);
            tessellator.draw();
        }
        this.renderDecorations(mouseX, mouseY);
        GL11.glEnable(3553);
        GL11.glShadeModel(7424);
        GL11.glEnable(3008);
        GL11.glDisable(3042);
    }

    private void renderBackground(int i, int j, int k, int l) {
        Tessellator tessellator = Tessellator.INSTANCE;
        GL11.glBindTexture(3553, this.minecraft.textureManager.getTextureId("/gui/background.png"));
        GL11.glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        float f = 32.0f;
        tessellator.startQuads();
        tessellator.color(0x404040, l);
        tessellator.vertex(0.0, j, 0.0, 0.0, (float)j / f);
        tessellator.vertex(this.width, j, 0.0, (float)this.width / f, (float)j / f);
        tessellator.color(0x404040, k);
        tessellator.vertex(this.width, i, 0.0, (float)this.width / f, (float)i / f);
        tessellator.vertex(0.0, i, 0.0, 0.0, (float)i / f);
        tessellator.draw();
    }
}

