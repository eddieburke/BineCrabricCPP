package net.minecraft.client.gui.screen.option;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.gui.screen.Screen;
import net.minecraft.client.gui.widget.ButtonWidget;
import net.minecraft.client.gui.widget.OptionButtonWidget;
import net.minecraft.client.gui.widget.SliderWidget;
import net.minecraft.client.option.GameOptions;
import net.minecraft.client.option.Option;
import net.minecraft.client.resource.language.TranslationStorage;

@Environment(value=EnvType.CLIENT)
public abstract class SettingsBaseScreen extends Screen {
    protected Screen parent;
    protected GameOptions options;
    protected String title;

    public SettingsBaseScreen(Screen parent, GameOptions options, String title) {
        this.parent = parent;
        this.options = options;
        this.title = title;
    }

    protected abstract Option[] getOptions();

    public void init() {
        Option[] opts = getOptions();
        for (int i = 0; i < opts.length; i++) {
            Option option = opts[i];
            int x = this.width / 2 - 155 + (i % 2) * 160;
            int y = this.height / 6 + 24 * (i / 2);
            if (option.isSlider()) {
                this.buttons.add(new SliderWidget(option.getId(), x, y, option, this.options.getString(option), this.options.getFloat(option)));
            } else {
                this.buttons.add(new OptionButtonWidget(option.getId(), x, y, option, this.options.getString(option)));
            }
        }
        int doneY = this.height / 6 + 24 * ((opts.length + 1) / 2) + 12;
        this.buttons.add(new ButtonWidget(200, this.width / 2 - 100, doneY, TranslationStorage.getInstance().get("gui.done")));
    }

    protected void buttonClicked(ButtonWidget button) {
        if (!button.active) return;
        if (button.id == 200) {
            this.minecraft.options.save();
            this.minecraft.setScreen(this.parent);
            return;
        }
        if (button instanceof OptionButtonWidget) {
            Option opt = ((OptionButtonWidget)button).getOption();
            if (opt != null) {
                this.options.setInt(opt, 1);
                button.text = this.options.getString(opt);
            }
        }
    }

    public void render(int mouseX, int mouseY, float delta) {
        this.renderBackground();
        this.drawCenteredTextWithShadow(this.textRenderer, this.title, this.width / 2, 20, 0xFFFFFF);
        super.render(mouseX, mouseY, delta);
    }
}
