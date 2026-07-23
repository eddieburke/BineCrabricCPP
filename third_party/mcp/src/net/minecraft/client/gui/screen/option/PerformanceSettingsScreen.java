package net.minecraft.client.gui.screen.option;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.option.GameOptions;
import net.minecraft.client.option.Option;
import net.minecraft.client.gui.screen.Screen;

@Environment(value=EnvType.CLIENT)
public class PerformanceSettingsScreen extends SettingsBaseScreen {
    private static final Option[] OPTIONS = {
        Option.FRAMERATE_LIMIT, Option.SMOOTH_FPS,
        Option.SMOOTH_INPUT, Option.ADVANCED_OPENGL,
        Option.VBO_ENABLED, Option.CHUNK_UPDATES,
        Option.CHUNK_UPDATES_DYNAMIC, Option.PRELOADED_CHUNKS,
        Option.ENTITY_DISTANCE
    };

    public PerformanceSettingsScreen(Screen parent, GameOptions options) {
        super(parent, options, "Performance Settings");
    }

    protected Option[] getOptions() { return OPTIONS; }
}
