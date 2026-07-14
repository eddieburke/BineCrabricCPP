#include "net/minecraft/server/ServerProperties.hpp"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include "net/minecraft/server/ServerLog.hpp"
namespace net::minecraft::server {
namespace {
Logger& propertiesLogger() {
  return ServerLog::LOGGER;
}
std::string trimCopy(std::string value) {
  const auto notSpace = [](unsigned char c) { return !std::isspace(c); };
  value.erase(value.begin(), std::find_if(value.begin(), value.end(), notSpace));
  value.erase(std::find_if(value.rbegin(), value.rend(), notSpace).base(), value.end());
  return value;
}
} // namespace
ServerProperties::ServerProperties(const std::filesystem::path& file) : propertiesFile_(file), persistToFile_(true) {
  if(std::filesystem::exists(propertiesFile_)) {
    try {
      std::ifstream input(propertiesFile_);
      std::string line;
      while(std::getline(input, line)) {
        if(line.empty() || line[0] == '#') {
          continue;
        }
        const std::size_t separator = line.find('=');
        if(separator == std::string::npos) {
          continue;
        }
        std::string key = trimCopy(line.substr(0, separator));
        std::string value = trimCopy(line.substr(separator + 1));
        if(!key.empty()) {
          properties_.emplace(std::move(key), std::move(value));
        }
      }
    } catch(const std::exception& exception) {
      propertiesLogger().log(LogLevel::Warning, "Failed to load " + propertiesFile_.string(), &exception);
      generateNew();
    }
  } else {
    propertiesLogger().log(LogLevel::Warning, propertiesFile_.string() + " does not exist");
    generateNew();
  }
}
void ServerProperties::generateNew() {
  propertiesLogger().log(LogLevel::Info, "Generating new properties file");
  save();
}
void ServerProperties::save() {
  if(!persistToFile_) {
    return;
  }
  try {
    std::ofstream output(propertiesFile_);
    output << "#Minecraft server properties\n";
    for(const auto& entry : properties_) {
      output << entry.first << '=' << entry.second << '\n';
    }
  } catch(const std::exception& exception) {
    propertiesLogger().log(LogLevel::Warning, "Failed to save " + propertiesFile_.string(), &exception);
  }
}
std::string ServerProperties::getProperty(const std::string& property, const std::string& fallback) {
  const auto existing = properties_.find(property);
  if(existing == properties_.end()) {
    properties_.emplace(property, fallback);
    save();
    return fallback;
  }
  return existing->second;
}
int ServerProperties::getProperty(const std::string& property, int fallback) {
  try {
    return std::stoi(getProperty(property, std::to_string(fallback)));
  } catch(const std::exception&) {
    properties_.insert_or_assign(property, std::to_string(fallback));
    return fallback;
  }
}
bool ServerProperties::getProperty(const std::string& property, bool fallback) {
  const std::string value = getProperty(property, std::string(fallback ? "true" : "false"));
  if(value == "true") {
    return true;
  }
  if(value == "false") {
    return false;
  }
  properties_.insert_or_assign(property, fallback ? "true" : "false");
  return fallback;
}
void ServerProperties::setProperty(const std::string& property, const std::string& value) {
  properties_.insert_or_assign(property, value);
  save();
}
void ServerProperties::setProperty(const std::string& property, int value) {
  properties_.insert_or_assign(property, std::to_string(value));
  save();
}
void ServerProperties::setProperty(const std::string& property, bool value) {
  properties_.insert_or_assign(property, value ? "true" : "false");
  save();
}
} // namespace net::minecraft::server
