#include "msauth/MicrosoftAuth.hpp"

#include "net/minecraft/client/resource/ResourceDownloadThread.hpp"

#include <cctype>
#include <chrono>
#include <fstream>
#include <thread>
#include <vector>

namespace msauth {

// --- JSON field helpers ---

namespace json {
namespace {

std::size_t skipWs(const std::string& json, std::size_t pos)
{
    while (pos < json.size() && std::isspace(static_cast<unsigned char>(json[pos]))) {
        ++pos;
    }
    return pos;
}

bool parseJsonString(const std::string& json, std::size_t& pos, std::string* out)
{
    pos = skipWs(json, pos);
    if (pos >= json.size() || json[pos] != '"') {
        return false;
    }
    ++pos;
    if (out != nullptr) {
        out->clear();
    }
    while (pos < json.size()) {
        const char ch = json[pos++];
        if (ch == '"') {
            return true;
        }
        if (ch == '\\') {
            if (pos >= json.size()) {
                return false;
            }
            const char esc = json[pos++];
            if (out == nullptr) {
                continue;
            }
            switch (esc) {
            case '"':
            case '\\':
            case '/':
                out->push_back(esc);
                break;
            case 'b':
                out->push_back('\b');
                break;
            case 'f':
                out->push_back('\f');
                break;
            case 'n':
                out->push_back('\n');
                break;
            case 'r':
                out->push_back('\r');
                break;
            case 't':
                out->push_back('\t');
                break;
            default:
                out->push_back(esc);
                break;
            }
            continue;
        }
        if (out != nullptr) {
            out->push_back(ch);
        }
    }
    return false;
}

bool skipJsonValue(const std::string& json, std::size_t& pos);

bool skipJsonObject(const std::string& json, std::size_t& pos)
{
    pos = skipWs(json, pos);
    if (pos >= json.size() || json[pos] != '{') {
        return false;
    }
    ++pos;
    pos = skipWs(json, pos);
    if (pos < json.size() && json[pos] == '}') {
        ++pos;
        return true;
    }
    while (pos < json.size()) {
        if (!parseJsonString(json, pos, nullptr)) {
            return false;
        }
        pos = skipWs(json, pos);
        if (pos >= json.size() || json[pos] != ':') {
            return false;
        }
        ++pos;
        if (!skipJsonValue(json, pos)) {
            return false;
        }
        pos = skipWs(json, pos);
        if (pos >= json.size()) {
            return false;
        }
        if (json[pos] == '}') {
            ++pos;
            return true;
        }
        if (json[pos] != ',') {
            return false;
        }
        ++pos;
    }
    return false;
}

bool skipJsonArray(const std::string& json, std::size_t& pos)
{
    pos = skipWs(json, pos);
    if (pos >= json.size() || json[pos] != '[') {
        return false;
    }
    ++pos;
    pos = skipWs(json, pos);
    if (pos < json.size() && json[pos] == ']') {
        ++pos;
        return true;
    }
    while (pos < json.size()) {
        if (!skipJsonValue(json, pos)) {
            return false;
        }
        pos = skipWs(json, pos);
        if (pos >= json.size()) {
            return false;
        }
        if (json[pos] == ']') {
            ++pos;
            return true;
        }
        if (json[pos] != ',') {
            return false;
        }
        ++pos;
    }
    return false;
}

bool skipJsonPrimitive(const std::string& json, std::size_t& pos)
{
    pos = skipWs(json, pos);
    const std::size_t start = pos;
    while (pos < json.size()) {
        const char ch = json[pos];
        if (ch == ',' || ch == '}' || ch == ']' || std::isspace(static_cast<unsigned char>(ch))) {
            break;
        }
        ++pos;
    }
    return pos > start;
}

bool skipJsonValue(const std::string& json, std::size_t& pos)
{
    pos = skipWs(json, pos);
    if (pos >= json.size()) {
        return false;
    }
    switch (json[pos]) {
    case '"':
        return parseJsonString(json, pos, nullptr);
    case '{':
        return skipJsonObject(json, pos);
    case '[':
        return skipJsonArray(json, pos);
    default:
        return skipJsonPrimitive(json, pos);
    }
}

bool findFieldValueRange(
    const std::string& json, const std::string& key, std::size_t& valueStart, std::size_t& valueEnd)
{
    std::size_t pos = skipWs(json, 0);
    if (pos >= json.size() || json[pos] != '{') {
        return false;
    }
    ++pos;
    pos = skipWs(json, pos);
    if (pos < json.size() && json[pos] == '}') {
        return false;
    }
    while (pos < json.size()) {
        std::string currentKey;
        if (!parseJsonString(json, pos, &currentKey)) {
            return false;
        }
        pos = skipWs(json, pos);
        if (pos >= json.size() || json[pos] != ':') {
            return false;
        }
        ++pos;
        const std::size_t candidateStart = skipWs(json, pos);
        std::size_t candidateEnd = candidateStart;
        if (!skipJsonValue(json, candidateEnd)) {
            return false;
        }
        if (currentKey == key) {
            valueStart = candidateStart;
            valueEnd = candidateEnd;
            return true;
        }
        pos = skipWs(json, candidateEnd);
        if (pos >= json.size()) {
            return false;
        }
        if (json[pos] == '}') {
            return false;
        }
        if (json[pos] != ',') {
            return false;
        }
        ++pos;
    }
    return false;
}

} // namespace

std::optional<std::string> stringField(const std::string& json, const std::string& key)
{
    std::size_t valueStart = 0;
    std::size_t valueEnd = 0;
    if (!findFieldValueRange(json, key, valueStart, valueEnd)) {
        return std::nullopt;
    }
    std::string value;
    std::size_t pos = valueStart;
    if (!parseJsonString(json, pos, &value)) {
        return std::nullopt;
    }
    return value;
}

std::optional<int> intField(const std::string& json, const std::string& key)
{
    std::size_t valueStart = 0;
    std::size_t valueEnd = 0;
    if (!findFieldValueRange(json, key, valueStart, valueEnd)) {
        return std::nullopt;
    }
    std::size_t end = valueStart;
    while (end < valueEnd && (std::isdigit(static_cast<unsigned char>(json[end])) || json[end] == '-')) {
        ++end;
    }
    if (end == valueStart) {
        return std::nullopt;
    }
    try {
        return std::stoi(json.substr(valueStart, end - valueStart));
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<bool> boolField(const std::string& json, const std::string& key)
{
    std::size_t valueStart = 0;
    std::size_t valueEnd = 0;
    if (!findFieldValueRange(json, key, valueStart, valueEnd)) {
        return std::nullopt;
    }
    const std::string value = json.substr(valueStart, valueEnd - valueStart);
    if (value == "true") {
        return true;
    }
    if (value == "false") {
        return false;
    }
    return std::nullopt;
}

std::optional<std::string> objectField(const std::string& json, const std::string& key)
{
    std::size_t valueStart = 0;
    std::size_t valueEnd = 0;
    if (!findFieldValueRange(json, key, valueStart, valueEnd)) {
        return std::nullopt;
    }
    if (valueStart >= json.size() || json[valueStart] != '{') {
        return std::nullopt;
    }
    return json.substr(valueStart, valueEnd - valueStart);
}

std::string escape(const std::string& text)
{
    std::string out;
    out.reserve(text.size());
    for (const char ch : text) {
        switch (ch) {
        case '"':
            out += "\\\"";
            break;
        case '\\':
            out += "\\\\";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            out += ch;
            break;
        }
    }
    return out;
}

} // namespace json

// --- account ---

std::string MicrosoftAccount::sessionId() const
{
    if (profileId.empty()) {
        return {};
    }
    return "msa:" + profileId;
}

bool MicrosoftAccount::hasProfile() const noexcept
{
    return !profileName.empty() && !profileId.empty();
}

bool MicrosoftAccount::restorable() const noexcept
{
    return hasProfile() && !clientId.empty() && !refreshToken.empty();
}

bool MicrosoftAccount::valid() const noexcept
{
    return hasProfile() && !accessToken.empty();
}

// --- client id ---

namespace {

// Prism Launcher MSA app (supports device-code flow on consumers tenant).
constexpr const char* kDefaultClientId = "c36a9fb6-4f2a-41ff-90bd-ae7cc92031eb";
constexpr const char* kMsaScope = "XboxLive.SignIn XboxLive.offline_access";

std::string readTrimmedFile(const std::filesystem::path& path)
{
    std::ifstream input(path);
    if (!input.is_open()) {
        return {};
    }
    std::string line;
    std::getline(input, line);
    while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' ')) {
        line.pop_back();
    }
    std::size_t start = 0;
    while (start < line.size() && line[start] == ' ') {
        ++start;
    }
    return line.substr(start);
}

} // namespace

std::string loadMicrosoftClientId(const std::filesystem::path& runDirectory)
{
    const std::string fromRunDir = readTrimmedFile(runDirectory / "microsoft-client-id.txt");
    if (!fromRunDir.empty()) {
        return fromRunDir;
    }
    return kDefaultClientId;
}

// --- HTTP helpers ---

namespace {

namespace resource = net::minecraft::client::resource;

using HttpResponse = resource::HttpResponse;
using HttpHeader = resource::HttpHeader;

std::string urlEncodeForm(const std::string& value)
{
    static const char hex[] = "0123456789ABCDEF";
    std::string encoded;
    encoded.reserve(value.size());
    for (const unsigned char ch : value) {
        if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '-' || ch == '_'
            || ch == '.' || ch == '~') {
            encoded += static_cast<char>(ch);
        } else {
            encoded += '%';
            encoded += hex[ch >> 4];
            encoded += hex[ch & 0x0F];
        }
    }
    return encoded;
}

HttpResponse httpGet(const std::string& url, const std::vector<HttpHeader>& headers = {})
{
    resource::HttpRequest request;
    request.method = "GET";
    request.url = url;
    request.headers = headers;
    request.useBetacraftProxy = false;
    return resource::httpRequest(request);
}

HttpResponse httpPostForm(const std::string& url, const std::string& formBody)
{
    resource::HttpRequest request;
    request.method = "POST";
    request.url = url;
    request.headers = {
        {"Content-Type", "application/x-www-form-urlencoded"},
        {"Accept", "application/json"},
    };
    request.body = formBody;
    request.useBetacraftProxy = false;
    return resource::httpRequest(request);
}

HttpResponse httpPostJson(const std::string& url, const std::string& jsonBody,
    const std::vector<HttpHeader>& extraHeaders = {})
{
    resource::HttpRequest request;
    request.method = "POST";
    request.url = url;
    request.headers = {
        {"Content-Type", "application/json"},
        {"Accept", "application/json"},
    };
    request.headers.insert(request.headers.end(), extraHeaders.begin(), extraHeaders.end());
    request.body = jsonBody;
    request.useBetacraftProxy = false;
    return resource::httpRequest(request);
}

// --- MSA -> Xbox -> Minecraft auth chain ---

struct MsaTokenPair {
    std::string accessToken;
    std::string refreshToken;
};

AuthResult fail(std::string message)
{
    AuthResult result;
    result.error = std::move(message);
    return result;
}

std::string shortenMicrosoftError(std::string text)
{
    const std::size_t tracePos = text.find(" Trace ID:");
    if (tracePos != std::string::npos) {
        text.resize(tracePos);
    }
    while (!text.empty() && text.back() == ' ') {
        text.pop_back();
    }
    return text;
}

std::string formatHttpError(int statusCode, const std::string& json)
{
    if (const std::optional<std::string> description = json::stringField(json, "error_description")) {
        return "HTTP " + std::to_string(statusCode) + ": " + shortenMicrosoftError(*description);
    }
    if (const std::optional<std::string> error = json::stringField(json, "error")) {
        return "HTTP " + std::to_string(statusCode) + ": " + *error;
    }
    if (statusCode > 0) {
        return "HTTP " + std::to_string(statusCode);
    }
    return "Request failed";
}

std::optional<std::string> xboxUserHash(const std::string& json)
{
    const std::string marker = "\"xui\"";
    const std::size_t xuiPos = json.find(marker);
    if (xuiPos == std::string::npos) {
        return std::nullopt;
    }
    return json::stringField(json.substr(xuiPos), "uhs");
}

AuthResult completeFromMsa(const std::string& clientId, const std::string& msaAccessToken)
{
    const std::string xboxAuthBody = std::string(R"({"Properties":{"AuthMethod":"RPS","SiteName":"user.auth.xboxlive.com","RpsTicket":"d=)")
        + json::escape(msaAccessToken)
        + R"("},"RelyingParty":"http://auth.xboxlive.com","TokenType":"JWT"})";

    const HttpResponse xboxUserResponse = httpPostJson(
        "https://user.auth.xboxlive.com/user/authenticate",
        xboxAuthBody,
        {{"x-xbl-contract-version", "1"}});
    if (!xboxUserResponse.ok()) {
        return fail("Xbox user authentication failed (HTTP " + std::to_string(xboxUserResponse.statusCode) + ")");
    }

    const std::string xboxUserJson = xboxUserResponse.bodyAsString();
    const std::optional<std::string> xblToken = json::stringField(xboxUserJson, "Token");
    const std::optional<std::string> userHash = xboxUserHash(xboxUserJson);
    if (!xblToken.has_value() || !userHash.has_value()) {
        return fail("Could not parse Xbox user token");
    }

    const std::string xstsBody = std::string(R"({"Properties":{"SandboxId":"RETAIL","UserTokens":[")")
        + json::escape(*xblToken)
        + R"("]},"RelyingParty":"rp://api.minecraftservices.com/","TokenType":"JWT"})";

    const HttpResponse xstsResponse = httpPostJson(
        "https://xsts.auth.xboxlive.com/xsts/authorize",
        xstsBody,
        {{"x-xbl-contract-version", "1"}});
    if (!xstsResponse.ok()) {
        const std::optional<int> xerr = json::intField(xstsResponse.bodyAsString(), "XErr");
        if (xerr.has_value()) {
            return fail("Xbox authorization failed (XErr " + std::to_string(*xerr) + ")");
        }
        return fail("Xbox authorization failed (HTTP " + std::to_string(xstsResponse.statusCode) + ")");
    }

    const std::string xstsJson = xstsResponse.bodyAsString();
    const std::optional<std::string> xstsToken = json::stringField(xstsJson, "Token");
    const std::optional<std::string> xstsHash = xboxUserHash(xstsJson);
    if (!xstsToken.has_value() || !xstsHash.has_value() || *xstsHash != *userHash) {
        return fail("Could not parse Xbox services token");
    }

    const std::string identityToken = "XBL3.0 x=" + *userHash + ";" + *xstsToken;
    const std::string loginBody = std::string(R"({"identityToken":")") + json::escape(identityToken) + "\"}";
    const HttpResponse loginResponse = httpPostJson(
        "https://api.minecraftservices.com/authentication/login_with_xbox",
        loginBody);
    if (!loginResponse.ok()) {
        return fail("Minecraft login failed (HTTP " + std::to_string(loginResponse.statusCode) + ")");
    }

    const std::string loginJson = loginResponse.bodyAsString();
    const std::optional<std::string> accessToken = json::stringField(loginJson, "access_token");
    if (!accessToken.has_value() || accessToken->empty()) {
        return fail("Minecraft login response missing access token");
    }

    const HttpResponse profileResponse = httpGet(
        "https://api.minecraftservices.com/minecraft/profile",
        {{"Authorization", "Bearer " + *accessToken}});
    if (!profileResponse.ok()) {
        return fail("Could not load Minecraft profile (HTTP " + std::to_string(profileResponse.statusCode) + ")");
    }

    const std::string profileJson = profileResponse.bodyAsString();
    const std::optional<std::string> profileId = json::stringField(profileJson, "id");
    const std::optional<std::string> profileName = json::stringField(profileJson, "name");
    if (!profileId.has_value() || !profileName.has_value()) {
        return fail("Minecraft profile not found for this account");
    }

    AuthResult result;
    result.ok = true;
    result.account.clientId = clientId;
    result.account.accessToken = *accessToken;
    result.account.profileId = *profileId;
    result.account.profileName = *profileName;
    return result;
}

