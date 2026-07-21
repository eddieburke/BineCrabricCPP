#pragma once
// Microsoft/Xbox/Minecraft authentication owned by the native client.
#include <atomic>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include "net/minecraft/util/json/JsonFields.hpp"
namespace msauth {
struct MicrosoftAccount {
 std::string clientId;
 std::string profileName;
 std::string profileId;
 std::string accessToken;
 std::int64_t accessTokenExpiresAt = 0;
 std::string refreshToken;
 std::string skinUrl;
 std::string capeUrl;
 bool slimArms = false;
 std::string xboxGamertag;
 bool ownsMinecraft = false;
 [[nodiscard]] std::string sessionId() const;
 [[nodiscard]] bool hasProfile() const noexcept;
 [[nodiscard]] bool restorable() const noexcept;
 [[nodiscard]] bool valid() const noexcept;
};
enum class AuthStage {
 Idle,
 RequestingDeviceCode,
 WaitingForUser,
 RefreshingMicrosoftToken,
 AuthenticatingXboxUser,
 AuthorizingXboxServices,
 AuthorizingMinecraftServices,
 FetchingXboxProfile,
 LoggingIntoMinecraft,
 CheckingEntitlements,
 FetchingMinecraftProfile,
 CreatingMinecraftProfile,
 Complete,
};
using AuthProgressCallback = std::function<void(AuthStage)>;
[[nodiscard]] const char* authStageMessage(AuthStage stage) noexcept;
struct DeviceCodeChallenge {
 std::string deviceCode;
 std::string userCode;
 std::string verificationUri;
 std::string verificationUriComplete;
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
 bool canceled = false;
 bool needsProfileCreation = false;
 std::string error;
 MicrosoftAccount account;
};
[[nodiscard]] std::string loadMicrosoftClientId(const std::filesystem::path& runDirectory);
[[nodiscard]] DeviceCodeRequestResult requestDeviceCode(const std::string& clientId,
                                                        const std::atomic_bool* canceled = nullptr,
                                                        const AuthProgressCallback& progress = {});
[[nodiscard]] AuthResult loginWithDeviceCode(const std::string& clientId,
                                             const DeviceCodeChallenge& challenge,
                                             const std::atomic_bool* canceled = nullptr,
                                             const AuthProgressCallback& progress = {});
[[nodiscard]] AuthResult restoreFromRefreshToken(const MicrosoftAccount& savedAccount,
                                                 const std::atomic_bool* canceled = nullptr,
                                                 const AuthProgressCallback& progress = {});
[[nodiscard]] AuthResult createMinecraftProfile(const MicrosoftAccount& account,
                                                const std::string& profileName,
                                                const std::atomic_bool* canceled = nullptr,
                                                const AuthProgressCallback& progress = {});
namespace json = net::minecraft::util::json;
[[nodiscard]] std::string legacySkinUrl(const std::string& username);
[[nodiscard]] std::string legacyCapeUrl(const std::string& username);
[[nodiscard]] std::string resolveSkinUrl(const MicrosoftAccount& account);
[[nodiscard]] std::string resolveCapeUrl(const MicrosoftAccount& account);
[[nodiscard]] bool shouldUseBetacraftProxyForUrl(const std::string& url);
} // namespace msauth
