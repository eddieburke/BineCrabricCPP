package net.minecraft.client.gui.screen.option;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.gui.screen.Screen;
import net.minecraft.client.gui.widget.ButtonWidget;
import net.minecraft.client.option.GameOptions;
import net.minecraft.client.option.Option;
import net.minecraft.client.resource.language.TranslationStorage;

@Environment(value=EnvType.CLIENT)
public class WorldSettingsScreen extends SettingsBaseScreen {
    private static final Option[] OPTIONS = {
        Option.WEATHER,
        Option.TIME, Option.AUTOSAVE_TICKS,
        Option.FAST_DEBUG_INFO
    };

    public WorldSettingsScreen(Screen parent, GameOptions options) {
        super(parent, options, "World Settings");
    }

    protected Option[] getOptions() { return OPTIONS; }

    public void init() {
        super.init();
        int baseY = this.height / 6 + 24 * ((OPTIONS.length + 1) / 2) + 24;
        // Remove done button added by super, reposition it below fog button
        for (int i = this.buttons.size() - 1; i >= 0; i--) {
            if (((ButtonWidget)this.buttons.get(i)).id == 200) {
                this.buttons.remove(i);
                break;
            }
        }
        this.buttons.add(new ButtonWidget(100, this.width / 2 - 60, baseY, 120, 20, "Fog Settings..."));
        this.buttons.add(new ButtonWidget(200, this.width / 2 - 100, baseY + 24, TranslationStorage.getInstance().get("gui.done")));
    }

    protected void buttonClicked(ButtonWidget button) {
        if (button.id == 100) {
            this.minecraft.options.save();
            this.minecraft.setScreen(new FogSettingsScreen(this, this.options));
            return;
        }
        super.buttonClicked(button);
    }
}
