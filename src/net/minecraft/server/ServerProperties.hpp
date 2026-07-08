#pragma once
#include <filesystem>
#include <map>
#include <string>

namespace net::minecraft::server {
class ServerProperties {
   public:
    ServerProperties() = default;
    explicit ServerProperties(const std::filesystem::path& file);
    void generateNew();
    void save();
    [[nodiscard]] std::string getProperty(const std::string& property, const std::string& fallback);
    [[nodiscard]] int getProperty(const std::string& property, int fallback);
    [[nodiscard]] bool getProperty(const std::string& property, bool fallback);

    [[nodiscard]] bool persistsToFile() const noexcept {
        return persistToFile_;
    }

    void setProperty(const std::string& property, const std::string& value);
    void setProperty(const std::string& property, int value);
    void setProperty(const std::string& property, bool value);

   private:
    std::map<std::string, std::string> properties_;
    std::filesystem::path propertiesFile_;
    bool persistToFile_ = false;
};
}  // namespace net::minecraft::server
