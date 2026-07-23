#include <gtest/gtest.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include "net/minecraft/mod/ModSettingsRegistry.hpp"
TEST(ModSettingsRegistry, AppliesValuesLoadedBeforeRegistration) {
 const auto suffix = std::chrono::steady_clock::now().time_since_epoch().count();
 const auto path = std::filesystem::temp_directory_path() /
                   ("minecraft_native_mod_settings_" + std::to_string(suffix) + ".txt");
 {
  std::ofstream output(path);
  output << "settings_registry_test.speed=1.75\n";
  output << "key_settings_registry_test.action=42\n";
  output << "settings_registry_test.quality=High\n";
 }
 auto& registry = net::minecraft::mod::ModSettingsRegistry::instance();
 registry.load(path);
 net::minecraft::mod::ModSettingDef setting;
 setting.key = "speed";
 setting.sliderMin = 1.0f;
 setting.sliderMax = 2.0f;
 setting.floatDefault = 1.0f;
 setting.floatCurrent = setting.floatDefault;
 registry.registerSetting("settings_registry_test", "Settings Registry Test", setting);
 const auto* loadedSetting = registry.findSetting("settings_registry_test", "speed");
 ASSERT_NE(loadedSetting, nullptr);
 EXPECT_FLOAT_EQ(loadedSetting->floatCurrent, 1.75f);
 net::minecraft::mod::ModKeybindDef keybind;
 keybind.id = "settings_registry_test.action";
 keybind.defaultKeyCode = 1;
 keybind.currentKeyCode = 1;
 registry.registerKeybind("settings_registry_test", keybind);
 const auto* loadedKeybind = registry.findKeybind("settings_registry_test.action");
 ASSERT_NE(loadedKeybind, nullptr);
 EXPECT_EQ(loadedKeybind->currentKeyCode, 42);
  net::minecraft::mod::ModSettingDef optSetting;
  optSetting.key = "quality";
  optSetting.kind = net::minecraft::mod::ModSettingDef::Options;
  optSetting.options = {"Low", "Medium", "High"};
  optSetting.optionDefault = 0;
  optSetting.optionCurrent = 0;
  registry.registerSetting("settings_registry_test", "Settings Registry Test", optSetting);
  const auto* loadedOpt = registry.findSetting("settings_registry_test", "quality");
  ASSERT_NE(loadedOpt, nullptr);
  EXPECT_EQ(loadedOpt->kind, net::minecraft::mod::ModSettingDef::Options);
  EXPECT_EQ(loadedOpt->optionCurrent, 2);
  std::error_code error;
  std::filesystem::remove(path, error);
}
