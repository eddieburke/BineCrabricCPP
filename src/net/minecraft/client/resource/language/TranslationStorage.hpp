#pragma once
#include <string>
#include <string_view>
#include <unordered_map>

#include "net/minecraft/client/resource/ResourcePack.hpp"

namespace net::minecraft::client::resource::language {
class TranslationStorage {
   public:
    explicit TranslationStorage(const ResourcePack& resources);
    [[nodiscard]] std::string get(std::string_view key) const;
    [[nodiscard]] std::string getClientTranslation(std::string_view key) const;

   private:
    void loadProperties(const std::string& text);
    [[nodiscard]] static std::string unescapePropertiesValue(std::string_view value);
    [[nodiscard]] static std::string decodeUnicodeEscape(std::string_view digits);
    std::unordered_map<std::string, std::string> translations_;
};
}  // namespace net::minecraft::client::resource::language
