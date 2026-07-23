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
public class OptionsScreen
extends Screen {
    private Screen parent;
    protected String title = "Options";
    private GameOptions options;
    private static Option[] RENDER_OPTIONS = new Option[]{
        Option.MUSIC, Option.SOUND,
        Option.INVERT_MOUSE, Option.SENSITIVITY,
        Option.DIFFICULTY, Option.FOV,
        Option.VIEW_BOBBING, Option.GUI_SCALE
    };

    public OptionsScreen(Screen parent, GameOptions options) {
        this.parent = parent;
        this.options = options;
    }

    public void init() {
        TranslationStorage ts = TranslationStorage.getInstance();
        this.title = ts.get("options.title");
        int n = 0;
        for (Option option : RENDER_OPTIONS) {
            int x = this.width / 2 - 155 + n % 2 * 160;
            int y = this.height / 6 + 24 * (n >> 1);
            if (option.isSlider()) {
                this.buttons.add(new SliderWidget(option.getId(), x, y, option, this.options.getString(option), this.options.getFloat(option)));
            } else {
                this.buttons.add(new OptionButtonWidget(option.getId(), x, y, option, this.options.getString(option)));
            }
            ++n;
        }
        int videoY = this.height / 6 + 24 * ((RENDER_OPTIONS.length + 1) / 2);
        this.buttons.add(new ButtonWidget(101, this.width / 2 - 100, videoY, ts.get("options.video")));
        this.buttons.add(new ButtonWidget(100, this.width / 2 - 100, videoY + 24, ts.get("options.controls")));
        this.buttons.add(new ButtonWidget(200, this.width / 2 - 100, videoY + 48, ts.get("gui.done")));
    }

    protected void buttonClicked(ButtonWidget button) {
        if (!button.active) return;
        if (button.id < 100 && button instanceof OptionButtonWidget) {
            this.options.setInt(((OptionButtonWidget)button).getOption(), 1);
            button.text = this.options.getString(Option.getById(button.id));
        }
        if (button.id == 101) {
            this.minecraft.options.save();
            this.minecraft.setScreen(new VideoOptionsScreen(this, this.options));
        }
        if (button.id == 100) {
            this.minecraft.options.save();
            this.minecraft.setScreen(new KeybindsScreen(this, this.options));
        }
        if (button.id == 200) {
            this.minecraft.options.save();
            this.minecraft.setScreen(this.parent);
        }
    }

    public void render(int mouseX, int mouseY, float delta) {
        this.renderBackground();
        this.drawCenteredTextWithShadow(this.textRenderer, this.title, this.width / 2, 20, 0xFFFFFF);
        super.render(mouseX, mouseY, delta);
    }
}
