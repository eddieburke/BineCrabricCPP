package net.minecraft.client.gui.screen.option;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.option.GameOptions;
import net.minecraft.client.option.Option;
import net.minecraft.client.gui.screen.Screen;

@Environment(value=EnvType.CLIENT)
public class DetailSettingsScreen extends SettingsBaseScreen {
    private static final Option[] OPTIONS = {
        Option.CLOUDS, Option.CLOUD_HEIGHT,
        Option.TREES, Option.GRASS,
        Option.WATER, Option.RAIN,
        Option.SKY, Option.STARS
    };

    public DetailSettingsScreen(Screen parent, GameOptions options) {
        super(parent, options, "Detail Settings");
    }

    protected Option[] getOptions() { return OPTIONS; }
}
