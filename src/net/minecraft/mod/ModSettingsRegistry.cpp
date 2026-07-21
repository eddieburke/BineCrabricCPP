#include "net/minecraft/mod/ModSettingsRegistry.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
namespace net::minecraft::mod {
namespace {
void applyPersistedValue(ModSettingDef& setting, const std::string& value) {
 if(setting.kind == ModSettingDef::Slider) {
  try {
   setting.floatCurrent = std::clamp(std::stof(value), setting.sliderMin, setting.sliderMax);
   if(setting.sliderInteger) {
    setting.floatCurrent = std::round(setting.floatCurrent);
   }
  } catch(...) {
  }
 } else {
  setting.boolCurrent = value == "true" || value == "1";
 }
}
} // namespace
ModSettingsRegistry& ModSettingsRegistry::instance() {
 static ModSettingsRegistry s;
 return s;
}
void ModSettingsRegistry::registerSetting(const std::string& modId, const std::string& modName, ModSettingDef setting) {
 auto& settings = settings_[modId];
 const auto persisted = persistedValues_.find(modId + "." + setting.key);
 if(persisted != persistedValues_.end()) {
  applyPersistedValue(setting, persisted->second);
 }
 const auto existing = std::find_if(settings.begin(), settings.end(), [&setting](const ModSettingDef& value) {
  return value.key == setting.key;
 });
 if(existing != settings.end()) {
  *existing = std::move(setting);
 } else {
  settings.push_back(std::move(setting));
 }
 if(!modName.empty()) {
  modNames_[modId] = modName;
 }
}
void ModSettingsRegistry::registerKeybind(const std::string& modId, ModKeybindDef keybind) {
 const auto persisted = persistedValues_.find("key_" + keybind.id);
 if(persisted != persistedValues_.end()) {
  try {
   keybind.currentKeyCode = std::stoi(persisted->second);
  } catch(...) {
  }
 }
 const auto existing = std::find_if(keybinds_.begin(), keybinds_.end(), [&keybind](const ModKeybindDef& value) {
  return value.id == keybind.id;
 });
 if(existing != keybinds_.end()) {
  *existing = std::move(keybind);
 } else {
  keybinds_.push_back(std::move(keybind));
 }
 (void)modId;
}
const std::vector<ModSettingDef>& ModSettingsRegistry::getSettings(const std::string& modId) const {
 static const std::vector<ModSettingDef> kEmpty;
 auto it = settings_.find(modId);
 return it != settings_.end() ? it->second : kEmpty;
}
const std::unordered_map<std::string, std::string>& ModSettingsRegistry::getModNames() const {
 return modNames_;
}
std::vector<std::pair<std::string, std::vector<ModSettingDef*>>> ModSettingsRegistry::getAllSettings() {
 std::vector<std::pair<std::string, std::vector<ModSettingDef*>>> result;
 result.reserve(settings_.size());
 for(auto& [modId, vec] : settings_) {
  std::vector<ModSettingDef*> ptrs;
  ptrs.reserve(vec.size());
  for(auto& s : vec) {
   ptrs.push_back(&s);
  }
  result.emplace_back(modId, std::move(ptrs));
 }
 std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
 return result;
}
ModSettingDef* ModSettingsRegistry::findSetting(const std::string& modId, const std::string& key) {
 auto it = settings_.find(modId);
 if(it == settings_.end()) {
  return nullptr;
 }
 for(auto& s : it->second) {
  if(s.key == key) {
   return &s;
  }
 }
 return nullptr;
}
ModKeybindDef* ModSettingsRegistry::findKeybind(const std::string& id) {
 for(auto& kb : keybinds_) {
  if(kb.id == id) {
   return &kb;
  }
 }
 return nullptr;
}
std::vector<ModKeybindDef*> ModSettingsRegistry::getAllKeybinds() {
 std::vector<ModKeybindDef*> result;
 result.reserve(keybinds_.size());
 for(auto& kb : keybinds_) {
  result.push_back(&kb);
 }
 return result;
}
int ModSettingsRegistry::findKeybindByKey(int keyCode) const {
 for(const auto& kb : keybinds_) {
  if(kb.currentKeyCode == keyCode) {
   return 1;
  }
 }
 return 0;
}
float ModSettingsRegistry::getFloatValue(const std::string& modId, const std::string& key, float defaultValue) const {
 auto* setting = const_cast<ModSettingsRegistry*>(this)->findSetting(modId, key);
 if(setting && setting->kind == ModSettingDef::Slider) {
  return setting->floatCurrent;
 }
 return defaultValue;
}
bool ModSettingsRegistry::getBoolValue(const std::string& modId, const std::string& key, bool defaultValue) const {
 auto* setting = const_cast<ModSettingsRegistry*>(this)->findSetting(modId, key);
 if(setting && setting->kind == ModSettingDef::Toggle) {
  return setting->boolCurrent;
 }
 return defaultValue;
}
void ModSettingsRegistry::setFloatValue(const std::string& modId, const std::string& key, float value) {
 auto* setting = findSetting(modId, key);
 if(setting && setting->kind == ModSettingDef::Slider) {
  setting->floatCurrent = std::clamp(value, setting->sliderMin, setting->sliderMax);
  save();
 }
}
void ModSettingsRegistry::setBoolValue(const std::string& modId, const std::string& key, bool value) {
 auto* setting = findSetting(modId, key);
 if(setting && setting->kind == ModSettingDef::Toggle) {
  setting->boolCurrent = value;
  save();
 }
}
void ModSettingsRegistry::load(const std::filesystem::path& path) {
 storagePath_ = path;
 persistedValues_.clear();
 std::ifstream in(storagePath_);
 if(!in.is_open()) {
  return;
 }
 std::string line;
 while(std::getline(in, line)) {
  if(line.empty() || line[0] == '#') {
   continue;
  }
  auto eq = line.find('=');
  if(eq == std::string::npos) {
   continue;
  }
  std::string key = line.substr(0, eq);
  std::string value = line.substr(eq + 1);
  persistedValues_[key] = value;
  if(key.rfind("key_", 0) == 0) {
   std::string kbId = key.substr(4);
   auto* kb = findKeybind(kbId);
   if(kb != nullptr) {
    try {
     kb->currentKeyCode = std::stoi(value);
    } catch(...) {
    }
   }
  } else {
   auto dot = key.find('.');
   if(dot != std::string::npos) {
    std::string modId = key.substr(0, dot);
    std::string sKey = key.substr(dot + 1);
    auto* s = findSetting(modId, sKey);
    if(s != nullptr) {
     applyPersistedValue(*s, value);
    }
   }
  }
 }
}
void ModSettingsRegistry::save() {
 std::ofstream out(storagePath_);
 if(!out.is_open()) {
  return;
 }
 out << "# Auto-generated mod settings - do not edit manually\n";
 for(const auto& [modId, settings] : settings_) {
  for(const auto& s : settings) {
   if(s.kind == ModSettingDef::Slider) {
    out << modId << "." << s.key << "=" << s.floatCurrent << "\n";
   } else {
    out << modId << "." << s.key << "=" << (s.boolCurrent ? "true" : "false") << "\n";
   }
  }
 }
 for(const auto& kb : keybinds_) {
  out << "key_" << kb.id << "=" << kb.currentKeyCode << "\n";
 }
}
} // namespace net::minecraft::mod
