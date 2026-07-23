/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.gui.screen;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.gui.screen.Screen;
import net.minecraft.client.gui.widget.ButtonWidget;
import net.minecraft.client.gui.widget.EntryListWidget;
import net.minecraft.client.render.Tessellator;
import net.minecraft.client.render.item.ItemRenderer;
import net.minecraft.client.render.platform.Lighting;
import net.minecraft.client.resource.language.I18n;
import net.minecraft.client.resource.language.TranslationStorage;
import net.minecraft.item.Item;
import net.minecraft.stat.ItemOrBlockStat;
import net.minecraft.stat.PlayerStats;
import net.minecraft.stat.Stat;
import net.minecraft.stat.Stats;
import org.lwjgl.input.Mouse;
import org.lwjgl.opengl.GL11;

@Environment(value=EnvType.CLIENT)
public class StatsScreen
extends Screen {
    private static ItemRenderer itemRenderer = new ItemRenderer();
    protected Screen parent;
    protected String title = "Select world";
    private GeneralStatsListWidget generalStats;
    private ItemStatsListWidget itemStats;
    private BlockStatsListWidget blockStats;
    private PlayerStats stats;
    private EntryListWidget selectedStatsList = null;

    public StatsScreen(Screen parent, PlayerStats stats) {
        this.parent = parent;
        this.stats = stats;
    }

    public void init() {
        this.title = I18n.getTranslation("gui.stats");
        this.generalStats = new GeneralStatsListWidget();
        this.generalStats.registerButtons(this.buttons, 1, 1);
        this.itemStats = new ItemStatsListWidget();
        this.itemStats.registerButtons(this.buttons, 1, 1);
        this.blockStats = new BlockStatsListWidget();
        this.blockStats.registerButtons(this.buttons, 1, 1);
        this.selectedStatsList = this.generalStats;
        this.createButtons();
    }

    public void createButtons() {
        TranslationStorage translationStorage = TranslationStorage.getInstance();
        this.buttons.add(new ButtonWidget(0, this.width / 2 + 4, this.height - 28, 150, 20, translationStorage.get("gui.done")));
        this.buttons.add(new ButtonWidget(1, this.width / 2 - 154, this.height - 52, 100, 20, translationStorage.get("stat.generalButton")));
        ButtonWidget buttonWidget = new ButtonWidget(2, this.width / 2 - 46, this.height - 52, 100, 20, translationStorage.get("stat.blocksButton"));
        this.buttons.add(buttonWidget);
        ButtonWidget buttonWidget2 = new ButtonWidget(3, this.width / 2 + 62, this.height - 52, 100, 20, translationStorage.get("stat.itemsButton"));
        this.buttons.add(buttonWidget2);
        if (this.blockStats.getEntryCount() == 0) {
            buttonWidget.active = false;
        }
        if (this.itemStats.getEntryCount() == 0) {
            buttonWidget2.active = false;
        }
    }

    protected void buttonClicked(ButtonWidget button) {
        if (!button.active) {
            return;
        }
        if (button.id == 0) {
            this.minecraft.setScreen(this.parent);
        } else if (button.id == 1) {
            this.selectedStatsList = this.generalStats;
        } else if (button.id == 3) {
            this.selectedStatsList = this.itemStats;
        } else if (button.id == 2) {
            this.selectedStatsList = this.blockStats;
        } else {
            this.selectedStatsList.buttonClicked(button);
        }
    }

    public void render(int mouseX, int mouseY, float delta) {
        this.selectedStatsList.render(mouseX, mouseY, delta);
        this.drawCenteredTextWithShadow(this.textRenderer, this.title, this.width / 2, 20, 0xFFFFFF);
        super.render(mouseX, mouseY, delta);
    }

    private void render(int mouseX, int mouseY, int tickDelta) {
        this.renderIcon(mouseX + 1, mouseY + 1);
        GL11.glEnable(32826);
        GL11.glPushMatrix();
        GL11.glRotatef(180.0f, 1.0f, 0.0f, 0.0f);
        Lighting.turnOn();
        GL11.glPopMatrix();
        itemRenderer.renderGuiItem(this.textRenderer, this.minecraft.textureManager, tickDelta, 0, Item.ITEMS[tickDelta].getTextureId(0), mouseX + 2, mouseY + 2);
        Lighting.turnOff();
        GL11.glDisable(32826);
    }

    private void renderIcon(int x, int y) {
        this.renderIcon(x, y, 0, 0);
    }

    private void renderIcon(int x, int y, int u, int v) {
        int n = this.minecraft.textureManager.getTextureId("/gui/slot.png");
        GL11.glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        this.minecraft.textureManager.bindTexture(n);
        Tessellator tessellator = Tessellator.INSTANCE;
        tessellator.startQuads();
        tessellator.vertex(x + 0, y + 18, this.zOffset, (float)(u + 0) * 0.0078125f, (float)(v + 18) * 0.0078125f);
        tessellator.vertex(x + 18, y + 18, this.zOffset, (float)(u + 18) * 0.0078125f, (float)(v + 18) * 0.0078125f);
        tessellator.vertex(x + 18, y + 0, this.zOffset, (float)(u + 18) * 0.0078125f, (float)(v + 0) * 0.0078125f);
        tessellator.vertex(x + 0, y + 0, this.zOffset, (float)(u + 0) * 0.0078125f, (float)(v + 0) * 0.0078125f);
        tessellator.draw();
    }

    @Environment(value=EnvType.CLIENT)
    class GeneralStatsListWidget
    extends EntryListWidget {
        public GeneralStatsListWidget() {
            super(StatsScreen.this.minecraft, StatsScreen.this.width, StatsScreen.this.height, 32, StatsScreen.this.height - 64, 10);
            this.setRenderSelectionHighlight(false);
        }

        protected int getEntryCount() {
            return Stats.GENERAL_STATS.size();
        }

        protected void entryClicked(int index, boolean doubleClick) {
        }

        protected boolean isSelectedEntry(int index) {
            return false;
        }

        protected int getEntriesHeight() {
            return this.getEntryCount() * 10;
        }

        protected void renderBackground() {
            StatsScreen.this.renderBackground();
        }

        protected void renderEntry(int index, int x, int y, int l, Tessellator tessellator) {
            Stat stat = (Stat)Stats.GENERAL_STATS.get(index);
            StatsScreen.this.drawTextWithShadow(StatsScreen.this.textRenderer, stat.stringId, x + 2, y + 1, index % 2 == 0 ? 0xFFFFFF : 0x909090);
            String string = stat.format(StatsScreen.this.stats.get(stat));
            StatsScreen.this.drawTextWithShadow(StatsScreen.this.textRenderer, string, x + 2 + 213 - StatsScreen.this.textRenderer.getWidth(string), y + 1, index % 2 == 0 ? 0xFFFFFF : 0x909090);
        }
    }

    @Environment(value=EnvType.CLIENT)
    class ItemStatsListWidget
    extends AbstractStatsListWidget {
        public ItemStatsListWidget() {
            this.entries = new ArrayList();
            for (ItemOrBlockStat itemOrBlockStat : Stats.ITEM_STATS) {
                boolean bl = false;
                int n = itemOrBlockStat.getItemOrBlockId();
                if (StatsScreen.this.stats.get(itemOrBlockStat) > 0) {
                    bl = true;
                } else if (Stats.BROKEN[n] != null && StatsScreen.this.stats.get(Stats.BROKEN[n]) > 0) {
                    bl = true;
                } else if (Stats.CRAFTED[n] != null && StatsScreen.this.stats.get(Stats.CRAFTED[n]) > 0) {
                    bl = true;
                }
                if (!bl) continue;
                this.entries.add(itemOrBlockStat);
            }
            this.statComparator = new Comparator(){

                public int compare(ItemOrBlockStat arg, ItemOrBlockStat arg2) {
                    int n = arg.getItemOrBlockId();
                    int n2 = arg2.getItemOrBlockId();
                    Stat stat = null;
                    Stat stat2 = null;
                    if (ItemStatsListWidget.this.selectedTab == 0) {
                        stat = Stats.BROKEN[n];
                        stat2 = Stats.BROKEN[n2];
                    } else if (ItemStatsListWidget.this.selectedTab == 1) {
                        stat = Stats.CRAFTED[n];
                        stat2 = Stats.CRAFTED[n2];
                    } else if (ItemStatsListWidget.this.selectedTab == 2) {
                        stat = Stats.USED[n];
                        stat2 = Stats.USED[n2];
                    }
                    if (stat != null || stat2 != null) {
                        int n3;
                        if (stat == null) {
                            return 1;
                        }
                        if (stat2 == null) {
                            return -1;
                        }
                        int n4 = StatsScreen.this.stats.get(stat);
                        if (n4 != (n3 = StatsScreen.this.stats.get(stat2))) {
                            return (n4 - n3) * ItemStatsListWidget.this.statSortOrder;
                        }
                    }
                    return n - n2;
                }
            };
        }

        protected void renderHeader(int x, int y, Tessellator tessellator) {
            super.renderHeader(x, y, tessellator);
            if (this.clickedIconId == 0) {
                StatsScreen.this.renderIcon(x + 115 - 18 + 1, y + 1 + 1, 72, 18);
            } else {
                StatsScreen.this.renderIcon(x + 115 - 18, y + 1, 72, 18);
            }
            if (this.clickedIconId == 1) {
                StatsScreen.this.renderIcon(x + 165 - 18 + 1, y + 1 + 1, 18, 18);
            } else {
                StatsScreen.this.renderIcon(x + 165 - 18, y + 1, 18, 18);
            }
            if (this.clickedIconId == 2) {
                StatsScreen.this.renderIcon(x + 215 - 18 + 1, y + 1 + 1, 36, 18);
            } else {
                StatsScreen.this.renderIcon(x + 215 - 18, y + 1, 36, 18);
            }
        }

        protected void renderEntry(int index, int x, int y, int l, Tessellator tessellator) {
            ItemOrBlockStat itemOrBlockStat = this.getEntry(index);
            int n = itemOrBlockStat.getItemOrBlockId();
            StatsScreen.this.render(x + 40, y, n);
            this.renderStat((ItemOrBlockStat)Stats.BROKEN[n], x + 115, y, index % 2 == 0);
            this.renderStat((ItemOrBlockStat)Stats.CRAFTED[n], x + 165, y, index % 2 == 0);
            this.renderStat(itemOrBlockStat, x + 215, y, index % 2 == 0);
        }

        protected String getColumnHeader(int column) {
            if (column == 1) {
                return "stat.crafted";
            }
            if (column == 2) {
                return "stat.used";
            }
            return "stat.depleted";
        }
    }

    @Environment(value=EnvType.CLIENT)
    class BlockStatsListWidget
    extends AbstractStatsListWidget {
        public BlockStatsListWidget() {
            this.entries = new ArrayList();
            for (ItemOrBlockStat itemOrBlockStat : Stats.BLOCK_MINED_STATS) {
                boolean bl = false;
                int n = itemOrBlockStat.getItemOrBlockId();
                if (StatsScreen.this.stats.get(itemOrBlockStat) > 0) {
                    bl = true;
                } else if (Stats.USED[n] != null && StatsScreen.this.stats.get(Stats.USED[n]) > 0) {
                    bl = true;
                } else if (Stats.CRAFTED[n] != null && StatsScreen.this.stats.get(Stats.CRAFTED[n]) > 0) {
                    bl = true;
                }
                if (!bl) continue;
                this.entries.add(itemOrBlockStat);
            }
            this.statComparator = new Comparator(){

                public int compare(ItemOrBlockStat arg, ItemOrBlockStat arg2) {
                    int n = arg.getItemOrBlockId();
                    int n2 = arg2.getItemOrBlockId();
                    Stat stat = null;
                    Stat stat2 = null;
                    if (BlockStatsListWidget.this.selectedTab == 2) {
                        stat = Stats.MINE_BLOCK[n];
                        stat2 = Stats.MINE_BLOCK[n2];
                    } else if (BlockStatsListWidget.this.selectedTab == 0) {
                        stat = Stats.CRAFTED[n];
                        stat2 = Stats.CRAFTED[n2];
                    } else if (BlockStatsListWidget.this.selectedTab == 1) {
                        stat = Stats.USED[n];
                        stat2 = Stats.USED[n2];
                    }
                    if (stat != null || stat2 != null) {
                        int n3;
                        if (stat == null) {
                            return 1;
                        }
                        if (stat2 == null) {
                            return -1;
                        }
                        int n4 = StatsScreen.this.stats.get(stat);
                        if (n4 != (n3 = StatsScreen.this.stats.get(stat2))) {
                            return (n4 - n3) * BlockStatsListWidget.this.statSortOrder;
                        }
                    }
                    return n - n2;
                }
            };
        }

        protected void renderHeader(int x, int y, Tessellator tessellator) {
            super.renderHeader(x, y, tessellator);
            if (this.clickedIconId == 0) {
                StatsScreen.this.renderIcon(x + 115 - 18 + 1, y + 1 + 1, 18, 18);
            } else {
                StatsScreen.this.renderIcon(x + 115 - 18, y + 1, 18, 18);
            }
            if (this.clickedIconId == 1) {
                StatsScreen.this.renderIcon(x + 165 - 18 + 1, y + 1 + 1, 36, 18);
            } else {
                StatsScreen.this.renderIcon(x + 165 - 18, y + 1, 36, 18);
            }
            if (this.clickedIconId == 2) {
                StatsScreen.this.renderIcon(x + 215 - 18 + 1, y + 1 + 1, 54, 18);
            } else {
                StatsScreen.this.renderIcon(x + 215 - 18, y + 1, 54, 18);
            }
        }

        protected void renderEntry(int index, int x, int y, int l, Tessellator tessellator) {
            ItemOrBlockStat itemOrBlockStat = this.getEntry(index);
            int n = itemOrBlockStat.getItemOrBlockId();
            StatsScreen.this.render(x + 40, y, n);
            this.renderStat((ItemOrBlockStat)Stats.CRAFTED[n], x + 115, y, index % 2 == 0);
            this.renderStat((ItemOrBlockStat)Stats.USED[n], x + 165, y, index % 2 == 0);
            this.renderStat(itemOrBlockStat, x + 215, y, index % 2 == 0);
        }

        protected String getColumnHeader(int column) {
            if (column == 0) {
                return "stat.crafted";
            }
            if (column == 1) {
                return "stat.used";
            }
            return "stat.mined";
        }
    }

    @Environment(value=EnvType.CLIENT)
    abstract class AbstractStatsListWidget
    extends EntryListWidget {
        protected int clickedIconId;
        protected List entries;
        protected Comparator statComparator;
        protected int selectedTab;
        protected int statSortOrder;

        protected AbstractStatsListWidget() {
            super(StatsScreen.this.minecraft, StatsScreen.this.width, StatsScreen.this.height, 32, StatsScreen.this.height - 64, 20);
            this.clickedIconId = -1;
            this.selectedTab = -1;
            this.statSortOrder = 0;
            this.setRenderSelectionHighlight(false);
            this.setHeader(true, 20);
        }

        protected void entryClicked(int index, boolean doubleClick) {
        }

        protected boolean isSelectedEntry(int index) {
            return false;
        }

        protected void renderBackground() {
            StatsScreen.this.renderBackground();
        }

        protected void renderHeader(int x, int y, Tessellator tessellator) {
            if (!Mouse.isButtonDown(0)) {
                this.clickedIconId = -1;
            }
            if (this.clickedIconId == 0) {
                StatsScreen.this.renderIcon(x + 115 - 18, y + 1, 0, 0);
            } else {
                StatsScreen.this.renderIcon(x + 115 - 18, y + 1, 0, 18);
            }
            if (this.clickedIconId == 1) {
                StatsScreen.this.renderIcon(x + 165 - 18, y + 1, 0, 0);
            } else {
                StatsScreen.this.renderIcon(x + 165 - 18, y + 1, 0, 18);
            }
            if (this.clickedIconId == 2) {
                StatsScreen.this.renderIcon(x + 215 - 18, y + 1, 0, 0);
            } else {
                StatsScreen.this.renderIcon(x + 215 - 18, y + 1, 0, 18);
            }
            if (this.selectedTab != -1) {
                int n = 79;
                int n2 = 18;
                if (this.selectedTab == 1) {
                    n = 129;
                } else if (this.selectedTab == 2) {
                    n = 179;
                }
                if (this.statSortOrder == 1) {
                    n2 = 36;
                }
                StatsScreen.this.renderIcon(x + n, y + 1, n2, 0);
            }
        }

        protected void headerClicked(int x, int y) {
            this.clickedIconId = -1;
            if (x >= 79 && x < 115) {
                this.clickedIconId = 0;
            } else if (x >= 129 && x < 165) {
                this.clickedIconId = 1;
            } else if (x >= 179 && x < 215) {
                this.clickedIconId = 2;
            }
            if (this.clickedIconId >= 0) {
                this.click(this.clickedIconId);
                ((StatsScreen)StatsScreen.this).minecraft.soundManager.playSound("random.click", 1.0f, 1.0f);
            }
        }

        protected final int getEntryCount() {
            return this.entries.size();
        }

        protected final ItemOrBlockStat getEntry(int index) {
            return (ItemOrBlockStat)this.entries.get(index);
        }

        protected abstract String getColumnHeader(int var1);

        protected void renderStat(ItemOrBlockStat stat, int x, int y, boolean isRowEven) {
            if (stat != null) {
                String string = stat.format(StatsScreen.this.stats.get(stat));
                StatsScreen.this.drawTextWithShadow(StatsScreen.this.textRenderer, string, x - StatsScreen.this.textRenderer.getWidth(string), y + 5, isRowEven ? 0xFFFFFF : 0x909090);
            } else {
                String string = "-";
                StatsScreen.this.drawTextWithShadow(StatsScreen.this.textRenderer, string, x - StatsScreen.this.textRenderer.getWidth(string), y + 5, isRowEven ? 0xFFFFFF : 0x909090);
            }
        }

        protected void renderDecorations(int mouseX, int mouseY) {
            if (mouseY < this.top || mouseY > this.bottom) {
                return;
            }
            int n = this.getEntryAt(mouseX, mouseY);
            int n2 = StatsScreen.this.width / 2 - 92 - 16;
            if (n >= 0) {
                if (mouseX < n2 + 40 || mouseX > n2 + 40 + 20) {
                    return;
                }
                ItemOrBlockStat itemOrBlockStat = this.getEntry(n);
                this.renderStat(itemOrBlockStat, mouseX, mouseY);
            } else {
                String string = "";
                if (mouseX >= n2 + 115 - 18 && mouseX <= n2 + 115) {
                    string = this.getColumnHeader(0);
                } else if (mouseX >= n2 + 165 - 18 && mouseX <= n2 + 165) {
                    string = this.getColumnHeader(1);
                } else if (mouseX >= n2 + 215 - 18 && mouseX <= n2 + 215) {
                    string = this.getColumnHeader(2);
                } else {
                    return;
                }
                string = ("" + TranslationStorage.getInstance().get(string)).trim();
                if (string.length() > 0) {
                    int n3 = mouseX + 12;
                    int n4 = mouseY - 12;
                    int n5 = StatsScreen.this.textRenderer.getWidth(string);
                    StatsScreen.this.fillGradient(n3 - 3, n4 - 3, n3 + n5 + 3, n4 + 8 + 3, -1073741824, -1073741824);
                    StatsScreen.this.textRenderer.drawWithShadow(string, n3, n4, -1);
                }
            }
        }

        protected void renderStat(ItemOrBlockStat stat, int x, int y) {
            if (stat == null) {
                return;
            }
            Item item = Item.ITEMS[stat.getItemOrBlockId()];
            String string = ("" + TranslationStorage.getInstance().getClientTranslation(item.getTranslationKey())).trim();
            if (string.length() > 0) {
                int n = x + 12;
                int n2 = y - 12;
                int n3 = StatsScreen.this.textRenderer.getWidth(string);
                StatsScreen.this.fillGradient(n - 3, n2 - 3, n + n3 + 3, n2 + 8 + 3, -1073741824, -1073741824);
                StatsScreen.this.textRenderer.drawWithShadow(string, n, n2, -1);
            }
        }

        protected void click(int buttonDownTime) {
            if (buttonDownTime != this.selectedTab) {
                this.selectedTab = buttonDownTime;
                this.statSortOrder = -1;
            } else if (this.statSortOrder == -1) {
                this.statSortOrder = 1;
            } else {
                this.selectedTab = -1;
                this.statSortOrder = 0;
            }
            Collections.sort(this.entries, this.statComparator);
        }
    }
}

