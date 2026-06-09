#pragma once



#include "net/minecraft/client/option/ApplyFlags.hpp"
#include "net/minecraft/client/option/KeyBinding.hpp"



#include <cstdint>

#include <filesystem>

#include <fstream>

#include <sstream>

#include <string>

#include <string_view>



namespace net::minecraft {

class World;

} // namespace net::minecraft



namespace net::minecraft::client {

class Minecraft;

} // namespace net::minecraft::client



namespace net::minecraft::client::option {



class GameOptions {

public:

    float musicVolume = 1.0f;

    float soundVolume = 1.0f;

    float mouseSensitivity = 0.5f;

    bool invertYMouse = false;

    int viewDistance = 0;

    bool bobView = true;

    int stereoMode = 0;

    int advancedOpengl = 0;

    int fpsLimit = 1;



    [[nodiscard]] bool isStereoActive() const noexcept { return stereoMode != 0; }

    [[nodiscard]] bool isAnaglyphActive() const noexcept { return stereoMode == 1; }

    [[nodiscard]] bool isSideBySideActive() const noexcept { return stereoMode == 2; }

    bool fancyGraphics = true;

    bool ao = true;

    bool frustumCulling = true;

    std::string skin = "Default";

    int difficulty = 2;

    int guiScale = 0;

    float fieldOfView = 0.0f;



    // Quality

    float ofRenderScale = 1.0f;

    int ofMipmapLevel = 0;

    bool ofMipmapLinear = false;

    float ofAoLevel = 1.0f;

    float ofBrightness = 0.0f;

    bool ofClearWater = false;



    // Performance

    bool ofSmoothFps = false;

    bool ofSmoothInput = false;

    bool ofVBO = false;

    float ofChunkUpdates = 0.5f;

    bool ofChunkUpdatesDynamic = false;

    int ofPreloadedChunks = 0;

    float ofEntityDistanceScale = 1.0f;



    // Details

    int ofClouds = 0;

    float ofCloudsHeight = 0.0f;

    int ofTrees = 0;

    int ofGrass = 0;

    int ofWater = 0;

    int ofRain = 0;

    bool ofSky = true;

    bool ofStars = true;



    // Animations

    int ofAnimatedWater = 0;

    int ofAnimatedLava = 0;

    bool ofAnimatedFire = true;

    bool ofAnimatedPortal = true;

    bool ofAnimatedRedstone = true;

    bool ofAnimatedExplosion = true;

    bool ofAnimatedFlame = true;

    bool ofAnimatedSmoke = true;



    // World / misc

    bool ofWeather = true;

    int ofTime = 0;

    int ofAutoSaveTicks = 4000;

    bool ofFastDebugInfo = false;



    // Stereo / 3D

    float ofStereoOffset = 0.07f;

    float ofStereoSeparation = 0.1f;

    bool ofStereoRedBlueOrder = false;

    float ofHandStereoOffset = 0.07f;

    float ofHandStereoSeparation = 0.1f;

    float ofHandDepth = 0.0f;



    // Fog

    bool ofFogFancy = false;

    int ofFogProjection = 0;

    float ofFogStart = 0.2f;

    int ofFogMode = 0;

    float ofFogEnd = 0.8f;

    float ofFogDensity = 0.1f;

    float ofFogColorRed = 0.0f;

    float ofFogColorGreen = 0.0f;

    float ofFogColorBlue = 0.0f;

    int ofFogColorMode = 0;



    bool hideHud = false;

    bool thirdPerson = false;

    bool debugHud = false;

    bool debugCamera = false;

    bool cinematicMode = false;

    bool discreteScroll = false;

    float totalDiscreteScroll = 1.0f;



    std::string lastServer;



    KeyBinding forwardKey  {"key.forward",   17};

    KeyBinding leftKey     {"key.left",       30};

    KeyBinding backKey     {"key.back",       31};

    KeyBinding rightKey    {"key.right",      32};

    KeyBinding jumpKey     {"key.jump",       57};

    KeyBinding inventoryKey{"key.inventory",  18};

    KeyBinding dropKey     {"key.drop",       16};

    KeyBinding chatKey     {"key.chat",       20};

    KeyBinding fogKey      {"key.fog",        33};

    KeyBinding sneakKey    {"key.sneak",      42};



    KeyBinding* allKeys[10] = {

        &forwardKey, &leftKey, &backKey, &rightKey, &jumpKey,

        &sneakKey, &dropKey, &inventoryKey, &chatKey, &fogKey

    };



    std::filesystem::path optionsFile;

    Minecraft* minecraft = nullptr;



    void bindMinecraft(Minecraft* client)

    {

        minecraft = client;

    }



    void load();

    void save();



    void applyToWorld(net::minecraft::World* world) const;



    void cycle(std::string_view persistKey, int delta);

    void setFloat(std::string_view persistKey, float value);



    [[nodiscard]] float getFloat(std::string_view persistKey) const;

    [[nodiscard]] bool getBoolean(std::string_view persistKey) const;

    [[nodiscard]] std::string getString(std::string_view persistKey) const;

    [[nodiscard]] std::string getKeybindName(int index) const;

    [[nodiscard]] std::string getKeybindKey(int index) const;

    void setKeybindKey(int index, int keyCode);



    static constexpr int kKeybindCount = 10;



private:

    void applyDerivedSettings();

    void applySideEffects(ApplyFlags flags);

};



} // namespace net::minecraft::client::option

