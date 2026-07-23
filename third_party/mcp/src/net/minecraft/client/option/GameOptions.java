package net.minecraft.client.option;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.PrintWriter;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.Minecraft;
import net.minecraft.client.option.KeyBinding;
import net.minecraft.client.option.Option;
import net.minecraft.client.resource.language.I18n;
import net.minecraft.client.resource.language.TranslationStorage;
import org.lwjgl.input.Keyboard;

@Environment(value=EnvType.CLIENT)
public class GameOptions {
    private static final String[] RENDER_DISTANCE_KEYS = new String[]{"options.renderDistance.far", "options.renderDistance.normal", "options.renderDistance.short", "options.renderDistance.tiny"};
    private static final String[] DIFFICULTY_KEYS = new String[]{"options.difficulty.peaceful", "options.difficulty.easy", "options.difficulty.normal", "options.difficulty.hard"};
    private static final String[] GUI_SCALE_KEYS = new String[]{"options.guiScale.auto", "options.guiScale.small", "options.guiScale.normal", "options.guiScale.large"};
    private static final String[] PERFORMANCE_KEYS = new String[]{"performance.max", "performance.balanced", "performance.powersaver"};
    private static final String[] TREES_KEYS = new String[]{"options.trees.fancy", "options.trees.fast"};
    private static final String[] GRASS_KEYS = new String[]{"options.grass.fancy", "options.grass.fast", "options.grass.better_fancy", "options.grass.better_fast"};
    private static final String[] WATER_KEYS = new String[]{"options.water.fancy", "options.water.fast", "options.water.off"};
    private static final String[] RAIN_KEYS = new String[]{"options.rain.fancy", "options.rain.fast", "options.rain.off"};
    private static final String[] ANIMATED_KEYS = new String[]{"options.anim.on", "options.anim.off"};
    private static final String[] ANIMATED_LAVA_KEYS = new String[]{"options.anim.on", "options.anim.off"};
    private static final String[] STEREO_MODE_KEYS = new String[]{"options.stereo.off", "options.stereo.anaglyph", "options.stereo.sbs"};
    private static final String[] ADVANCED_OPENGL_KEYS = new String[]{"options.advancedOpengl.off", "options.advancedOpengl.fast", "options.advancedOpengl.fancy"};
    private static final String[] MIPMAP_LEVEL_KEYS = new String[]{"options.off", "1", "2", "3", "4"};
    private static final String[] TIME_KEYS = new String[]{"options.time.default", "options.time.day", "options.time.night"};
    private static final String[] AUTOSAVE_KEYS = new String[]{"2s", "20s", "3min", "30min"};
    private static final String[] PRELOADED_CHUNKS_KEYS = new String[]{"options.off", "2", "4", "6", "8"};
    private static final String[] FOG_MODE_KEYS = new String[]{"Linear", "Exp"};
    private static final String[] FOG_PROJECTION_KEYS = new String[]{"Spherical", "Cylindrical"};
    private static final String[] FOG_COLOR_MODE_KEYS = new String[]{"Sky Color", "Custom"};
    private static final int[] OF_PRELOADED_CHUNKS = {0, 2, 4, 6, 8};
    private static final int[] OF_AUTOSAVE_TICKS = {40, 400, 4000, 40000};

    // Core options
    public float musicVolume = 1.0f;
    public float soundVolume = 1.0f;
    public float mouseSensitivity = 0.5f;
    public boolean invertYMouse = false;
    public int viewDistance = 0;
    public boolean bobView = true;
    public boolean anaglyph3d = false;
    public int advancedOpengl = 0;
    public int fpsLimit = 1;
    public boolean fancyGraphics = true;
    public boolean ao = true;
    public String skin = "Default";
    public int difficulty = 2;
    public int guiScale = 0;
    public float fieldOfView = 0.0f;

    // Quality
    public float ofRenderScale = 1.0f;
    public int ofMipmapLevel = 0;
    public boolean ofMipmapLinear = false;
    public float ofAoLevel = 1.0f;
    public float ofBrightness = 0.0f;
    public boolean ofClearWater = false;

    // Performance
    public boolean ofSmoothFps = false;
    public boolean ofSmoothInput = false;
    public boolean ofVBO = false;
    public float ofChunkUpdates = 0.5f;
    public boolean ofChunkUpdatesDynamic = false;
    public int ofPreloadedChunks = 0;
    public float ofEntityDistanceScale = 1.0f;