// nullopt = still pending; empty accessToken = hard failure/denied.
std::optional<MsaTokenPair> pollDeviceCodeToken(const std::string& clientId, const std::string& deviceCode)
{
    const std::string body = "grant_type=" + urlEncodeForm("urn:ietf:params:oauth:grant-type:device_code")
        + "&client_id=" + urlEncodeForm(clientId)
        + "&device_code=" + urlEncodeForm(deviceCode);
    const HttpResponse response = httpPostForm(
        "https://login.microsoftonline.com/consumers/oauth2/v2.0/token",
        body);
    const std::string json = response.bodyAsString();
    if (response.ok()) {
        MsaTokenPair tokens;
        if (const std::optional<std::string> accessToken = json::stringField(json, "access_token")) {
            tokens.accessToken = *accessToken;
        }
        if (const std::optional<std::string> refreshToken = json::stringField(json, "refresh_token")) {
            tokens.refreshToken = *refreshToken;
        }
        if (tokens.accessToken.empty()) {
            return std::nullopt;
        }
        return tokens;
    }

    const std::optional<std::string> error = json::stringField(json, "error");
    if (error.has_value() && (*error == "authorization_pending" || *error == "slow_down")) {
        return std::nullopt;
    }

    return MsaTokenPair {};
}

std::optional<MsaTokenPair> refreshMsaAccessToken(const std::string& clientId, const std::string& refreshToken)
{
    const std::string body = "grant_type=" + urlEncodeForm("refresh_token")
        + "&client_id=" + urlEncodeForm(clientId)
        + "&refresh_token=" + urlEncodeForm(refreshToken)
        + "&scope=" + urlEncodeForm(kMsaScope);
    const HttpResponse response = httpPostForm(
        "https://login.microsoftonline.com/consumers/oauth2/v2.0/token",
        body);
    if (!response.ok()) {
        return std::nullopt;
    }
    const std::string json = response.bodyAsString();
    MsaTokenPair tokens;
    if (const std::optional<std::string> accessToken = json::stringField(json, "access_token")) {
        tokens.accessToken = *accessToken;
    }
    if (const std::optional<std::string> newRefreshToken = json::stringField(json, "refresh_token")) {
        tokens.refreshToken = *newRefreshToken;
    } else {
        tokens.refreshToken = refreshToken;
    }
    if (tokens.accessToken.empty()) {
        return std::nullopt;
    }
    return tokens;
}

} // namespace

