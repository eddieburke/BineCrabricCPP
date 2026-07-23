/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.gui.screen.world;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Collections;
import java.util.Date;
import java.util.List;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.SingleplayerInteractionManager;
import net.minecraft.client.gui.screen.ConfirmScreen;
import net.minecraft.client.gui.screen.Screen;
import net.minecraft.client.gui.screen.world.CreateWorldScreen;
import net.minecraft.client.gui.screen.world.EditWorldScreen;
import net.minecraft.client.gui.widget.ButtonWidget;
import net.minecraft.client.gui.widget.EntryListWidget;
import net.minecraft.client.render.Tessellator;
import net.minecraft.client.resource.language.TranslationStorage;
import net.minecraft.util.math.MathHelper;
import net.minecraft.world.storage.WorldSaveInfo;
import net.minecraft.world.storage.WorldStorageSource;

@Environment(value=EnvType.CLIENT)
public class SelectWorldScreen
extends Screen {
    private final DateFormat dateFormat = new SimpleDateFormat();
    protected Screen parent;
    protected String title = "Select world";
    private boolean selected = false;
    private int selectedWorldId;
    private List saves;
    private WorldListWidget worldList;
    private String worldText;
    private String conversionText;
    private boolean isInChildScreen;
    private ButtonWidget renameWorldButton;
    private ButtonWidget playSelectedWorldButton;
    private ButtonWidget deleteWorldButton;

    public SelectWorldScreen(Screen parent) {
        this.parent = parent;
    }

    public void init() {
        TranslationStorage translationStorage = TranslationStorage.getInstance();
        this.title = translationStorage.get("selectWorld.title");
        this.worldText = translationStorage.get("selectWorld.world");
        this.conversionText = translationStorage.get("selectWorld.conversion");
        this.getSaves();
        this.worldList = new WorldListWidget();
        this.worldList.registerButtons(this.buttons, 4, 5);
        this.addButton();
    }

    private void getSaves() {
        WorldStorageSource worldStorageSource = this.minecraft.getWorldStorageSource();
        this.saves = worldStorageSource.getAll();
        Collections.sort(this.saves);
        this.selectedWorldId = -1;
    }

    protected String getSaveFileNames(int index) {
        return ((WorldSaveInfo)this.saves.get(index)).getSaveName();
    }

    protected String getWorldName(int index) {
        String string = ((WorldSaveInfo)this.saves.get(index)).getName();
        if (string == null || MathHelper.isNullOrEmpty(string)) {
            TranslationStorage translationStorage = TranslationStorage.getInstance();
            string = translationStorage.get("selectWorld.world") + " " + (index + 1);
        }
        return string;
    }

    public void addButton() {
        TranslationStorage translationStorage = TranslationStorage.getInstance();
        this.playSelectedWorldButton = new ButtonWidget(1, this.width / 2 - 154, this.height - 52, 150, 20, translationStorage.get("selectWorld.select"));
        this.buttons.add(this.playSelectedWorldButton);
        this.renameWorldButton = new ButtonWidget(6, this.width / 2 - 154, this.height - 28, 70, 20, translationStorage.get("selectWorld.rename"));
        this.buttons.add(this.renameWorldButton);
        this.deleteWorldButton = new ButtonWidget(2, this.width / 2 - 74, this.height - 28, 70, 20, translationStorage.get("selectWorld.delete"));
        this.buttons.add(this.deleteWorldButton);
        this.buttons.add(new ButtonWidget(3, this.width / 2 + 4, this.height - 52, 150, 20, translationStorage.get("selectWorld.create")));
        this.buttons.add(new ButtonWidget(0, this.width / 2 + 4, this.height - 28, 150, 20, translationStorage.get("gui.cancel")));
        this.playSelectedWorldButton.active = false;
        this.renameWorldButton.active = false;
        this.deleteWorldButton.active = false;
    }

    protected void buttonClicked(ButtonWidget button) {
        if (!button.active) {
            return;
        }
        if (button.id == 2) {
            String string = this.getWorldName(this.selectedWorldId);
            if (string != null) {
                this.isInChildScreen = true;
                TranslationStorage translationStorage = TranslationStorage.getInstance();
                String string2 = translationStorage.get("selectWorld.deleteQuestion");
                String string3 = "'" + string + "' " + translationStorage.get("selectWorld.deleteWarning");
                String string4 = translationStorage.get("selectWorld.deleteButton");
                String string5 = translationStorage.get("gui.cancel");
                ConfirmScreen confirmScreen = new ConfirmScreen(this, string2, string3, string4, string5, this.selectedWorldId);
                this.minecraft.setScreen(confirmScreen);
            }
        } else if (button.id == 1) {
            this.selectWorld(this.selectedWorldId);
        } else if (button.id == 3) {
            this.minecraft.setScreen(new CreateWorldScreen(this));
        } else if (button.id == 6) {
            this.minecraft.setScreen(new EditWorldScreen(this, this.getSaveFileNames(this.selectedWorldId)));
        } else if (button.id == 0) {
            this.minecraft.setScreen(this.parent);
        } else {
            this.worldList.buttonClicked(button);
        }
    }

    public void selectWorld(int id) {
        this.minecraft.setScreen(null);
        if (this.selected) {
            return;
        }
        this.selected = true;
        this.minecraft.interactionManager = new SingleplayerInteractionManager(this.minecraft);
        String string = this.getSaveFileNames(id);
        if (string == null) {
            string = "World" + id;
        }
        this.minecraft.startGame(string, this.getWorldName(id), 0L);
        this.minecraft.setScreen(null);
    }

    public void confirmed(boolean confirmed, int id) {
        if (this.isInChildScreen) {
            this.isInChildScreen = false;
            if (confirmed) {
                WorldStorageSource worldStorageSource = this.minecraft.getWorldStorageSource();
                worldStorageSource.flush();
                worldStorageSource.delete(this.getSaveFileNames(id));
                this.getSaves();
            }
            this.minecraft.setScreen(this);
        }
    }

    public void render(int mouseX, int mouseY, float delta) {
        this.worldList.render(mouseX, mouseY, delta);
        this.drawCenteredTextWithShadow(this.textRenderer, this.title, this.width / 2, 20, 0xFFFFFF);
        super.render(mouseX, mouseY, delta);
    }

    @Environment(value=EnvType.CLIENT)
    class WorldListWidget
    extends EntryListWidget {
        public WorldListWidget() {
            super(SelectWorldScreen.this.minecraft, SelectWorldScreen.this.width, SelectWorldScreen.this.height, 32, SelectWorldScreen.this.height - 64, 36);
        }

        protected int getEntryCount() {
            return SelectWorldScreen.this.saves.size();
        }

        protected void entryClicked(int index, boolean doubleClick) {
            boolean bl;
            SelectWorldScreen.this.selectedWorldId = index;
            ((SelectWorldScreen)SelectWorldScreen.this).playSelectedWorldButton.active = bl = SelectWorldScreen.this.selectedWorldId >= 0 && SelectWorldScreen.this.selectedWorldId < this.getEntryCount();
            ((SelectWorldScreen)SelectWorldScreen.this).renameWorldButton.active = bl;
            ((SelectWorldScreen)SelectWorldScreen.this).deleteWorldButton.active = bl;
            if (doubleClick && bl) {
                SelectWorldScreen.this.selectWorld(index);
            }
        }

        protected boolean isSelectedEntry(int index) {
            return index == SelectWorldScreen.this.selectedWorldId;
        }

        protected int getEntriesHeight() {
            return SelectWorldScreen.this.saves.size() * 36;
        }

        protected void renderBackground() {
            SelectWorldScreen.this.renderBackground();
        }

        protected void renderEntry(int index, int x, int y, int l, Tessellator tessellator) {
            WorldSaveInfo worldSaveInfo = (WorldSaveInfo)SelectWorldScreen.this.saves.get(index);
            String string = worldSaveInfo.getName();
            if (string == null || MathHelper.isNullOrEmpty(string)) {
                string = SelectWorldScreen.this.worldText + " " + (index + 1);
            }
            String string2 = worldSaveInfo.getSaveName();
            string2 = string2 + " (" + SelectWorldScreen.this.dateFormat.format(new Date(worldSaveInfo.getLastPlayed()));
            long l2 = worldSaveInfo.getSize();
            string2 = string2 + ", " + (float)(l2 / 1024L * 100L / 1024L) / 100.0f + " MB)";
            String string3 = "";
            if (worldSaveInfo.isSameVersion()) {
                string3 = SelectWorldScreen.this.conversionText + " " + string3;
            }
            SelectWorldScreen.this.drawTextWithShadow(SelectWorldScreen.this.textRenderer, string, x + 2, y + 1, 0xFFFFFF);
            SelectWorldScreen.this.drawTextWithShadow(SelectWorldScreen.this.textRenderer, string2, x + 2, y + 12, 0x808080);
            SelectWorldScreen.this.drawTextWithShadow(SelectWorldScreen.this.textRenderer, string3, x + 2, y + 12 + 10, 0x808080);
        }
    }
}