    // Details
    public int ofClouds = 0;
    public float ofCloudsHeight = 0.0f;
    public int ofTrees = 0;
    public int ofGrass = 0;
    public int ofWater = 0;
    public int ofRain = 0;
    public boolean ofSky = true;
    public boolean ofStars = true;

    // Animations
    public int ofAnimatedWater = 0;
    public int ofAnimatedLava = 0;
    public boolean ofAnimatedFire = true;
    public boolean ofAnimatedPortal = true;
    public boolean ofAnimatedRedstone = true;
    public boolean ofAnimatedExplosion = true;
    public boolean ofAnimatedFlame = true;
    public boolean ofAnimatedSmoke = true;

    // World / misc
    public boolean ofWeather = true;
    public int ofTime = 0;
    public int ofAutoSaveTicks = 4000;
    public boolean ofFastDebugInfo = false;

    // Stereo / 3D
    public int stereoMode = 0;
    public float ofStereoOffset = 0.07f;
    public float ofStereoSeparation = 0.1f;
    public boolean ofStereoRedBlueOrder = false;
    public float ofHandStereoOffset = 0.07f;
    public float ofHandStereoSeparation = 0.1f;
    public float ofHandDepth = 0.0f;

    // Fog
    public boolean ofFogFancy = false;
    public int ofFogProjection = 0;
    public float ofFogStart = 0.2f;
    public int ofFogMode = 0;
    public float ofFogEnd = 0.8f;
    public float ofFogDensity = 0.1f;
    public float ofFogColorRed = 0.0f;
    public float ofFogColorGreen = 0.0f;
    public float ofFogColorBlue = 0.0f;
    public int ofFogColorMode = 0;

    public KeyBinding forwardKey = new KeyBinding("key.forward", Keyboard.KEY_W);
    public KeyBinding leftKey = new KeyBinding("key.left", Keyboard.KEY_A);
    public KeyBinding backKey = new KeyBinding("key.back", Keyboard.KEY_S);
    public KeyBinding rightKey = new KeyBinding("key.right", Keyboard.KEY_D);
    public KeyBinding jumpKey = new KeyBinding("key.jump", Keyboard.KEY_SPACE);
    public KeyBinding inventoryKey = new KeyBinding("key.inventory", Keyboard.KEY_E);
    public KeyBinding dropKey = new KeyBinding("key.drop", Keyboard.KEY_Q);
    public KeyBinding chatKey = new KeyBinding("key.chat", Keyboard.KEY_T);
    public KeyBinding fogKey = new KeyBinding("key.fog", Keyboard.KEY_F);
    public KeyBinding sneakKey = new KeyBinding("key.sneak", Keyboard.KEY_LSHIFT);
    public KeyBinding[] allKeys = new KeyBinding[]{this.forwardKey, this.leftKey, this.backKey, this.rightKey, this.jumpKey, this.sneakKey, this.dropKey, this.inventoryKey, this.chatKey, this.fogKey};
    protected Minecraft minecraft;
    private File file;
    public boolean hideHud = false;
    public boolean thirdPerson = false;
    public boolean debugHud = false;
    public String lastServer = "";
    public boolean discreteScroll = false;
    public boolean cinematicMode = false;
    public boolean debugCamera = false;
    public float totalDiscreteScroll = 1.0f;
    public float gamma = 1.0f;

    public GameOptions(Minecraft minecraft, File file) {
        this.minecraft = minecraft;
        this.file = new File(file, "options.txt");
        this.load();
    }

    public GameOptions() {
    }

    public String getKeybindName(int index) {
        TranslationStorage translationStorage = TranslationStorage.getInstance();
        return translationStorage.get(this.allKeys[index].translationKey);
    }

    public String getKeybindKey(int index) {
        return Keyboard.getKeyName(this.allKeys[index].code);
    }

    public void setKeybindKey(int index, int keyCode) {
        this.allKeys[index].code = keyCode;
        this.save();
    }

