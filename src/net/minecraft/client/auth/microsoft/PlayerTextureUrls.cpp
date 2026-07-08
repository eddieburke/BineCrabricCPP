#include "net/minecraft/client/auth/microsoft/MicrosoftAuth.hpp"

namespace msauth {
std::string legacySkinUrl(const std::string& username) {
    if (username.empty()) {
        return {};
    }
    return "http://s3.amazonaws.com/MinecraftSkins/" + username + ".png";
}

std::string legacyCapeUrl(const std::string& username) {
    if (username.empty()) {
        return {};
    }
    return "http://s3.amazonaws.com/MinecraftCloaks/" + username + ".png";
}

std::string resolveSkinUrl(const MicrosoftAccount& account) {
    if (!account.skinUrl.empty()) {
        return account.skinUrl;
    }
    return legacySkinUrl(account.profileName);
}

std::string resolveCapeUrl(const MicrosoftAccount& account) {
    if (!account.capeUrl.empty()) {
        return account.capeUrl;
    }
    return legacyCapeUrl(account.profileName);
}

bool shouldUseBetacraftProxyForUrl(const std::string& url) {
    return url.find("s3.amazonaws.com") != std::string::npos || url.find("MinecraftResources") != std::string::npos ||
           url.find("MinecraftSkins") != std::string::npos || url.find("MinecraftCloaks") != std::string::npos;
}
}  // namespace msauth
