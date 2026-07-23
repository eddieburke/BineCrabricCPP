package net.minecraft.client.gui.screen.option;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.option.GameOptions;
import net.minecraft.client.option.Option;
import net.minecraft.client.gui.screen.Screen;

@Environment(value=EnvType.CLIENT)
public class StereoSettingsScreen extends SettingsBaseScreen {
    private static final Option[] OPTIONS = {
        Option.STEREO_MODE, Option.STEREO_OFFSET,
        Option.STEREO_SEPARATION, Option.STEREO_RED_BLUE_ORDER,
        Option.HAND_STEREO_OFFSET, Option.HAND_STEREO_SEPARATION,
        Option.HAND_DEPTH
    };

    public StereoSettingsScreen(Screen parent, GameOptions options) {
        super(parent, options, "3D Settings");
    }

    protected Option[] getOptions() { return OPTIONS; }
}