    public void setFloat(Option option, float value) {
        switch (option) {
            case MUSIC: this.musicVolume = value; if (this.minecraft != null) this.minecraft.soundManager.updateMusicVolume(); break;
            case SOUND: this.soundVolume = value; if (this.minecraft != null) this.minecraft.soundManager.updateMusicVolume(); break;
            case SENSITIVITY: this.mouseSensitivity = value; break;
            case FOV: this.fieldOfView = value; break;
            case RENDER_SCALE: this.ofRenderScale = 1.0f + value * 4.0f; break;
            case AO_LEVEL: this.ofAoLevel = value; this.ao = value > 0; break;
            case BRIGHTNESS: this.ofBrightness = value; break;
            case CHUNK_UPDATES: this.ofChunkUpdates = value; break;
            case ENTITY_DISTANCE: this.ofEntityDistanceScale = 0.25f + value * 3.75f; break;
            case CLOUD_HEIGHT: this.ofCloudsHeight = value; break;
            case STEREO_OFFSET: this.ofStereoOffset = value; break;
            case STEREO_SEPARATION: this.ofStereoSeparation = value; break;
            case HAND_STEREO_OFFSET: this.ofHandStereoOffset = value; break;
            case HAND_STEREO_SEPARATION: this.ofHandStereoSeparation = value; break;
            case HAND_DEPTH: this.ofHandDepth = value; break;
            case FOG_START: this.ofFogStart = value; break;
            case FOG_END: this.ofFogEnd = value; break;
            case FOG_DENSITY: this.ofFogDensity = value; break;
            case FOG_COLOR_RED: this.ofFogColorRed = value; break;
            case FOG_COLOR_GREEN: this.ofFogColorGreen = value; break;
            case FOG_COLOR_BLUE: this.ofFogColorBlue = value; break;
            default: break;
        }
        this.save();
    }

    public void setInt(Option option, int value) {
        switch (option) {
            case INVERT_MOUSE: this.invertYMouse = !this.invertYMouse; break;
            case VIEW_BOBBING: this.bobView = !this.bobView; break;
            case RENDER_DISTANCE: this.viewDistance = (this.viewDistance + value) & 3; break;
            case GUI_SCALE: this.guiScale = (this.guiScale + value) & 3; break;
            case ADVANCED_OPENGL:
                this.advancedOpengl = (this.advancedOpengl + value + 3) % 3;
                if (this.minecraft != null) this.minecraft.worldRenderer.reload();
                break;
            case FRAMERATE_LIMIT: this.fpsLimit = (this.fpsLimit + value + 3) % 3; break;
            case DIFFICULTY: this.difficulty = (this.difficulty + value) & 3; break;
            case GRAPHICS:
                this.fancyGraphics = !this.fancyGraphics;
                if (this.minecraft != null) this.minecraft.worldRenderer.reload();
                break;
            case AMBIENT_OCCLUSION:
                this.ao = !this.ao;
                if (this.minecraft != null) this.minecraft.worldRenderer.reload();
                break;
            case MIPMAP_LEVEL: this.ofMipmapLevel = (this.ofMipmapLevel + value + 5) % 5; break;
            case MIPMAP_TYPE: this.ofMipmapLinear = !this.ofMipmapLinear; break;
            case CLEAR_WATER: this.ofClearWater = !this.ofClearWater; break;
            case SMOOTH_FPS: this.ofSmoothFps = !this.ofSmoothFps; break;
            case SMOOTH_INPUT: this.ofSmoothInput = !this.ofSmoothInput; break;
            case VBO_ENABLED: this.ofVBO = !this.ofVBO; break;
            case CHUNK_UPDATES_DYNAMIC: this.ofChunkUpdatesDynamic = !this.ofChunkUpdatesDynamic; break;
            case PRELOADED_CHUNKS: this.ofPreloadedChunks = cycleArray(this.ofPreloadedChunks, value, OF_PRELOADED_CHUNKS); break;
            case CLOUDS: this.ofClouds = (this.ofClouds + value + 2) % 2; break;
            case TREES: this.ofTrees = (this.ofTrees + value + 2) % 2; break;
            case GRASS: this.ofGrass = (this.ofGrass + value + 4) % 4; break;
            case WATER: this.ofWater = (this.ofWater + value + 3) % 3; break;
            case RAIN: this.ofRain = (this.ofRain + value + 3) % 3; break;
            case SKY: this.ofSky = !this.ofSky; break;
            case STARS: this.ofStars = !this.ofStars; break;
            case ANIMATED_WATER: this.ofAnimatedWater = (this.ofAnimatedWater + value + 2) % 2; break;
            case ANIMATED_LAVA: this.ofAnimatedLava = (this.ofAnimatedLava + value + 2) % 2; break;
            case ANIMATED_FIRE: this.ofAnimatedFire = !this.ofAnimatedFire; break;
            case ANIMATED_PORTAL: this.ofAnimatedPortal = !this.ofAnimatedPortal; break;
            case ANIMATED_REDSTONE: this.ofAnimatedRedstone = !this.ofAnimatedRedstone; break;
            case ANIMATED_EXPLOSION: this.ofAnimatedExplosion = !this.ofAnimatedExplosion; break;
            case ANIMATED_FLAME: this.ofAnimatedFlame = !this.ofAnimatedFlame; break;
            case ANIMATED_SMOKE: this.ofAnimatedSmoke = !this.ofAnimatedSmoke; break;
            case WEATHER: this.ofWeather = !this.ofWeather; break;
            case TIME: this.ofTime = (this.ofTime + value + 3) % 3; break;
            case AUTOSAVE_TICKS: this.ofAutoSaveTicks = cycleArray(this.ofAutoSaveTicks, value, OF_AUTOSAVE_TICKS); break;
            case FAST_DEBUG_INFO: this.ofFastDebugInfo = !this.ofFastDebugInfo; break;
            case STEREO_MODE: this.stereoMode = (this.stereoMode + value + 3) % 3; break;
            case STEREO_RED_BLUE_ORDER: this.ofStereoRedBlueOrder = !this.ofStereoRedBlueOrder; break;
            case FOG_FANCY: this.ofFogFancy = !this.ofFogFancy; break;
            case FOG_MODE: this.ofFogMode = (this.ofFogMode + value + 2) % 2; break;
            case FOG_PROJECTION: this.ofFogProjection = (this.ofFogProjection + value + 2) % 2; break;
            case FOG_COLOR_MODE: this.ofFogColorMode = (this.ofFogColorMode + value + 2) % 2; break;
            default: break;
        }
        this.save();
    }

