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
  int fpsLimit = 1;
  bool fancyGraphics = true;
  bool ao = true;
  bool frustumCulling = true;
  std::string skin = "Default";
  int difficulty = 2;
  int guiScale = 0;
  float fieldOfView = 0.0f;
  // Quality
  float renderScale = 1.0f;
  int mipmapLevel = 0;
  bool mipmapLinear = false;
  float aoLevel = 1.0f;
  float brightness = 0.0f;
  bool clearWater = false;
  // Performance
  bool smoothFps = false;
  bool smoothInput = false;
  bool entityShadows = true;
  bool vbo = false;
  float chunkUpdates = 0.5f;
  bool chunkUpdatesDynamic = false;
  int preloadedChunks = 0;
  float entityDistanceScale = 1.0f;
  // Details
  int clouds = 0;
  float cloudsHeight = 0.0f;
  int trees = 0;
  int grass = 0;
  int water = 0;
  int rain = 0;
  bool sky = true;
  bool stars = true;
  // Animations
  int animatedWater = 0;
  int animatedLava = 0;
  bool animatedFire = true;
  bool animatedPortal = true;
  bool animatedRedstone = true;
  bool animatedExplosion = true;
  bool animatedFlame = true;
  bool animatedSmoke = true;
  // World / misc
  bool weather = true;
  int time = 0;
  int autoSaveTicks = 4000;
  bool fastDebugInfo = false;
  // Fog
  bool fogFancy = false;
  int fogProjection = 0;
  float fogStart = 0.2f;
  int fogMode = 0;
  float fogEnd = 0.8f;
  float fogDensity = 0.1f;
  float fogColorRed = 0.0f;
  float fogColorGreen = 0.0f;
  float fogColorBlue = 0.0f;
  int fogColorMode = 0;
  bool hideHud = false;
  bool thirdPerson = false;
  bool debugHud = false;
  bool debugCamera = false;
  bool cinematicMode = false;
  bool discreteScroll = false;
  float totalDiscreteScroll = 1.0f;
  std::string lastServer;
  bool modsEnabled = true;
  KeyBinding forwardKey{"key.forward", 17};
  KeyBinding leftKey{"key.left", 30};
  KeyBinding backKey{"key.back", 31};
  KeyBinding rightKey{"key.right", 32};
  KeyBinding jumpKey{"key.jump", 57};
  KeyBinding inventoryKey{"key.inventory", 18};
  KeyBinding dropKey{"key.drop", 16};
  KeyBinding chatKey{"key.chat", 20};
  KeyBinding fogKey{"key.fog", 33};
  KeyBinding sneakKey{"key.sneak", 42};
  KeyBinding* allKeys[10] = {
      &forwardKey, &leftKey, &backKey,
      &rightKey, &jumpKey,
      &sneakKey, &dropKey, &inventoryKey,
      &chatKey, &fogKey};
  std::filesystem::path optionsFile;
  Minecraft* minecraft = nullptr;
  void bindMinecraft(Minecraft* client) {
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
