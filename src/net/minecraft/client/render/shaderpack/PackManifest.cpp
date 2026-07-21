#include "net/minecraft/client/render/shaderpack/PackManifest.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <string_view>
#include "net/minecraft/mod/model/JsonValue.hpp"
namespace net::minecraft::client::render::shaderpack {
namespace {
using net::minecraft::mod::model::JsonValue;
const JsonValue& field(const JsonValue& value, std::string_view key) {
 return value[key];
}
std::string stringField(const JsonValue& value, std::string_view key, std::string fallback = {}) {
 const JsonValue& item = field(value, key);
 return item.isString() ? item.asString() : std::move(fallback);
}
double numberField(const JsonValue& value, std::string_view key, double fallback) {
 const JsonValue& item = field(value, key);
 return item.isNumber() ? item.asNumber() : fallback;
}
bool validIdentifier(std::string_view value) {
 if(value.empty() || (!std::isalpha(static_cast<unsigned char>(value.front())) && value.front() != '_')) {
  return false;
 }
 return std::all_of(value.begin() + 1, value.end(), [](unsigned char ch) { return std::isalnum(ch) || ch == '_'; });
}
bool normalizeDefault(PackSetting& setting) {
 if(setting.type == SettingType::Bool) {
  std::string value = setting.defaultValue;
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
  if(value == "1" || value == "true" || value == "on") {
   setting.defaultValue = "1";
   return true;
  }
  if(value == "0" || value == "false" || value == "off") {
   setting.defaultValue = "0";
   return true;
  }
  return false;
 }
 char* end = nullptr;
 double value = std::strtod(setting.defaultValue.c_str(), &end);
 if(end == setting.defaultValue.c_str() || *end != '\0' || !std::isfinite(value)) {
  return false;
 }
 value = std::clamp(value, setting.minimum, setting.maximum);
 value = setting.minimum + std::round((value - setting.minimum) / setting.step) * setting.step;
 value = std::clamp(value, setting.minimum, setting.maximum);
 setting.defaultValue = setting.type == SettingType::Int ? std::to_string(static_cast<int>(std::lround(value)))
                                                         : std::to_string(value);
 return true;
}
} // namespace
bool PackManifest::parse(const std::string& text, PackManifest& out, std::string& error) {
 out = PackManifest{};
 JsonValue root;
 if(!JsonValue::parse(text, root, error) || !root.isObject()) {
  if(error.empty()) {
   error = "pack.json root must be an object";
  }
  return false;
 }
 out.name = stringField(root, "name", out.name);
 out.version = stringField(root, "version", out.version);
 const JsonValue& settings = field(root, "settings");
 if(settings.isArray()) {
  for(std::size_t i = 0; i < settings.size(); ++i) {
   const JsonValue& value = settings.at(i);
   if(!value.isObject()) {
    continue;
   }
   PackSetting setting;
   setting.key = stringField(value, "key");
   const std::string type = stringField(value, "type", "bool");
   if(type == "int") {
    setting.type = SettingType::Int;
    setting.step = 1.0;
   } else if(type == "float") {
    setting.type = SettingType::Float;
   } else if(type == "bool") {
    setting.type = SettingType::Bool;
    setting.step = 1.0;
   } else {
    continue;
   }
   if(!validIdentifier(setting.key) ||
      std::any_of(out.settings.begin(), out.settings.end(), [&](const PackSetting& current) { return current.key == setting.key; })) {
    continue;
   }
   setting.label = stringField(value, "label", setting.key);
   setting.minimum = numberField(value, "min", setting.type == SettingType::Bool ? 0.0 : setting.minimum);
   setting.maximum = numberField(value, "max", setting.type == SettingType::Bool ? 1.0 : setting.maximum);
   setting.step = numberField(value, "step", setting.step);
   const JsonValue& defaultValue = field(value, "default");
   if(defaultValue.isString()) {
    setting.defaultValue = defaultValue.asString();
   } else if(defaultValue.isNumber()) {
    setting.defaultValue = setting.type == SettingType::Int
                               ? std::to_string(static_cast<int>(std::lround(defaultValue.asNumber())))
                               : std::to_string(defaultValue.asNumber());
   } else if(defaultValue.type() == JsonValue::Type::Boolean) {
    setting.defaultValue = defaultValue.asBool() ? "1" : "0";
   } else {
    setting.defaultValue = setting.type == SettingType::Bool ? "1" : "0";
   }
   if(!std::isfinite(setting.minimum) || !std::isfinite(setting.maximum) || !std::isfinite(setting.step) ||
      setting.maximum < setting.minimum || setting.step <= 0.0) {
    error = "invalid range for setting " + setting.key;
    return false;
   }
   if(!normalizeDefault(setting)) {
    error = "invalid default for setting " + setting.key;
    return false;
   }
   out.settings.push_back(std::move(setting));
  }
 }
 const JsonValue& programs = field(root, "programs");
 if(programs.isObject()) {
  for(const auto& [key, value] : programs.members()) {
   if(!value.isObject()) {
    continue;
   }
   PackProgram program;
   program.stage = stringField(value, "stage");
   program.vertex = stringField(value, "vsh");
   program.fragment = stringField(value, "fsh");
   if(key.empty() || out.programs.contains(key)) {
    error = "duplicate or empty program name";
    return false;
   }
   if(program.stage != "post" && program.stage != "terrain" && program.stage != "entities" &&
      program.stage != "sky") {
    error = "program " + key + " has invalid or missing stage";
    return false;
   }
   if(program.vertex.empty() || program.fragment.empty()) {
    error = "program " + key + " has incomplete sources";
    return false;
   }
   out.programs.emplace(key, std::move(program));
  }
 }
 const JsonValue& targets = field(root, "targets");
 if(targets.isObject()) {
  for(const auto& [key, value] : targets.members()) {
   if(!value.isObject()) {
    continue;
   }
   PackTarget target;
   target.format = stringField(value, "format", target.format);
   target.scale = static_cast<float>(std::clamp(numberField(value, "scale", 1.0), 0.05, 4.0));
   if(key.empty() || out.targets.contains(key) || (target.format != "RGBA8" && target.format != "RGBA16F")) {
    error = "invalid target declaration";
    return false;
   }
   out.targets.emplace(key, target);
  }
 }
 const JsonValue& passes = field(root, "passes");
 if(passes.isArray()) {
  const auto validResource = [&out](const std::string& resource) {
   return resource.empty() || resource == "screen" || resource == "colortex0" || resource == "depthtex0" ||
          out.targets.contains(resource);
  };
  for(std::size_t i = 0; i < passes.size(); ++i) {
   const JsonValue& value = passes.at(i);
   if(!value.isObject()) {
    continue;
   }
   PackPass pass;
   pass.name = stringField(value, "name", "pass" + std::to_string(i));
   pass.type = stringField(value, "type", "post");
   pass.program = stringField(value, "program");
   pass.output = stringField(value, "output", "screen");
   const JsonValue& inputs = field(value, "inputs");
   if(inputs.isArray()) {
    for(std::size_t input = 0; input < inputs.size(); ++input) {
     if(inputs.at(input).isString()) {
      pass.inputs.push_back(inputs.at(input).asString());
     }
    }
   }
   if(pass.program.empty() || !out.programs.contains(pass.program)) {
    error = "pass " + pass.name + " references no complete program";
    return false;
   }
   const PackProgram& programSpec = out.programs.at(pass.program);
   if(pass.type == "terrain") {
    if(programSpec.stage != "terrain") {
     error = "pass " + pass.name + " must use a terrain program";
     return false;
    }
    if(programSpec.vertex.empty() || programSpec.fragment.empty()) {
     error = "pass " + pass.name + " terrain program has incomplete sources";
     return false;
    }
   } else if(pass.type == "post") {
    if(programSpec.stage != "post") {
     error = "pass " + pass.name + " must use a post program";
     return false;
    }
    if(!validResource(pass.output)) {
     error = "pass " + pass.name + " references undeclared output target";
     return false;
    }
    if(std::any_of(pass.inputs.begin(), pass.inputs.end(), [&validResource](const std::string& input) {
        return !validResource(input);
       })) {
     error = "pass " + pass.name + " references undeclared input target";
     return false;
    }
   } else {
    error = "pass " + pass.name + " has invalid type";
    return false;
   }
   out.passes.push_back(std::move(pass));
  }
 }
 if(out.programs.empty()) {
  error = "pack.json declares no complete programs";
  return false;
 }
 out.valid_ = true;
 return true;
}
} // namespace net::minecraft::client::render::shaderpack