    private int cycleArray(int current, int dir, int[] values) {
        int idx = 0;
        for (int i = 0; i < values.length; i++) {
            if (values[i] == current) { idx = i; break; }
        }
        idx = (idx + dir + values.length) % values.length;
        return values[idx];
    }

    public float getFloat(Option option) {
        switch (option) {
            case MUSIC: return this.musicVolume;
            case SOUND: return this.soundVolume;
            case SENSITIVITY: return this.mouseSensitivity;
            case FOV: return this.fieldOfView;
            case RENDER_SCALE: return (this.ofRenderScale - 1.0f) / 4.0f;
            case AO_LEVEL: return this.ofAoLevel;
            case BRIGHTNESS: return this.ofBrightness;
            case CHUNK_UPDATES: return this.ofChunkUpdates;
            case ENTITY_DISTANCE: return (this.ofEntityDistanceScale - 0.25f) / 3.75f;
            case CLOUD_HEIGHT: return this.ofCloudsHeight;
            case STEREO_OFFSET: return this.ofStereoOffset;
            case STEREO_SEPARATION: return this.ofStereoSeparation;
            case HAND_STEREO_OFFSET: return this.ofHandStereoOffset;
            case HAND_STEREO_SEPARATION: return this.ofHandStereoSeparation;
            case HAND_DEPTH: return this.ofHandDepth;
            case FOG_START: return this.ofFogStart;
            case FOG_END: return this.ofFogEnd;
            case FOG_DENSITY: return this.ofFogDensity;
            case FOG_COLOR_RED: return this.ofFogColorRed;
            case FOG_COLOR_GREEN: return this.ofFogColorGreen;
            case FOG_COLOR_BLUE: return this.ofFogColorBlue;
            default: return 0.0f;
        }
    }

