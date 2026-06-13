#include "msauth/AccountStorage.hpp"
#include "msauth/SecretProtection.hpp"

#include <cctype>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace msauth {
namespace {

std::filesystem::path accountFilePath(const std::filesystem::path& runDirectory)
{
    return runDirectory / "msauth-account.json";
}

std::optional<std::string> readFile(const std::filesystem::path& path)
{
    std::ifstream input(path);
    if (!input.is_open()) {
        return std::nullopt;
    }
    return std::string((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
}

// --- Prism/PolyMC/MultiMC accounts.json import ---

bool skipJsonStringLiteral(const std::string& json, std::size_t& pos)
{
    if (pos >= json.size() || json[pos] != '"') {
        return false;
    }
    ++pos;
    while (pos < json.size()) {
        const char ch = json[pos++];
        if (ch == '"') {
            return true;
        }
        if (ch == '\\') {
            if (pos >= json.size()) {
                return false;
            }
            ++pos;
        }
    }
    return false;
}

bool skipJsonCompound(const std::string& json, std::size_t& pos, char open, char close)
{
    if (pos >= json.size() || json[pos] != open) {
        return false;
    }
    ++pos;
    while (pos < json.size()) {
        const char ch = json[pos];
        if (ch == '"') {
            if (!skipJsonStringLiteral(json, pos)) {
                return false;
            }
            continue;
        }
        if (ch == open) {
            if (!skipJsonCompound(json, pos, open, close)) {
                return false;
            }
            continue;
        }
        if (ch == '{') {
            if (!skipJsonCompound(json, pos, '{', '}')) {
                return false;
            }
            continue;
        }
        if (ch == '[') {
            if (!skipJsonCompound(json, pos, '[', ']')) {
                return false;
            }
            continue;
        }
        ++pos;
        if (ch == close) {
            return true;
        }
    }
    return false;
}

void collectJsonObjects(const std::string& json, std::vector<std::string>& out)
{
    for (std::size_t pos = 0; pos < json.size();) {
        const char ch = json[pos];
        if (ch == '"') {
            if (!skipJsonStringLiteral(json, pos)) {
                return;
            }
            continue;
        }
        if (ch == '{') {
            std::size_t end = pos;
            if (!skipJsonCompound(json, end, '{', '}')) {
                return;
            }
            out.push_back(json.substr(pos, end - pos));
        }
        ++pos;
    }
}

std::optional<MicrosoftAccount> parseAccountSection(const std::string& section)
{
    const std::optional<std::string> type = json::stringField(section, "type");
    if (!type.has_value() || *type != "MSA") {
        return std::nullopt;
    }

    std::optional<std::string> ygg = json::objectField(section, "ygg");
    std::optional<std::string> profile = json::objectField(section, "profile");
    if (!profile.has_value() && ygg.has_value()) {
        profile = json::objectField(*ygg, "profile");
    }
    if (!profile.has_value() || !ygg.has_value()) {
        return std::nullopt;
    }

    const std::optional<std::string> profileName = json::stringField(*profile, "name");
    const std::optional<std::string> profileId = json::stringField(*profile, "id");
    const std::optional<std::string> accessToken = json::stringField(*ygg, "token");
    std::optional<std::string> refreshToken = json::stringField(*ygg, "refresh_token");
    if (!refreshToken.has_value()) {
        refreshToken = json::stringField(section, "refresh_token");
    }
    if (!refreshToken.has_value()) {
        // Prism/PolyMC/MultiMC serialize the MSA token block (tokenToJSON) under "msa":
        //   "msa": { "token": "<msa access token>", "refresh_token": "<refresh>", ... }
        // "token" is the access-token STRING, and refresh_token is its SIBLING — not nested
        // beneath it. Read the sibling; fall back to the nested form only for safety.
        if (const std::optional<std::string> msa = json::objectField(section, "msa")) {
            refreshToken = json::stringField(*msa, "refresh_token");
            if (!refreshToken.has_value()) {
                if (const std::optional<std::string> token = json::objectField(*msa, "token")) {
                    refreshToken = json::stringField(*token, "refresh_token");
                }
            }
        }
    }
    const std::optional<std::string> clientId = json::stringField(section, "msa-client-id");

    if (!profileName.has_value() || !profileId.has_value() || !accessToken.has_value() || accessToken->empty()) {
        return std::nullopt;
    }

    MicrosoftAccount account;
    account.clientId = clientId.has_value() && !clientId->empty()
        ? *clientId
        : loadMicrosoftClientId(std::filesystem::path {});
    account.profileName = *profileName;
    account.profileId = *profileId;
    account.accessToken = *accessToken;
    if (refreshToken.has_value()) {
        account.refreshToken = *refreshToken;
    }
    if (!account.valid()) {
        return std::nullopt;
    }
    return account;
}

std::optional<MicrosoftAccount> parseFlatAccountJson(const std::string& json)
{
    MicrosoftAccount account;
    if (const std::optional<std::string> value = json::stringField(json, "clientId")) {
        account.clientId = *value;
    }
    if (const std::optional<std::string> value = json::stringField(json, "profileName")) {
        account.profileName = *value;
    }
    if (const std::optional<std::string> value = json::stringField(json, "profileId")) {
        account.profileId = *value;
    }
    if (const std::optional<std::string> value = json::stringField(json, "accessToken")) {
        account.accessToken = *value;
    }
    if (const std::optional<std::string> value = json::stringField(json, "refreshToken")) {
        account.refreshToken = *value;
    }
    if (const std::optional<std::string> value = json::stringField(json, "refreshTokenProtected")) {
        if (std::optional<std::string> decrypted = secret::unprotectForCurrentUser(*value)) {
            account.refreshToken = *decrypted;
            secret::wipeString(*decrypted);
        } else {
            return std::nullopt;
        }
    }
    if (account.clientId.empty()) {
        account.clientId = loadMicrosoftClientId(std::filesystem::path {});
    }
    if (!account.valid() && !account.restorable()) {
        return std::nullopt;
    }
    return account;
}

std::optional<MicrosoftAccount> parseSavedAccountJson(const std::string& json)
{
    std::optional<MicrosoftAccount> parsed = parseFlatAccountJson(json);
    if (!parsed.has_value()) {
        return std::nullopt;
    }
    if (!parsed->hasProfile()) {
        return std::nullopt;
    }
    if (parsed->refreshToken.empty() && parsed->accessToken.empty()) {
        return std::nullopt;
    }
    return parsed;
}

std::optional<MicrosoftAccount> importFromLauncherJsonText(const std::string& json)
{
    std::vector<std::string> objects;
    collectJsonObjects(json, objects);
    std::optional<MicrosoftAccount> fallback;
    for (const std::string& section : objects) {
        if (const std::optional<MicrosoftAccount> account = parseAccountSection(section)) {
            if (json::boolField(section, "active").value_or(false)) {
                return account;
            }
            if (!fallback.has_value()) {
                fallback = account;
            }
        }
    }
    return fallback;
}

} // namespace

std::optional<MicrosoftAccount> loadAccount(const std::filesystem::path& runDirectory)
{
    const std::optional<std::string> json = readFile(accountFilePath(runDirectory));
    if (!json.has_value()) {
        return std::nullopt;
    }
    return parseSavedAccountJson(*json);
}

bool saveAccount(const std::filesystem::path& runDirectory, const MicrosoftAccount& account)
{
    if (!account.hasProfile() || account.clientId.empty() || account.refreshToken.empty()) {
        return false;
    }
    std::optional<std::string> protectedRefreshToken = secret::protectForCurrentUser(account.refreshToken);
    if (!protectedRefreshToken.has_value()) {
        return false;
    }

    std::ostringstream json;
    json << "{\n";
    json << "  \"storageVersion\": 2,\n";
    json << "  \"clientId\": \"" << json::escape(account.clientId) << "\",\n";
    json << "  \"profileName\": \"" << json::escape(account.profileName) << "\",\n";
    json << "  \"profileId\": \"" << json::escape(account.profileId) << "\",\n";
    json << "  \"refreshTokenProtected\": \"" << json::escape(*protectedRefreshToken) << "\"\n";
    json << "}\n";
    secret::wipeString(*protectedRefreshToken);

    std::ofstream output(accountFilePath(runDirectory), std::ios::trunc);
    if (!output.is_open()) {
        return false;
    }
    output << json.str();
    return output.good();
}

bool clearAccount(const std::filesystem::path& runDirectory)
{
    std::error_code error;
    return std::filesystem::remove(accountFilePath(runDirectory), error);
}

std::optional<MicrosoftAccount> importAccountFromJsonText(const std::string& json)
{
    if (const std::optional<MicrosoftAccount> flat = parseFlatAccountJson(json)) {
        return flat;
    }
    return importFromLauncherJsonText(json);
}

std::optional<MicrosoftAccount> importAccountFromJsonFile(const std::filesystem::path& path)
{
    const std::optional<std::string> json = readFile(path);
    if (!json.has_value()) {
        return std::nullopt;
    }
    return importAccountFromJsonText(*json);
}

} // namespace msauth
