/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.client.option;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;

@Environment(value=EnvType.CLIENT)
public enum Option {
    // Core
    MUSIC("options.music", true, false),
    SOUND("options.sound", true, false),
    INVERT_MOUSE("options.invertMouse", false, true),
    SENSITIVITY("options.sensitivity", true, false),
    DIFFICULTY("options.difficulty", false, false),
    FOV("FOV", true, false),
    VIEW_BOBBING("options.viewBobbing", false, true),
    GUI_SCALE("options.guiScale", false, false),

    // Quality
    GRAPHICS("options.graphics", false, false),
    RENDER_DISTANCE("options.renderDistance", false, false),
    RENDER_SCALE("Render Scale", true, false),
    MIPMAP_LEVEL("Mipmap Level", false, false),
    MIPMAP_TYPE("Mipmap Type", false, true),
    AO_LEVEL("Smooth Lighting", true, false),
    BRIGHTNESS("Brightness", true, false),
    CLEAR_WATER("Clear Water", false, true),

    // Performance
    FRAMERATE_LIMIT("options.framerateLimit", false, false),
    SMOOTH_FPS("Smooth FPS", false, true),
    SMOOTH_INPUT("Smooth Input", false, true),
    ADVANCED_OPENGL("options.advancedOpengl", false, false),
    VBO_ENABLED("VBO", false, true),
    CHUNK_UPDATES("Chunk Updates", true, false),
    CHUNK_UPDATES_DYNAMIC("Dynamic Updates", false, true),
    PRELOADED_CHUNKS("Preloaded Chunks", false, false),
    ENTITY_DISTANCE("Entity Distance", true, false),

    // Details
    CLOUDS("Clouds", false, false),
    CLOUD_HEIGHT("Cloud Height", true, false),
    TREES("Trees", false, false),
    GRASS("Grass", false, false),
    WATER("Water", false, false),
    RAIN("Rain & Snow", false, false),
    SKY("Sky", false, true),
    STARS("Stars", false, true),

    // Animations
    ANIMATED_WATER("Water Animated", false, false),
    ANIMATED_LAVA("Lava Animated", false, false),
    ANIMATED_FIRE("Fire Animated", false, true),
    ANIMATED_PORTAL("Portal Animated", false, true),
    ANIMATED_REDSTONE("Redstone Animated", false, true),
    ANIMATED_EXPLOSION("Explosion Animated", false, true),
    ANIMATED_FLAME("Flame Animated", false, true),
    ANIMATED_SMOKE("Smoke Animated", false, true),

    // World / misc
    WEATHER("Weather", false, true),
    TIME("Time", false, false),
    AUTOSAVE_TICKS("Autosave", false, false),
    FAST_DEBUG_INFO("Fast Debug Info", false, true),

    // Stereo / 3D
    STEREO_MODE("options.stereoMode", false, false),
    STEREO_OFFSET("3D Offset", true, false),
    STEREO_SEPARATION("3D Separation", true, false),
    STEREO_RED_BLUE_ORDER("Swap Left/Right", false, true),
    HAND_STEREO_OFFSET("Hand Convergence", true, false),
    HAND_STEREO_SEPARATION("Hand Separation", true, false),
    HAND_DEPTH("Hand Depth", true, false),

    // Fog
    FOG_FANCY("Custom Fog", false, true),
    FOG_PROJECTION("Fog Projection", false, false),
    FOG_START("Fog Start", true, false),
    FOG_MODE("Fog Mode", false, false),
    FOG_END("Fog End", true, false),
    FOG_DENSITY("Fog Density", true, false),
    FOG_COLOR_RED("Fog Red", true, false),
    FOG_COLOR_GREEN("Fog Green", true, false),
    FOG_COLOR_BLUE("Fog Blue", true, false),
    FOG_COLOR_MODE("Fog Color Mode", false, false);

    private final boolean slider;
    private final boolean toggle;
    private final String key;

    private Option(String key, boolean slider, boolean toggle) {
        this.key = key;
        this.slider = slider;
        this.toggle = toggle;
    }

    public static Option getById(int id) {
        for (Option option : Option.values()) {
            if (option.getId() != id) continue;
            return option;
        }
        return null;
    }

    public boolean isSlider() {
        return this.slider;
    }

    public boolean isToggle() {
        return this.toggle;
    }

    public int getId() {
        return this.ordinal();
    }

    public String getKey() {
        return this.key;
    }
}
