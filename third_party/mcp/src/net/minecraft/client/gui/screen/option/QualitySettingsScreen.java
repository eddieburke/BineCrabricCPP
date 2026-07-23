package net.minecraft.client.gui.screen.option;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.option.GameOptions;
import net.minecraft.client.option.Option;
import net.minecraft.client.gui.screen.Screen;

@Environment(value=EnvType.CLIENT)
public class QualitySettingsScreen extends SettingsBaseScreen {
    private static final Option[] OPTIONS = {
        Option.GRAPHICS, Option.RENDER_DISTANCE,
        Option.RENDER_SCALE, Option.MIPMAP_LEVEL,
        Option.MIPMAP_TYPE, Option.AO_LEVEL,
        Option.BRIGHTNESS, Option.CLEAR_WATER
    };

    public QualitySettingsScreen(Screen parent, GameOptions options) {
        super(parent, options, "Quality Settings");
    }

    protected Option[] getOptions() { return OPTIONS; }
}
