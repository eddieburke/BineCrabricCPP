#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace msauth {

struct MicrosoftAccount {
    std::string clientId;
    std::string profileName;
    std::string profileId;
    std::string accessToken;
    std::string refreshToken;

    [[nodiscard]] std::string sessionId() const;
    [[nodiscard]] bool hasProfile() const noexcept;
    [[nodiscard]] bool restorable() const noexcept;
    [[nodiscard]] bool valid() const noexcept;
};

struct DeviceCodeChallenge {
    std::string deviceCode;
    std::string userCode;
    std::string verificationUri;
    int expiresIn = 0;
    int interval = 5;
};

struct DeviceCodeRequestResult {
    bool ok = false;
    std::string error;
    DeviceCodeChallenge challenge;
};

struct AuthResult {
    bool ok = false;
    std::string error;
    MicrosoftAccount account;
};

[[nodiscard]] std::string loadMicrosoftClientId(const std::filesystem::path& runDirectory);

[[nodiscard]] DeviceCodeRequestResult requestDeviceCode(const std::string& clientId);
[[nodiscard]] AuthResult loginWithDeviceCode(const std::string& clientId, const DeviceCodeChallenge& challenge);
[[nodiscard]] AuthResult restoreFromRefreshToken(const MicrosoftAccount& savedAccount);

// Minimal flat-JSON field helpers shared with AccountStorage.
namespace json {
[[nodiscard]] std::optional<std::string> stringField(const std::string& json, const std::string& key);
[[nodiscard]] std::optional<int> intField(const std::string& json, const std::string& key);
[[nodiscard]] std::optional<bool> boolField(const std::string& json, const std::string& key);
[[nodiscard]] std::optional<std::string> objectField(const std::string& json, const std::string& key);
[[nodiscard]] std::string escape(const std::string& text);
} // namespace json

} // namespace msauth
