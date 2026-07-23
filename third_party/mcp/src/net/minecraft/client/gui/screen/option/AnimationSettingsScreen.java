package net.minecraft.client.gui.screen.option;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.option.GameOptions;
import net.minecraft.client.option.Option;
import net.minecraft.client.gui.screen.Screen;

@Environment(value=EnvType.CLIENT)
public class AnimationSettingsScreen extends SettingsBaseScreen {
    private static final Option[] OPTIONS = {
        Option.ANIMATED_WATER, Option.ANIMATED_LAVA,
        Option.ANIMATED_FIRE, Option.ANIMATED_PORTAL,
        Option.ANIMATED_REDSTONE, Option.ANIMATED_EXPLOSION,
        Option.ANIMATED_FLAME, Option.ANIMATED_SMOKE
    };

    public AnimationSettingsScreen(Screen parent, GameOptions options) {
        super(parent, options, "Animation Settings");
    }

    protected Option[] getOptions() { return OPTIONS; }
}