DeviceCodeRequestResult requestDeviceCode(const std::string& clientId)
{
    DeviceCodeRequestResult result;
    const std::string body = "client_id=" + urlEncodeForm(clientId)
        + "&scope=" + urlEncodeForm(kMsaScope);
    const HttpResponse response = httpPostForm(
        "https://login.microsoftonline.com/consumers/oauth2/v2.0/devicecode",
        body);
    const std::string json = response.bodyAsString();
    if (!response.ok()) {
        result.error = formatHttpError(response.statusCode, json);
        if (response.statusCode == 0) {
            result.error = "Could not reach login.microsoftonline.com";
        }
        return result;
    }

    DeviceCodeChallenge challenge;
    if (const std::optional<std::string> deviceCode = json::stringField(json, "device_code")) {
        challenge.deviceCode = *deviceCode;
    }
    if (const std::optional<std::string> userCode = json::stringField(json, "user_code")) {
        challenge.userCode = *userCode;
    }
    if (const std::optional<std::string> uri = json::stringField(json, "verification_uri")) {
        challenge.verificationUri = *uri;
    }
    if (const std::optional<int> expiresIn = json::intField(json, "expires_in")) {
        challenge.expiresIn = *expiresIn;
    }
    if (const std::optional<int> interval = json::intField(json, "interval")) {
        challenge.interval = *interval > 0 ? *interval : 5;
    }
    if (challenge.deviceCode.empty() || challenge.userCode.empty() || challenge.verificationUri.empty()) {
        result.error = formatHttpError(response.statusCode, json);
        if (result.error == "Request failed") {
            result.error = "Device code response missing required fields";
        }
        return result;
    }

    result.ok = true;
    result.challenge = std::move(challenge);
    return result;
}

