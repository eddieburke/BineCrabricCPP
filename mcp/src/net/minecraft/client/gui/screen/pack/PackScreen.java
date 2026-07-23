/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.gui.screen.pack;

import java.io.File;
import java.util.List;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.Minecraft;
import net.minecraft.client.gui.screen.Screen;
import net.minecraft.client.gui.widget.ButtonWidget;
import net.minecraft.client.gui.widget.EntryListWidget;
import net.minecraft.client.gui.widget.OptionButtonWidget;
import net.minecraft.client.render.Tessellator;
import net.minecraft.client.resource.language.TranslationStorage;
import net.minecraft.client.resource.pack.TexturePack;
import org.lwjgl.Sys;
import org.lwjgl.opengl.GL11;

@Environment(value=EnvType.CLIENT)
public class PackScreen
extends Screen {
    protected Screen parent;
    private int cooldown = -1;
    private String texturePacksDir = "";
    private PackListWidget texturePacks;

    public PackScreen(Screen parent) {
        this.parent = parent;
    }

    public void init() {
        TranslationStorage translationStorage = TranslationStorage.getInstance();
        this.buttons.add(new OptionButtonWidget(5, this.width / 2 - 154, this.height - 48, translationStorage.get("texturePack.openFolder")));
        this.buttons.add(new OptionButtonWidget(6, this.width / 2 + 4, this.height - 48, translationStorage.get("gui.done")));
        this.minecraft.texturePacks.reload();
        this.texturePacksDir = new File(Minecraft.getRunDirectory(), "texturepacks").getAbsolutePath();
        this.texturePacks = new PackListWidget();
        this.texturePacks.registerButtons(this.buttons, 7, 8);
    }

    protected void buttonClicked(ButtonWidget button) {
        if (!button.active) {
            return;
        }
        if (button.id == 5) {
            Sys.openURL("file://" + this.texturePacksDir);
        } else if (button.id == 6) {
            this.minecraft.textureManager.reload();
            this.minecraft.setScreen(this.parent);
        } else {
            this.texturePacks.buttonClicked(button);
        }
    }

    protected void mouseClicked(int mouseX, int mouseY, int button) {
        super.mouseClicked(mouseX, mouseY, button);
    }

    protected void mouseReleased(int mouseX, int mouseY, int button) {
        super.mouseReleased(mouseX, mouseY, button);
    }

    public void render(int mouseX, int mouseY, float delta) {
        this.texturePacks.render(mouseX, mouseY, delta);
        if (this.cooldown <= 0) {
            this.minecraft.texturePacks.reload();
            this.cooldown += 20;
        }
        TranslationStorage translationStorage = TranslationStorage.getInstance();
        this.drawCenteredTextWithShadow(this.textRenderer, translationStorage.get("texturePack.title"), this.width / 2, 16, 0xFFFFFF);
        this.drawCenteredTextWithShadow(this.textRenderer, translationStorage.get("texturePack.folderInfo"), this.width / 2 - 77, this.height - 26, 0x808080);
        super.render(mouseX, mouseY, delta);
    }

    public void tick() {
        super.tick();
        --this.cooldown;
    }

    @Environment(value=EnvType.CLIENT)
    class PackListWidget
    extends EntryListWidget {
        public PackListWidget() {
            super(PackScreen.this.minecraft, PackScreen.this.width, PackScreen.this.height, 32, PackScreen.this.height - 55 + 4, 36);
        }

        protected int getEntryCount() {
            List list = ((PackScreen)PackScreen.this).minecraft.texturePacks.getAvailable();
            return list.size();
        }

        protected void entryClicked(int index, boolean doubleClick) {
            List list = ((PackScreen)PackScreen.this).minecraft.texturePacks.getAvailable();
            ((PackScreen)PackScreen.this).minecraft.texturePacks.select((TexturePack)list.get(index));
            ((PackScreen)PackScreen.this).minecraft.textureManager.reload();
        }

        protected boolean isSelectedEntry(int index) {
            List list = ((PackScreen)PackScreen.this).minecraft.texturePacks.getAvailable();
            return ((PackScreen)PackScreen.this).minecraft.texturePacks.selected == list.get(index);
        }

        protected int getEntriesHeight() {
            return this.getEntryCount() * 36;
        }

        protected void renderBackground() {
            PackScreen.this.renderBackground();
        }

        protected void renderEntry(int index, int x, int y, int l, Tessellator tessellator) {
            TexturePack texturePack = (TexturePack)((PackScreen)PackScreen.this).minecraft.texturePacks.getAvailable().get(index);
            texturePack.bindIcon(PackScreen.this.minecraft);
            GL11.glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
            tessellator.startQuads();
            tessellator.color(0xFFFFFF);
            tessellator.vertex(x, y + l, 0.0, 0.0, 1.0);
            tessellator.vertex(x + 32, y + l, 0.0, 1.0, 1.0);
            tessellator.vertex(x + 32, y, 0.0, 1.0, 0.0);
            tessellator.vertex(x, y, 0.0, 0.0, 0.0);
            tessellator.draw();
            PackScreen.this.drawTextWithShadow(PackScreen.this.textRenderer, texturePack.name, x + 32 + 2, y + 1, 0xFFFFFF);
            PackScreen.this.drawTextWithShadow(PackScreen.this.textRenderer, texturePack.descriptionLine1, x + 32 + 2, y + 12, 0x808080);
            PackScreen.this.drawTextWithShadow(PackScreen.this.textRenderer, texturePack.descriptionLine2, x + 32 + 2, y + 12 + 10, 0x808080);
        }
    }
}