    public boolean getBoolean(Option option) {
        switch (option) {
            case INVERT_MOUSE: return this.invertYMouse;
            case VIEW_BOBBING: return this.bobView;
            case ADVANCED_OPENGL: return this.advancedOpengl > 0;
            case MIPMAP_TYPE: return this.ofMipmapLinear;
            case CLEAR_WATER: return this.ofClearWater;
            case SMOOTH_FPS: return this.ofSmoothFps;
            case SMOOTH_INPUT: return this.ofSmoothInput;
            case VBO_ENABLED: return this.ofVBO;
            case CHUNK_UPDATES_DYNAMIC: return this.ofChunkUpdatesDynamic;
            case SKY: return this.ofSky;
            case STARS: return this.ofStars;
            case ANIMATED_FIRE: return this.ofAnimatedFire;
            case ANIMATED_PORTAL: return this.ofAnimatedPortal;
            case ANIMATED_REDSTONE: return this.ofAnimatedRedstone;
            case ANIMATED_EXPLOSION: return this.ofAnimatedExplosion;
            case ANIMATED_FLAME: return this.ofAnimatedFlame;
            case ANIMATED_SMOKE: return this.ofAnimatedSmoke;
            case WEATHER: return this.ofWeather;
            case FAST_DEBUG_INFO: return this.ofFastDebugInfo;
            case STEREO_RED_BLUE_ORDER: return this.ofStereoRedBlueOrder;
            case FOG_FANCY: return this.ofFogFancy;
            default: return false;
        }
    }

    public String getString(Option option) {
        TranslationStorage ts = TranslationStorage.getInstance();
        String label = ts.get(option.getKey()) + ": ";
        if (option.isSlider()) {
            float f = this.getFloat(option);
            if (option == Option.SENSITIVITY) {
                if (f == 0.0f) return label + ts.get("options.sensitivity.min");
                if (f == 1.0f) return label + ts.get("options.sensitivity.max");
                return label + (int)(f * 200.0f) + "%";
            }
            if (option == Option.FOV) {
                if (f == 0.0f) return label + "Normal";
                if (f == 1.0f) return label + "Quake Pro";
                return label + (int)(70.0f + f * 40.0f);
            }
            if (option == Option.RENDER_SCALE) {
                float v = 1.0f + f * 4.0f;
                return label + String.format("%.0fx", v);
            }
            if (option == Option.AO_LEVEL) {
                if (f == 0.0f) return label + ts.get("options.off");
                return label + (int)(f * 100.0f) + "%";
            }
            if (option == Option.ENTITY_DISTANCE) {
                float v = 25.0f + f * 375.0f;
                return label + (int)v + "%";
            }
            if (option == Option.CLOUD_HEIGHT) {
                if (f == 0.0f) return label + ts.get("options.off");
                return label + (int)(f * 100.0f) + "%";
            }
            if (f == 0.0f) return label + ts.get("options.off");
            return label + (int)(f * 100.0f) + "%";
        }
        if (option.isToggle()) {
            return label + (this.getBoolean(option) ? ts.get("options.on") : ts.get("options.off"));
        }
        switch (option) {
            case RENDER_DISTANCE: return label + ts.get(RENDER_DISTANCE_KEYS[this.viewDistance]);
            case DIFFICULTY: return label + ts.get(DIFFICULTY_KEYS[this.difficulty]);
            case GUI_SCALE: return label + ts.get(GUI_SCALE_KEYS[this.guiScale]);
            case FRAMERATE_LIMIT: return label + I18n.getTranslation(PERFORMANCE_KEYS[this.fpsLimit]);
            case GRAPHICS: return label + ts.get(this.fancyGraphics ? "options.graphics.fancy" : "options.graphics.fast");
            case MIPMAP_LEVEL: return label + (this.ofMipmapLevel == 0 ? ts.get("options.off") : String.valueOf(this.ofMipmapLevel));
            case CLOUDS: {
                String[] k = {"Fancy", "Normal"};
                return label + k[this.ofClouds & 1];
            }
            case TREES: return label + (this.ofTrees == 0 ? "Fancy" : "Fast");
            case GRASS: { String[] k = {"Fancy", "Fast", "Better Fancy", "Better Fast"}; return label + k[this.ofGrass & 3]; }
            case WATER: { String[] k = {"Fancy", "Fast", "OFF"}; return label + k[this.ofWater % 3]; }
            case RAIN: { String[] k = {"Fancy", "Fast", "OFF"}; return label + k[this.ofRain % 3]; }
            case ANIMATED_WATER: return label + (this.ofAnimatedWater == 0 ? ts.get("options.on") : ts.get("options.off"));
            case ANIMATED_LAVA: return label + (this.ofAnimatedLava == 0 ? ts.get("options.on") : ts.get("options.off"));
            case STEREO_MODE: { String[] k = {"OFF", "Anaglyph", "Side-by-Side"}; return label + k[this.stereoMode % 3]; }
            case ADVANCED_OPENGL: { String[] k = {"OFF", "Fast", "Fancy"}; return label + k[this.advancedOpengl % 3]; }
            case PRELOADED_CHUNKS: return label + (this.ofPreloadedChunks == 0 ? ts.get("options.off") : String.valueOf(this.ofPreloadedChunks));
            case AUTOSAVE_TICKS: {
                int[] vals = OF_AUTOSAVE_TICKS;
                String[] labels = {"2s", "20s", "3min", "30min"};
                for (int i = 0; i < vals.length; i++) if (vals[i] == this.ofAutoSaveTicks) return label + labels[i];
                return label + "?";
            }
            case TIME: { String[] k = {"Default", "Day", "Night"}; return label + k[this.ofTime % 3]; }
            case FOG_MODE: return label + (this.ofFogMode == 0 ? "Linear" : "Exp");
            case FOG_PROJECTION: return label + (this.ofFogProjection == 0 ? "Spherical" : "Cylindrical");
            case FOG_COLOR_MODE: return label + (this.ofFogColorMode == 0 ? "Sky Color" : "Custom");
            default: return label;
        }
    }