AuthResult loginWithDeviceCode(const std::string& clientId, const DeviceCodeChallenge& challenge)
{
    const int pollSeconds = challenge.interval > 0 ? challenge.interval : 5;
    const int maxAttempts = challenge.expiresIn > 0 ? (challenge.expiresIn / pollSeconds) + 2 : 180;
    for (int attempt = 0; attempt < maxAttempts; ++attempt) {
        std::this_thread::sleep_for(std::chrono::seconds(pollSeconds));
        const std::optional<MsaTokenPair> msaTokens = pollDeviceCodeToken(clientId, challenge.deviceCode);
        if (!msaTokens.has_value()) {
            continue;
        }
        if (msaTokens->accessToken.empty()) {
            return fail("Microsoft sign-in failed or was denied");
        }
        AuthResult result = completeFromMsa(clientId, msaTokens->accessToken);
        if (result.ok) {
            result.account.refreshToken = msaTokens->refreshToken;
        }
        return result;
    }
    return fail("Microsoft sign-in timed out");
}

AuthResult restoreFromRefreshToken(const MicrosoftAccount& savedAccount)
{
    if (savedAccount.clientId.empty() || savedAccount.refreshToken.empty()) {
        return fail("Saved account is missing refresh credentials");
    }
    const std::optional<MsaTokenPair> msaTokens = refreshMsaAccessToken(savedAccount.clientId, savedAccount.refreshToken);
    if (!msaTokens.has_value() || msaTokens->accessToken.empty()) {
        return fail("Could not refresh Microsoft session; sign in again");
    }
    AuthResult result = completeFromMsa(savedAccount.clientId, msaTokens->accessToken);
    if (result.ok) {
        result.account.refreshToken = msaTokens->refreshToken.empty() ? savedAccount.refreshToken : msaTokens->refreshToken;
    }
    return result;
}

} // namespace msauth
