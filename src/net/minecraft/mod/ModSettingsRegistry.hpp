#pragma once
#include <filesystem>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
namespace net::minecraft::mod {
struct ModSettingDef {
 std::string key;
 std::string label;
 enum Kind { Slider,
             Toggle,
             Options } kind = Slider;
 float sliderMin = 0.0f;
 float sliderMax = 1.0f;
 float sliderStep = 0.0f;
 int sliderDecimals = 2;
 bool sliderInteger = false;
 float floatDefault = 0.0f;
 bool boolDefault = false;
 float floatCurrent = 0.0f;
 bool boolCurrent = false;
 std::vector<std::string> options;
 int optionDefault = 0;
 int optionCurrent = 0;
};
struct ModKeybindDef {
 std::string id;
 std::string label;
 int defaultKeyCode = 0;
 int currentKeyCode = 0;
};
class ModSettingsRegistry {
 public:
 static ModSettingsRegistry& instance();
 void registerSetting(const std::string& modId, const std::string& modName, ModSettingDef setting);
 void registerKeybind(const std::string& modId, ModKeybindDef keybind);
 [[nodiscard]] const std::vector<ModSettingDef>& getSettings(const std::string& modId) const;
 [[nodiscard]] const std::unordered_map<std::string, std::string>& getModNames() const;
 [[nodiscard]] std::vector<std::pair<std::string, std::vector<ModSettingDef*>>> getAllSettings();
 [[nodiscard]] ModSettingDef* findSetting(const std::string& modId, const std::string& key);
 [[nodiscard]] ModKeybindDef* findKeybind(const std::string& id);
 [[nodiscard]] std::vector<ModKeybindDef*> getAllKeybinds();
 [[nodiscard]] int findKeybindByKey(int keyCode) const;
 void load(const std::filesystem::path& path = "mod_settings.txt");
 void save();
 [[nodiscard]] float getFloatValue(const std::string& modId, const std::string& key, float defaultValue) const;
 [[nodiscard]] bool getBoolValue(const std::string& modId, const std::string& key, bool defaultValue) const;
 [[nodiscard]] std::string getOptionValue(const std::string& modId, const std::string& key, const std::string& defaultValue) const;
 void setFloatValue(const std::string& modId, const std::string& key, float value);
 void setBoolValue(const std::string& modId, const std::string& key, bool value);
 void setOptionValue(const std::string& modId, const std::string& key, const std::string& value);
 void markKeyPressed(int keyCode);
 bool consumeKeybind(const std::string& id);

 private:
 ModSettingsRegistry() = default;
 std::unordered_map<std::string, std::vector<ModSettingDef>> settings_;
 std::unordered_map<std::string, std::string> modNames_;
 std::vector<ModKeybindDef> keybinds_;
 std::unordered_set<int> pendingKeys_;
 std::filesystem::path storagePath_ = "mod_settings.txt";
 std::unordered_map<std::string, std::string> persistedValues_;
};
} // namespace net::minecraft::mod
