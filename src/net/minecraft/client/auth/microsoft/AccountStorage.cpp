#include "net/minecraft/client/auth/microsoft/AccountStorage.hpp"
#include <fstream>
#include <mutex>
#include <sstream>
#include "net/minecraft/client/auth/microsoft/SecretProtection.hpp"
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif
namespace msauth {
namespace {
struct AccountCache {
  bool valid = false;
  bool exists = false;
  std::filesystem::path path;
  std::filesystem::file_time_type modified{};
  std::optional<MicrosoftAccount> account;
};
std::mutex gAccountCacheMutex;
AccountCache gAccountCache;
void replaceAccountCache(AccountCache next) {
  std::lock_guard<std::mutex> lock(gAccountCacheMutex);
  if(gAccountCache.account.has_value()) {
    secret::wipeString(gAccountCache.account->accessToken);
    secret::wipeString(gAccountCache.account->refreshToken);
  }
  gAccountCache = std::move(next);
}
std::filesystem::path accountFilePath(const std::filesystem::path& runDirectory) {
  return runDirectory / "msauth-account.json";
}
std::optional<std::string> readFile(const std::filesystem::path& path) {
  std::ifstream input(path);
  if(!input.is_open()) {
    return std::nullopt;
  }
  return std::string((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
}
std::optional<MicrosoftAccount> parseFlatAccountJson(const std::string& json) {
  MicrosoftAccount account;
  if(const std::optional<std::string> value = json::stringField(json, "clientId")) {
    account.clientId = *value;
  }
  if(const std::optional<std::string> value = json::stringField(json, "profileName")) {
    account.profileName = *value;
  }
  if(const std::optional<std::string> value = json::stringField(json, "profileId")) {
    account.profileId = *value;
  }
  if(const std::optional<std::string> value = json::stringField(json, "accessToken")) {
    account.accessToken = *value;
  }
  if(const std::optional<std::string> value = json::stringField(json, "refreshToken")) {
    account.refreshToken = *value;
  }
  if(const std::optional<std::string> value = json::stringField(json, "refreshTokenProtected")) {
    if(std::optional<std::string> decrypted = secret::unprotectForCurrentUser(*value)) {
      account.refreshToken = *decrypted;
      secret::wipeString(*decrypted);
    } else {
      return std::nullopt;
    }
  }
  if(const std::optional<std::string> value = json::stringField(json, "skinUrl")) {
    account.skinUrl = *value;
  }
  if(const std::optional<std::string> value = json::stringField(json, "capeUrl")) {
    account.capeUrl = *value;
  }
  if(const std::optional<std::string> value = json::stringField(json, "xboxGamertag")) {
    account.xboxGamertag = *value;
  }
  account.ownsMinecraft = json::boolField(json, "ownsMinecraft").value_or(false);
  if(account.clientId.empty()) {
    account.clientId = loadMicrosoftClientId(std::filesystem::path{});
  }
  if(!account.valid() && !account.restorable()) {
    return std::nullopt;
  }
  return account;
}
std::optional<MicrosoftAccount> parseSavedAccountJson(const std::string& json) {
  std::optional<MicrosoftAccount> parsed = parseFlatAccountJson(json);
  if(!parsed.has_value()) {
    return std::nullopt;
  }
  if(!parsed->hasProfile()) {
    return std::nullopt;
  }
  if(parsed->refreshToken.empty() && parsed->accessToken.empty()) {
    return std::nullopt;
  }
  return parsed;
}
} // namespace
std::optional<MicrosoftAccount> loadAccount(const std::filesystem::path& runDirectory) {
  const std::filesystem::path path = accountFilePath(runDirectory);
  std::error_code error;
  const bool exists = std::filesystem::exists(path, error) && !error;
  const std::filesystem::file_time_type modified =
      exists ? std::filesystem::last_write_time(path, error) : std::filesystem::file_time_type{};
  {
    std::lock_guard<std::mutex> lock(gAccountCacheMutex);
    if(gAccountCache.valid && gAccountCache.path == path && gAccountCache.exists == exists &&
       (!exists || (!error && gAccountCache.modified == modified))) {
      return gAccountCache.account;
    }
  }
  const std::optional<std::string> json = exists ? readFile(path) : std::nullopt;
  const std::optional<MicrosoftAccount> parsed = json.has_value() ? parseSavedAccountJson(*json) : std::nullopt;
  replaceAccountCache(AccountCache{true, exists, path, modified, parsed});
  if(!json.has_value()) {
    return std::nullopt;
  }
  return parsed;
}
bool saveAccount(const std::filesystem::path& runDirectory, const MicrosoftAccount& account) {
  if(!account.hasProfile() || account.clientId.empty() || account.refreshToken.empty()) {
    return false;
  }
  std::optional<std::string> protectedRefreshToken = secret::protectForCurrentUser(account.refreshToken);
  if(!protectedRefreshToken.has_value()) {
    return false;
  }
  std::ostringstream json;
  json << "{\n";
  json << "  \"storageVersion\": 2,\n";
  json << "  \"clientId\": \"" << json::escape(account.clientId) << "\",\n";
  json << "  \"profileName\": \"" << json::escape(account.profileName) << "\",\n";
  json << "  \"profileId\": \"" << json::escape(account.profileId) << "\",\n";
  if(!account.skinUrl.empty()) {
    json << "  \"skinUrl\": \"" << json::escape(account.skinUrl) << "\",\n";
  }
  if(!account.capeUrl.empty()) {
    json << "  \"capeUrl\": \"" << json::escape(account.capeUrl) << "\",\n";
  }
  if(!account.xboxGamertag.empty()) {
    json << "  \"xboxGamertag\": \"" << json::escape(account.xboxGamertag) << "\",\n";
  }
  json << "  \"ownsMinecraft\": " << (account.ownsMinecraft ? "true" : "false") << ",\n";
  json << "  \"refreshTokenProtected\": \"" << json::escape(*protectedRefreshToken) << "\"\n";
  json << "}\n";
  secret::wipeString(*protectedRefreshToken);
  const std::filesystem::path finalPath = accountFilePath(runDirectory);
  std::filesystem::path temporaryPath = finalPath;
  temporaryPath += ".tmp";
  std::ofstream output(temporaryPath, std::ios::binary | std::ios::trunc);
  if(!output.is_open()) {
    return false;
  }
  output << json.str();
  output.flush();
  const bool writeOk = output.good();
  output.close();
  if(!writeOk) {
    std::error_code ignored;
    std::filesystem::remove(temporaryPath, ignored);
    return false;
  }
#ifdef _WIN32
  if(!MoveFileExW(temporaryPath.c_str(), finalPath.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
    std::error_code ignored;
    std::filesystem::remove(temporaryPath, ignored);
    return false;
  }
#else
  std::error_code error;
  std::filesystem::rename(temporaryPath, finalPath, error);
  if(error) {
    std::filesystem::remove(temporaryPath, error);
    return false;
  }
#endif
  std::error_code modifiedError;
  const std::filesystem::file_time_type modified = std::filesystem::last_write_time(finalPath, modifiedError);
  MicrosoftAccount cachedAccount = account;
  secret::wipeString(cachedAccount.accessToken);
  cachedAccount.accessTokenExpiresAt = 0;
  replaceAccountCache(AccountCache{true, true, finalPath, modified, std::move(cachedAccount)});
  return true;
}
bool clearAccount(const std::filesystem::path& runDirectory) {
  std::error_code error;
  const std::filesystem::path path = accountFilePath(runDirectory);
  const bool removed = std::filesystem::remove(path, error);
  const bool absent = removed || (!error && !std::filesystem::exists(path));
  if(absent) {
    replaceAccountCache(AccountCache{true, false, path, {}, std::nullopt});
  }
  return absent;
}
} // namespace msauth