    public void load() {
        try {
            if (!this.file.exists()) return;
            BufferedReader br = new BufferedReader(new FileReader(this.file));
            String line;
            while ((line = br.readLine()) != null) {
                try {
                    String[] parts = line.split(":");
                    switch (parts[0]) {
                        case "music": this.musicVolume = Float.parseFloat(parts[1]); break;
                        case "sound": this.soundVolume = Float.parseFloat(parts[1]); break;
                        case "mouseSensitivity": this.mouseSensitivity = Float.parseFloat(parts[1]); break;
                        case "invertYMouse": this.invertYMouse = parts[1].equals("true"); break;
                        case "viewDistance": this.viewDistance = Integer.parseInt(parts[1]); break;
                        case "guiScale": this.guiScale = Integer.parseInt(parts[1]); break;
                        case "bobView": this.bobView = parts[1].equals("true"); break;
                        case "anaglyph3d": this.anaglyph3d = parts[1].equals("true"); break;
                        case "advancedOpengl": this.advancedOpengl = Integer.parseInt(parts[1]); break;
                        case "fpsLimit": this.fpsLimit = Integer.parseInt(parts[1]); break;
                        case "difficulty": this.difficulty = Integer.parseInt(parts[1]); break;
                        case "fancyGraphics": this.fancyGraphics = parts[1].equals("true"); break;
                        case "ao": this.ao = parts[1].equals("true"); break;
                        case "skin": this.skin = parts[1]; break;
                        case "lastServer": if (parts.length >= 2) this.lastServer = parts[1]; break;
                        case "fov": this.fieldOfView = Float.parseFloat(parts[1]); break;
                        case "ofBrightness": this.ofBrightness = Float.parseFloat(parts[1]); break;
                        case "ofRenderScale": this.ofRenderScale = Float.parseFloat(parts[1]); break;
                        case "ofMipmapLevel": this.ofMipmapLevel = Integer.parseInt(parts[1]); break;
                        case "ofMipmapLinear": this.ofMipmapLinear = parts[1].equals("true"); break;
                        case "ofAoLevel": this.ofAoLevel = Float.parseFloat(parts[1]); break;
                        case "ofClearWater": this.ofClearWater = parts[1].equals("true"); break;
                        case "ofSmoothFps": this.ofSmoothFps = parts[1].equals("true"); break;
                        case "ofSmoothInput": this.ofSmoothInput = parts[1].equals("true"); break;
                        case "ofVBO": this.ofVBO = parts[1].equals("true"); break;
                        case "ofChunkUpdates": this.ofChunkUpdates = Float.parseFloat(parts[1]); break;
                        case "ofChunkUpdatesDynamic": this.ofChunkUpdatesDynamic = parts[1].equals("true"); break;
                        case "ofPreloadedChunks": this.ofPreloadedChunks = Integer.parseInt(parts[1]); break;
                        case "ofEntityDistanceScale": this.ofEntityDistanceScale = Float.parseFloat(parts[1]); break;
                        case "ofClouds": this.ofClouds = Integer.parseInt(parts[1]); break;
                        case "ofCloudsHeight": this.ofCloudsHeight = Float.parseFloat(parts[1]); break;
                        case "ofTrees": this.ofTrees = Integer.parseInt(parts[1]); break;
                        case "ofGrass": this.ofGrass = Integer.parseInt(parts[1]); break;
                        case "ofWater": this.ofWater = Integer.parseInt(parts[1]); break;
                        case "ofRain": this.ofRain = Integer.parseInt(parts[1]); break;
                        case "ofSky": this.ofSky = parts[1].equals("true"); break;
                        case "ofStars": this.ofStars = parts[1].equals("true"); break;
                        case "ofAnimatedWater": this.ofAnimatedWater = Integer.parseInt(parts[1]); break;
                        case "ofAnimatedLava": this.ofAnimatedLava = Integer.parseInt(parts[1]); break;
                        case "ofAnimatedFire": this.ofAnimatedFire = parts[1].equals("true"); break;
                        case "ofAnimatedPortal": this.ofAnimatedPortal = parts[1].equals("true"); break;
                        case "ofAnimatedRedstone": this.ofAnimatedRedstone = parts[1].equals("true"); break;
                        case "ofAnimatedExplosion": this.ofAnimatedExplosion = parts[1].equals("true"); break;
                        case "ofAnimatedFlame": this.ofAnimatedFlame = parts[1].equals("true"); break;
                        case "ofAnimatedSmoke": this.ofAnimatedSmoke = parts[1].equals("true"); break;
                        case "ofWeather": this.ofWeather = parts[1].equals("true"); break;
                        case "ofTime": this.ofTime = Integer.parseInt(parts[1]); break;
                        case "ofAutoSaveTicks": this.ofAutoSaveTicks = Integer.parseInt(parts[1]); break;
                        case "ofFastDebugInfo": this.ofFastDebugInfo = parts[1].equals("true"); break;
                        case "stereoMode": this.stereoMode = Integer.parseInt(parts[1]); break;
                        case "ofStereoOffset": this.ofStereoOffset = Float.parseFloat(parts[1]); break;
                        case "ofStereoSeparation": this.ofStereoSeparation = Float.parseFloat(parts[1]); break;
                        case "ofStereoRedBlueOrder": this.ofStereoRedBlueOrder = parts[1].equals("true"); break;
                        case "ofHandStereoOffset": this.ofHandStereoOffset = Float.parseFloat(parts[1]); break;
                        case "ofHandStereoSeparation": this.ofHandStereoSeparation = Float.parseFloat(parts[1]); break;
                        case "ofHandDepth": this.ofHandDepth = Float.parseFloat(parts[1]); break;
                        case "ofFogFancy": this.ofFogFancy = parts[1].equals("true"); break;
                        case "ofFogProjection": this.ofFogProjection = Integer.parseInt(parts[1]); break;
                        case "ofFogStart": this.ofFogStart = Float.parseFloat(parts[1]); break;
                        case "ofFogMode": this.ofFogMode = Integer.parseInt(parts[1]); break;
                        case "ofFogEnd": this.ofFogEnd = Float.parseFloat(parts[1]); break;
                        case "ofFogDensity": this.ofFogDensity = Float.parseFloat(parts[1]); break;
                        case "ofFogColorRed": this.ofFogColorRed = Float.parseFloat(parts[1]); break;
                        case "ofFogColorGreen": this.ofFogColorGreen = Float.parseFloat(parts[1]); break;
                        case "ofFogColorBlue": this.ofFogColorBlue = Float.parseFloat(parts[1]); break;
                        case "ofFogColorMode": this.ofFogColorMode = Integer.parseInt(parts[1]); break;
                        default: break;
                    }
                    for (int i = 0; i < this.allKeys.length; ++i) {
                        if (!parts[0].equals("key_" + this.allKeys[i].translationKey)) continue;
                        this.allKeys[i].code = Integer.parseInt(parts[1]);
                    }
                } catch (Exception e) {
                    System.out.println("Skipping bad option: " + line);
                }
            }
            br.close();
        } catch (Exception e) {
            System.out.println("Failed to load options");
            e.printStackTrace();
        }
    }

