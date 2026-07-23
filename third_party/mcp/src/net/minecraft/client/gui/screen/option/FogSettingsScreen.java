package net.minecraft.client.gui.screen.option;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.option.GameOptions;
import net.minecraft.client.option.Option;
import net.minecraft.client.gui.screen.Screen;
import net.minecraft.client.gui.widget.ButtonWidget;
import net.minecraft.client.gui.widget.OptionButtonWidget;
import net.minecraft.client.gui.widget.SliderWidget;
import net.minecraft.client.resource.language.TranslationStorage;

@Environment(value=EnvType.CLIENT)
public class FogSettingsScreen extends SettingsBaseScreen {
    private static final Option[] OPTIONS = {
        Option.FOG_FANCY, Option.FOG_PROJECTION,
        Option.FOG_START, Option.FOG_MODE,
        Option.FOG_END, Option.FOG_DENSITY,
        Option.FOG_COLOR_MODE
    };

    public FogSettingsScreen(Screen parent, GameOptions options) {
        super(parent, options, "Fog Settings");
    }

    protected Option[] getOptions() { return OPTIONS; }

    public void init() {
        super.init();
        updateButtonStates();
    }

    protected void buttonClicked(ButtonWidget button) {
        super.buttonClicked(button);
        if (button.id == Option.FOG_FANCY.getId() || button.id == Option.FOG_MODE.getId()) {
            updateButtonStates();
        }
    }

    private void updateButtonStates() {
        boolean customFog = this.options.ofFogFancy;
        boolean linear = this.options.ofFogMode == 0;
        for (int i = 0; i < this.buttons.size(); i++) {
            ButtonWidget btn = (ButtonWidget)this.buttons.get(i);
            Option opt = Option.getById(btn.id);
            if (opt == null || opt == Option.FOG_FANCY) continue;
            if (!customFog) {
                btn.active = false;
            } else if (opt == Option.FOG_START || opt == Option.FOG_END) {
                btn.active = linear;
            } else if (opt == Option.FOG_DENSITY) {
                btn.active = !linear;
            } else {
                btn.active = true;
            }
        }
    }
}
