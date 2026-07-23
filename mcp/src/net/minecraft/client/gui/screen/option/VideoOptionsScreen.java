package net.minecraft.client.gui.screen.option;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.gui.screen.Screen;
import net.minecraft.client.gui.widget.ButtonWidget;
import net.minecraft.client.option.GameOptions;
import net.minecraft.client.resource.language.TranslationStorage;

@Environment(value=EnvType.CLIENT)
public class VideoOptionsScreen
extends Screen {
    private Screen parent;
    protected String title = "Video Settings";
    private GameOptions options;

    public VideoOptionsScreen(Screen parent, GameOptions options) {
        this.parent = parent;
        this.options = options;
    }

    public void init() {
        TranslationStorage ts = TranslationStorage.getInstance();
        this.title = ts.get("options.videoTitle");
        int startY = this.height / 6;
        int x1 = this.width / 2 - 155;
        int x2 = this.width / 2 + 5;

        this.buttons.add(new WideButtonWidget(100, this.width / 2 - 155, startY, 310, "Quality..."));
        this.buttons.add(new ButtonWidget(101, x1, startY + 24, 150, 20, "Performance..."));
        this.buttons.add(new ButtonWidget(102, x2, startY + 24, 150, 20, "Details..."));
        this.buttons.add(new ButtonWidget(103, x1, startY + 48, 150, 20, "Animations..."));
        this.buttons.add(new ButtonWidget(104, x2, startY + 48, 150, 20, "World..."));
        this.buttons.add(new ButtonWidget(105, x1, startY + 72, 150, 20, "3D Settings..."));
        this.buttons.add(new ButtonWidget(200, this.width / 2 - 100, startY + 96, ts.get("gui.done")));
    }

    protected void buttonClicked(ButtonWidget button) {
        if (!button.active) return;
        if (button.id == 100) { this.minecraft.options.save(); this.minecraft.setScreen(new QualitySettingsScreen(this, this.options)); }
        if (button.id == 101) { this.minecraft.options.save(); this.minecraft.setScreen(new PerformanceSettingsScreen(this, this.options)); }
        if (button.id == 102) { this.minecraft.options.save(); this.minecraft.setScreen(new DetailSettingsScreen(this, this.options)); }
        if (button.id == 103) { this.minecraft.options.save(); this.minecraft.setScreen(new AnimationSettingsScreen(this, this.options)); }
        if (button.id == 104) { this.minecraft.options.save(); this.minecraft.setScreen(new WorldSettingsScreen(this, this.options)); }
        if (button.id == 105) { this.minecraft.options.save(); this.minecraft.setScreen(new StereoSettingsScreen(this, this.options)); }
        if (button.id == 200) { this.minecraft.options.save(); this.minecraft.setScreen(this.parent); }
    }

    public void render(int mouseX, int mouseY, float delta) {
        this.renderBackground();
        this.drawCenteredTextWithShadow(this.textRenderer, this.title, this.width / 2, 20, 0xFFFFFF);
        super.render(mouseX, mouseY, delta);
    }

    static class WideButtonWidget extends ButtonWidget {
        public WideButtonWidget(int id, int x, int y, int width, String text) {
            super(id, x, y, width, 20, text);
        }
    }
}
