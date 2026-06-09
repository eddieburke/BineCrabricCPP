#include "net/minecraft/client/resource/language/TranslationStorage.hpp"

#include <cctype>
#include <sstream>
#include <string>

namespace net::minecraft::client::resource::language {

TranslationStorage::TranslationStorage(const ResourcePack& resources)
{
    loadProperties(resources.readText("lang/en_US.lang"));
    loadProperties(resources.readText("lang/stats_US.lang"));
}

std::string TranslationStorage::get(std::string_view key) const
{
    const auto it = translations_.find(std::string(key));
    if (it != translations_.end()) {
        return it->second;
    }
    return std::string(key);
}

std::string TranslationStorage::getClientTranslation(std::string_view key) const
{
    const std::string lookup = std::string(key) + ".name";
    const auto it = translations_.find(lookup);
    if (it != translations_.end()) {
        return it->second;
    }
    return {};
}

std::string TranslationStorage::decodeUnicodeEscape(std::string_view digits)
{
    unsigned long codePoint = 0;
    for (const char ch : digits) {
        if (!std::isxdigit(static_cast<unsigned char>(ch))) {
            return {};
        }
        codePoint = codePoint * 16U
            + static_cast<unsigned long>(std::isdigit(static_cast<unsigned char>(ch)) != 0 ? ch - '0'
                : (std::tolower(static_cast<unsigned char>(ch)) - 'a' + 10));
    }
    if (codePoint <= 0x7FU) {
        return std::string(1, static_cast<char>(codePoint));
    }
    if (codePoint <= 0x7FFU) {
        std::string out(2, '\0');
        out[0] = static_cast<char>(0xC0U | ((codePoint >> 6U) & 0x1FU));
        out[1] = static_cast<char>(0x80U | (codePoint & 0x3FU));
        return out;
    }
    std::string out(3, '\0');
    out[0] = static_cast<char>(0xE0U | ((codePoint >> 12U) & 0x0FU));
    out[1] = static_cast<char>(0x80U | ((codePoint >> 6U) & 0x3FU));
    out[2] = static_cast<char>(0x80U | (codePoint & 0x3FU));
    return out;
}

std::string TranslationStorage::unescapePropertiesValue(std::string_view value)
{
    std::string out;
    out.reserve(value.size());
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (value[i] != '\\') {
            out.push_back(value[i]);
            continue;
        }
        if (i + 1 >= value.size()) {
            break;
        }
        const char escape = value[++i];
        switch (escape) {
        case 't':
            out.push_back('\t');
            break;
        case 'n':
            out.push_back('\n');
            break;
        case 'r':
            out.push_back('\r');
            break;
        case 'f':
            out.push_back('\f');
            break;
        case 'u':
            if (i + 4 < value.size()) {
                out += decodeUnicodeEscape(value.substr(i + 1, 4));
                i += 4;
            }
            break;
        default:
            out.push_back(escape);
            break;
        }
    }
    return out;
}

void TranslationStorage::loadProperties(const std::string& text)
{
    std::istringstream stream(text);
    std::string line;
    std::string pendingKey;
    std::string pendingValue;
    bool continuation = false;

    auto commitEntry = [&]() {
        if (pendingKey.empty()) {
            return;
        }
        translations_[pendingKey] = unescapePropertiesValue(pendingValue);
        pendingKey.clear();
        pendingValue.clear();
        continuation = false;
    };

    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (continuation) {
            pendingValue += line;
            if (!line.empty() && line.back() == '\\') {
                pendingValue.pop_back();
                continue;
            }
            commitEntry();
            continue;
        }

        if (line.empty() || line.front() == '#') {
            continue;
        }

        const std::size_t separator = line.find('=');
        if (separator == std::string::npos) {
            continue;
        }

        pendingKey = line.substr(0, separator);
        pendingValue = line.substr(separator + 1);
        if (!pendingValue.empty() && pendingValue.back() == '\\') {
            pendingValue.pop_back();
            continuation = true;
            continue;
        }
        commitEntry();
    }

    if (continuation && !pendingKey.empty()) {
        commitEntry();
    }
}

} // namespace net::minecraft::client::resource::language