    public void save() {
        try {
            PrintWriter pw = new PrintWriter(new FileWriter(this.file));
            pw.println("music:" + this.musicVolume);
            pw.println("sound:" + this.soundVolume);
            pw.println("invertYMouse:" + this.invertYMouse);
            pw.println("mouseSensitivity:" + this.mouseSensitivity);
            pw.println("viewDistance:" + this.viewDistance);
            pw.println("guiScale:" + this.guiScale);
            pw.println("bobView:" + this.bobView);
            pw.println("anaglyph3d:" + this.anaglyph3d);
            pw.println("advancedOpengl:" + this.advancedOpengl);
            pw.println("fpsLimit:" + this.fpsLimit);
            pw.println("difficulty:" + this.difficulty);
            pw.println("fancyGraphics:" + this.fancyGraphics);
            pw.println("ao:" + this.ao);
            pw.println("skin:" + this.skin);
            pw.println("lastServer:" + this.lastServer);
            pw.println("fov:" + this.fieldOfView);
            pw.println("ofBrightness:" + this.ofBrightness);
            pw.println("ofRenderScale:" + this.ofRenderScale);
            pw.println("ofMipmapLevel:" + this.ofMipmapLevel);
            pw.println("ofMipmapLinear:" + this.ofMipmapLinear);
            pw.println("ofAoLevel:" + this.ofAoLevel);
            pw.println("ofClearWater:" + this.ofClearWater);
            pw.println("ofSmoothFps:" + this.ofSmoothFps);
            pw.println("ofSmoothInput:" + this.ofSmoothInput);
            pw.println("ofVBO:" + this.ofVBO);
            pw.println("ofChunkUpdates:" + this.ofChunkUpdates);
            pw.println("ofChunkUpdatesDynamic:" + this.ofChunkUpdatesDynamic);
            pw.println("ofPreloadedChunks:" + this.ofPreloadedChunks);
            pw.println("ofEntityDistanceScale:" + this.ofEntityDistanceScale);
            pw.println("ofClouds:" + this.ofClouds);
            pw.println("ofCloudsHeight:" + this.ofCloudsHeight);
            pw.println("ofTrees:" + this.ofTrees);
            pw.println("ofGrass:" + this.ofGrass);
            pw.println("ofWater:" + this.ofWater);
            pw.println("ofRain:" + this.ofRain);
            pw.println("ofSky:" + this.ofSky);
            pw.println("ofStars:" + this.ofStars);
            pw.println("ofAnimatedWater:" + this.ofAnimatedWater);
            pw.println("ofAnimatedLava:" + this.ofAnimatedLava);
            pw.println("ofAnimatedFire:" + this.ofAnimatedFire);
            pw.println("ofAnimatedPortal:" + this.ofAnimatedPortal);
            pw.println("ofAnimatedRedstone:" + this.ofAnimatedRedstone);
            pw.println("ofAnimatedExplosion:" + this.ofAnimatedExplosion);
            pw.println("ofAnimatedFlame:" + this.ofAnimatedFlame);
            pw.println("ofAnimatedSmoke:" + this.ofAnimatedSmoke);
            pw.println("ofWeather:" + this.ofWeather);
            pw.println("ofTime:" + this.ofTime);
            pw.println("ofAutoSaveTicks:" + this.ofAutoSaveTicks);
            pw.println("ofFastDebugInfo:" + this.ofFastDebugInfo);
            pw.println("stereoMode:" + this.stereoMode);
            pw.println("ofStereoOffset:" + this.ofStereoOffset);
            pw.println("ofStereoSeparation:" + this.ofStereoSeparation);
            pw.println("ofStereoRedBlueOrder:" + this.ofStereoRedBlueOrder);
            pw.println("ofHandStereoOffset:" + this.ofHandStereoOffset);
            pw.println("ofHandStereoSeparation:" + this.ofHandStereoSeparation);
            pw.println("ofHandDepth:" + this.ofHandDepth);
            pw.println("ofFogFancy:" + this.ofFogFancy);
            pw.println("ofFogProjection:" + this.ofFogProjection);
            pw.println("ofFogStart:" + this.ofFogStart);
            pw.println("ofFogMode:" + this.ofFogMode);
            pw.println("ofFogEnd:" + this.ofFogEnd);
            pw.println("ofFogDensity:" + this.ofFogDensity);
            pw.println("ofFogColorRed:" + this.ofFogColorRed);
            pw.println("ofFogColorGreen:" + this.ofFogColorGreen);
            pw.println("ofFogColorBlue:" + this.ofFogColorBlue);
            pw.println("ofFogColorMode:" + this.ofFogColorMode);
            for (int i = 0; i < this.allKeys.length; ++i) {
                pw.println("key_" + this.allKeys[i].translationKey + ":" + this.allKeys[i].code);
            }
            pw.close();
        } catch (Exception e) {
            System.out.println("Failed to save options");
            e.printStackTrace();
        }
    }
}
