#include "net/minecraft/client/auth/microsoft/MicrosoftAuth.hpp"
#include "net/minecraft/client/auth/microsoft/SecretProtection.hpp"
#include "net/minecraft/client/resource/ResourceDownloadThread.hpp"
#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <fstream>
#include <random>
#include <thread>
#include <vector>
namespace msauth {
const char* authStageMessage(AuthStage stage) noexcept {
  switch(stage) {
  case AuthStage::RequestingDeviceCode:
    return "Requesting Microsoft sign-in code...";
  case AuthStage::WaitingForUser:
    return "Waiting for Microsoft sign-in...";
  case AuthStage::RefreshingMicrosoftToken:
    return "Refreshing Microsoft account...";
  case AuthStage::AuthenticatingXboxUser:
    return "Signing in to Xbox Live...";
  case AuthStage::AuthorizingXboxServices:
    return "Authorizing Xbox services...";
  case AuthStage::AuthorizingMinecraftServices:
    return "Authorizing Minecraft services...";
  case AuthStage::FetchingXboxProfile:
    return "Fetching Xbox profile...";
  case AuthStage::LoggingIntoMinecraft:
    return "Signing in to Minecraft services...";
  case AuthStage::CheckingEntitlements:
    return "Checking Minecraft ownership...";
  case AuthStage::FetchingMinecraftProfile:
    return "Fetching Minecraft profile...";
  case AuthStage::CreatingMinecraftProfile:
    return "Creating Minecraft profile...";
  case AuthStage::Complete:
    return "Microsoft account ready";
  case AuthStage::Idle:
  default:
    return "";
  }
}
// --- account ---
std::string MicrosoftAccount::sessionId() const {
  if(profileId.empty()) {
    return {};
  }
  return "msa:" + profileId;
}
bool MicrosoftAccount::hasProfile() const noexcept {
  return !profileName.empty() && !profileId.empty();
}
bool MicrosoftAccount::restorable() const noexcept {
  return hasProfile() && !clientId.empty() && !refreshToken.empty();
}
bool MicrosoftAccount::valid() const noexcept {
  return hasProfile() && !accessToken.empty();
}
// --- client id ---
namespace {
// MultiMC MSA app (device-code flow on consumers tenant).
constexpr const char* kDefaultClientId = "499546d9-bbfe-4b9b-a086-eb3d75afb78f";
constexpr const char* kMsaScope = "XboxLive.signin offline_access";
std::string readTrimmedFile(const std::filesystem::path& path) {
  std::ifstream input(path);
  if(!input.is_open()) {
    return {};
  }
  std::string line;
  std::getline(input, line);
  while(!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' ')) {
    line.pop_back();
  }
  std::size_t start = 0;
  while(start < line.size() && line[start] == ' ') {
    ++start;
  }
  return line.substr(start);
}
} // namespace
std::string loadMicrosoftClientId(const std::filesystem::path& runDirectory) {
  const std::string fromRunDir = readTrimmedFile(runDirectory / "microsoft-client-id.txt");
  if(!fromRunDir.empty()) {
    return fromRunDir;
  }
  return kDefaultClientId;
}
// --- HTTP helpers ---
namespace {
namespace resource = net::minecraft::client::resource;
using HttpResponse = resource::HttpResponse;
using HttpHeader = resource::HttpHeader;
std::string urlEncodeForm(const std::string& value) {
  static const char hex[] = "0123456789ABCDEF";
  std::string encoded;
  encoded.reserve(value.size());
  for(const unsigned char ch : value) {
    if((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '-' ||
       ch == '_' || ch == '.' || ch == '~') {
      encoded += static_cast<char>(ch);
    } else {
      encoded += '%';
      encoded += hex[ch >> 4];
      encoded += hex[ch & 0x0F];
    }
  }
  return encoded;
}
[[nodiscard]] bool isCanceled(const std::atomic_bool* canceled) {
  return canceled != nullptr && canceled->load(std::memory_order_acquire);
}
HttpResponse httpGet(const std::string& url, const std::vector<HttpHeader>& headers = {},
                     const std::atomic_bool* canceled = nullptr) {
  resource::HttpRequest request;
  request.method = "GET";
  request.url = url;
  request.headers = headers;
  request.useBetacraftProxy = false;
  request.maxResponseBytes = 2U * 1024U * 1024U;
  request.cancelled = canceled;
  return resource::httpRequest(request);
}
HttpResponse httpPostForm(const std::string& url, const std::string& formBody,
                          const std::atomic_bool* canceled = nullptr) {
  resource::HttpRequest request;
  request.method = "POST";
  request.url = url;
  request.headers = {
      {"Content-Type", "application/x-www-form-urlencoded"},
      {"Accept", "application/json"},
  };
  request.body = formBody;
  request.useBetacraftProxy = false;
  request.maxResponseBytes = 2U * 1024U * 1024U;
  request.cancelled = canceled;
  return resource::httpRequest(request);
}
HttpResponse httpPostJson(const std::string& url, const std::string& jsonBody,
                          const std::vector<HttpHeader>& extraHeaders = {},
                          const std::atomic_bool* canceled = nullptr) {
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
  request.maxResponseBytes = 2U * 1024U * 1024U;
  request.cancelled = canceled;
  return resource::httpRequest(request);
}
// --- MSA -> Xbox -> Minecraft auth chain ---
struct MsaTokenPair {
  std::string accessToken;
  std::string refreshToken;
};
AuthResult fail(std::string message) {
  AuthResult result;
  result.error = std::move(message);
  return result;
}
AuthResult canceledResult() {
  AuthResult result;
  result.canceled = true;
  result.error = "Microsoft sign-in canceled";
  return result;
}
std::string formatXstsError(std::int64_t xerr) {
  switch(xerr) {
  case 2148916227:
    return "Your Xbox Live account has been banned by Microsoft for violating the Xbox Community Standards. "
           "This may happen if your account was shared or resold.";
  case 2148916229:
    return "This Microsoft account is linked to a family and your parent or guardian has not given you permission "
           "to play online.";
  case 2148916233:
    return "This Microsoft account does not have an Xbox Live profile. Buy Minecraft on minecraft.net first.";
  case 2148916234:
    return "This account has not accepted the Xbox Terms of Service. Please log in online and accept them.";
  case 2148916235:
    return "Xbox Live is not available in your country.";
  case 2148916236:
    return "This Microsoft account requires proof of age to play. Please sign in at login.live.com to provide proof "
           "of age.";
  case 2148916237:
    return "This Microsoft account has reached its playtime limit and has been blocked from logging in.";
  case 2148916238:
    return "This Microsoft account is underaged and is not linked to a family. See help.minecraft.net to set up your "
           "account.";
  default:
    return "XSTS authentication failed (XErr " + std::to_string(xerr) + ")";
  }
}
std::string formatMojangApiError(int statusCode, const std::string& json) {
  if(const std::optional<std::string> errorMessage = json::stringField(json, "errorMessage")) {
    return *errorMessage;
  }
  if(const std::optional<std::string> error = json::stringField(json, "error")) {
    return *error;
  }
  if(statusCode > 0) {
    return "HTTP " + std::to_string(statusCode);
  }
  return "Request failed";
}
std::optional<std::string> mojangDetailsStatus(const std::string& json) {
  if(const std::optional<std::string> detailsJson = json::objectField(json, "details")) {
    return json::stringField(*detailsJson, "status");
  }
  return std::nullopt;
}
std::optional<std::string> activeTextureUrl(const std::string& profileJson, const std::string& field) {
  for(const std::string& texture : json::objectArrayField(profileJson, field)) {
    if(json::stringField(texture, "state") == "ACTIVE") {
      return json::stringField(texture, "url");
    }
  }
  return std::nullopt;
}
void applyActiveSkin(const std::string& profileJson, MicrosoftAccount& account) {
  for(const std::string& skin : json::objectArrayField(profileJson, "skins")) {
    if(json::stringField(skin, "state") != "ACTIVE") {
      continue;
    }
    if(const std::optional<std::string> url = json::stringField(skin, "url")) {
      account.skinUrl = *url;
    }
    if(const std::optional<std::string> variant = json::stringField(skin, "variant")) {
      account.slimArms = *variant == "SLIM";
    }
    return;
  }
}
AuthResult authResultFromProfileJson(const std::string& clientId, const std::string& accessToken,
                                     const std::string& profileJson) {
  const std::optional<std::string> profileId = json::stringField(profileJson, "id");
  const std::optional<std::string> profileName = json::stringField(profileJson, "name");
  if(!profileId.has_value() || !profileName.has_value()) {
    return fail("Minecraft profile response could not be parsed");
  }
  AuthResult result;
  result.ok = true;
  result.account.clientId = clientId;
  result.account.accessToken = accessToken;
  result.account.profileId = *profileId;
  result.account.profileName = *profileName;
  applyActiveSkin(profileJson, result.account);
  if(const std::optional<std::string> capeUrl = activeTextureUrl(profileJson, "capes")) {
    result.account.capeUrl = *capeUrl;
  }
  return result;
}
AuthResult fetchMinecraftProfile(const std::string& clientId, const std::string& accessToken,
                                 const std::atomic_bool* canceled = nullptr) {
  if(isCanceled(canceled)) {
    return canceledResult();
  }
  const HttpResponse profileResponse =
      httpGet("https://api.minecraftservices.com/minecraft/profile", {{"Authorization", "Bearer " + accessToken}},
              canceled);
  if(isCanceled(canceled)) {
    return canceledResult();
  }
  if(profileResponse.statusCode == 404) {
    AuthResult result;
    result.needsProfileCreation = true;
    result.account.clientId = clientId;
    result.account.accessToken = accessToken;
    return result;
  }
  if(!profileResponse.ok()) {
    return fail("Could not load Minecraft profile: " +
                formatMojangApiError(profileResponse.statusCode, profileResponse.bodyAsString()));
  }
  return authResultFromProfileJson(clientId, accessToken, profileResponse.bodyAsString());
}
std::string shortenMicrosoftError(std::string text) {
  const std::size_t tracePos = text.find(" Trace ID:");
  if(tracePos != std::string::npos) {
    text.resize(tracePos);
  }
  while(!text.empty() && text.back() == ' ') {
    text.pop_back();
  }
  return text;
}
std::string formatHttpError(int statusCode, const std::string& json) {
  if(const std::optional<std::string> description = json::stringField(json, "error_description")) {
    if(description->find("different client id") != std::string::npos) {
      return "Saved Microsoft sign-in belongs to a different launcher client. Sign in again.";
    }
    return "HTTP " + std::to_string(statusCode) + ": " + shortenMicrosoftError(*description);
  }
  if(const std::optional<std::string> error = json::stringField(json, "error")) {
    return "HTTP " + std::to_string(statusCode) + ": " + *error;
  }
  if(statusCode > 0) {
    return "HTTP " + std::to_string(statusCode);
  }
  return "Request failed";
}
std::optional<std::string> xboxUserHash(const std::string& json) {
  const std::optional<std::string> displayClaims = json::objectField(json, "DisplayClaims");
  if(!displayClaims.has_value()) {
    return std::nullopt;
  }
  for(const std::string& claim : json::objectArrayField(*displayClaims, "xui")) {
    if(const std::optional<std::string> userHash = json::stringField(claim, "uhs")) {
      return userHash;
    }
  }
  return std::nullopt;
}
void reportProgress(const AuthProgressCallback& progress, AuthStage stage) {
  if(progress) {
    progress(stage);
  }
}
struct XboxToken {
  std::string token;
  std::string userHash;
};
std::optional<XboxToken> authorizeXboxToken(const std::string& userToken, const std::string& expectedUserHash,
                                            const std::string& relyingParty, const std::atomic_bool* canceled,
                                            std::string& error) {
  const std::string body = std::string(R"({"Properties":{"SandboxId":"RETAIL","UserTokens":[")") +
                           json::escape(userToken) + R"("]},"RelyingParty":")" + json::escape(relyingParty) +
                           R"(","TokenType":"JWT"})";
  const HttpResponse response =
      httpPostJson("https://xsts.auth.xboxlive.com/xsts/authorize", body, {{"x-xbl-contract-version", "1"}},
                   canceled);
  if(isCanceled(canceled)) {
    return std::nullopt;
  }
  if(!response.ok()) {
    if(const std::optional<std::int64_t> xerr = json::int64Field(response.bodyAsString(), "XErr")) {
      error = formatXstsError(*xerr);
    } else {
      error = "Xbox authorization failed (HTTP " + std::to_string(response.statusCode) + ")";
    }
    return std::nullopt;
  }
  const std::string responseJson = response.bodyAsString();
  XboxToken token;
  token.token = json::stringField(responseJson, "Token").value_or("");
  token.userHash = xboxUserHash(responseJson).value_or("");
  if(token.token.empty() || token.userHash.empty() || token.userHash != expectedUserHash) {
    error = "Could not parse Xbox authorization token";
    return std::nullopt;
  }
  return token;
}
std::optional<std::string> xboxGamertag(const std::string& profileJson) {
  for(const std::string& profileUser : json::objectArrayField(profileJson, "profileUsers")) {
    for(const std::string& setting : json::objectArrayField(profileUser, "settings")) {
      if(json::stringField(setting, "id") == "Gamertag") {
        return json::stringField(setting, "value");
      }
    }
  }
  return std::nullopt;
}
bool hasMinecraftEntitlement(const std::string& entitlementsJson) {
  return entitlementsJson.find("\"game_minecraft\"") != std::string::npos ||
         entitlementsJson.find("\"product_minecraft\"") != std::string::npos;
}
std::string entitlementRequestId() {
  std::array<unsigned char, 16> bytes{};
  std::random_device random;
  for(unsigned char& byte : bytes) {
    byte = static_cast<unsigned char>(random());
  }
  bytes[6] = static_cast<unsigned char>((bytes[6] & 0x0F) | 0x40);
  bytes[8] = static_cast<unsigned char>((bytes[8] & 0x3F) | 0x80);
  static constexpr char hex[] = "0123456789abcdef";
  std::string id;
  id.reserve(36);
  for(std::size_t i = 0; i < bytes.size(); ++i) {
    if(i == 4 || i == 6 || i == 8 || i == 10) {
      id.push_back('-');
    }
    id.push_back(hex[bytes[i] >> 4]);
    id.push_back(hex[bytes[i] & 0x0F]);
  }
  return id;
}
void inheritCredentials(AuthResult& result, const MicrosoftAccount& source) {
  if(!result.ok) {
    return;
  }
  result.account.refreshToken = source.refreshToken;
  result.account.xboxGamertag = source.xboxGamertag;
  result.account.ownsMinecraft = source.ownsMinecraft;
  result.account.accessTokenExpiresAt = source.accessTokenExpiresAt;
}
AuthResult completeFromMsa(const std::string& clientId, const std::string& msaAccessToken,
                           const std::atomic_bool* canceled, const AuthProgressCallback& progress) {
  if(isCanceled(canceled)) {
    return canceledResult();
  }
  reportProgress(progress, AuthStage::AuthenticatingXboxUser);
  const std::string xboxAuthBody =
      std::string(R"({"Properties":{"AuthMethod":"RPS","SiteName":"user.auth.xboxlive.com","RpsTicket":"d=)") +
      json::escape(msaAccessToken) + R"("},"RelyingParty":"http://auth.xboxlive.com","TokenType":"JWT"})";
  const HttpResponse xboxUserResponse = httpPostJson("https://user.auth.xboxlive.com/user/authenticate", xboxAuthBody,
                                                     {{"x-xbl-contract-version", "1"}}, canceled);
  if(isCanceled(canceled)) {
    return canceledResult();
  }
  if(!xboxUserResponse.ok()) {
    return fail("Xbox user authentication failed: " +
                formatHttpError(xboxUserResponse.statusCode, xboxUserResponse.bodyAsString()));
  }
  const std::string xboxUserJson = xboxUserResponse.bodyAsString();
  const std::optional<std::string> xblToken = json::stringField(xboxUserJson, "Token");
  const std::optional<std::string> userHash = xboxUserHash(xboxUserJson);
  if(!xblToken.has_value() || xblToken->empty() || !userHash.has_value() || userHash->empty()) {
    return fail("Could not parse Xbox user token");
  }
  std::string authorizationError;
  reportProgress(progress, AuthStage::AuthorizingXboxServices);
  const std::optional<XboxToken> xboxApi =
      authorizeXboxToken(*xblToken, *userHash, "http://xboxlive.com", canceled, authorizationError);
  if(isCanceled(canceled)) {
    return canceledResult();
  }
  if(!xboxApi.has_value()) {
    return fail(std::move(authorizationError));
  }
  reportProgress(progress, AuthStage::AuthorizingMinecraftServices);
  const std::optional<XboxToken> minecraftApi = authorizeXboxToken(
      *xblToken, *userHash, "rp://api.minecraftservices.com/", canceled, authorizationError);
  if(isCanceled(canceled)) {
    return canceledResult();
  }
  if(!minecraftApi.has_value()) {
    return fail(std::move(authorizationError));
  }
  reportProgress(progress, AuthStage::LoggingIntoMinecraft);
  const std::string identityToken = "XBL3.0 x=" + *userHash + ";" + minecraftApi->token;
  const std::string loginBody = std::string(R"({"xtoken":")") + json::escape(identityToken) +
                                R"(","platform":"PC_LAUNCHER"})";
  const HttpResponse loginResponse =
      httpPostJson("https://api.minecraftservices.com/launcher/login", loginBody, {}, canceled);
  if(isCanceled(canceled)) {
    return canceledResult();
  }
  if(!loginResponse.ok()) {
    return fail("Minecraft login failed: " +
                formatMojangApiError(loginResponse.statusCode, loginResponse.bodyAsString()));
  }
  const std::string loginJson = loginResponse.bodyAsString();
  const std::optional<std::string> accessToken = json::stringField(loginJson, "access_token");
  if(!accessToken.has_value() || accessToken->empty()) {
    return fail("Minecraft login response missing access token");
  }
  const int expiresIn = json::intField(loginJson, "expires_in").value_or(86'400);
  const std::int64_t accessTokenExpiresAt =
      std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count() +
      std::max(expiresIn, 60);
  reportProgress(progress, AuthStage::FetchingXboxProfile);
  const std::string xboxProfileUrl =
      "https://profile.xboxlive.com/users/me/profile/settings?settings=Gamertag,ModernGamertag,"
      "ModernGamertagSuffix,UniqueModernGamertag,GameDisplayName";
  const std::string xboxAuthorization = "XBL3.0 x=" + *userHash + ";" + xboxApi->token;
  const HttpResponse xboxProfileResponse =
      httpGet(xboxProfileUrl,
              {{"Accept", "application/json"},
               {"x-xbl-contract-version", "3"},
               {"Authorization", xboxAuthorization}},
              canceled);
  if(isCanceled(canceled)) {
    return canceledResult();
  }
  if(!xboxProfileResponse.ok()) {
    return fail("Could not load Xbox profile: " +
                formatMojangApiError(xboxProfileResponse.statusCode, xboxProfileResponse.bodyAsString()));
  }
  const std::string gamertag = xboxGamertag(xboxProfileResponse.bodyAsString()).value_or("");
  reportProgress(progress, AuthStage::CheckingEntitlements);
  const std::string requestId = entitlementRequestId();
  const HttpResponse entitlementResponse =
      httpGet("https://api.minecraftservices.com/entitlements/license?requestId=" + requestId,
              {{"Accept", "application/json"}, {"Authorization", "Bearer " + *accessToken}}, canceled);
  if(isCanceled(canceled)) {
    return canceledResult();
  }
  if(!entitlementResponse.ok()) {
    return fail("Could not check Minecraft ownership: " +
                formatMojangApiError(entitlementResponse.statusCode, entitlementResponse.bodyAsString()));
  }
  if(!hasMinecraftEntitlement(entitlementResponse.bodyAsString())) {
    return fail("This Microsoft account does not own Minecraft: Java Edition.");
  }
  reportProgress(progress, AuthStage::FetchingMinecraftProfile);
  AuthResult result = fetchMinecraftProfile(clientId, *accessToken, canceled);
  if(result.ok || result.needsProfileCreation) {
    result.account.ownsMinecraft = true;
    result.account.xboxGamertag = gamertag;
    result.account.accessTokenExpiresAt = accessTokenExpiresAt;
  }
  if(result.ok) {
    reportProgress(progress, AuthStage::Complete);
  }
  return result;
}
enum class DevicePollState {
  Pending,
  SlowDown,
  Succeeded,
  Failed,
  Canceled,
};
struct DevicePollResult {
  DevicePollState state = DevicePollState::Pending;
  MsaTokenPair tokens;
  std::string error;
};
DevicePollResult pollState(DevicePollState state) {
  return DevicePollResult{state, {}, {}};
}
DevicePollResult pollDeviceCodeToken(const std::string& clientId, const std::string& deviceCode,
                                     const std::atomic_bool* canceled) {
  if(isCanceled(canceled)) {
    return pollState(DevicePollState::Canceled);
  }
  const std::string body = "grant_type=" + urlEncodeForm("urn:ietf:params:oauth:grant-type:device_code") +
                           "&client_id=" + urlEncodeForm(clientId) + "&device_code=" + urlEncodeForm(deviceCode);
  const HttpResponse response =
      httpPostForm("https://login.microsoftonline.com/consumers/oauth2/v2.0/token", body, canceled);
  if(isCanceled(canceled)) {
    return pollState(DevicePollState::Canceled);
  }
  const std::string json = response.bodyAsString();
  if(response.ok()) {
    DevicePollResult result;
    result.state = DevicePollState::Succeeded;
    if(const std::optional<std::string> accessToken = json::stringField(json, "access_token")) {
      result.tokens.accessToken = *accessToken;
    }
    if(const std::optional<std::string> refreshToken = json::stringField(json, "refresh_token")) {
      result.tokens.refreshToken = *refreshToken;
    }
    if(result.tokens.accessToken.empty()) {
      result.state = DevicePollState::Failed;
      result.error = "Microsoft token response missing access_token";
    }
    return result;
  }
  const std::optional<std::string> error = json::stringField(json, "error");
  if(error == "authorization_pending") {
    return pollState(DevicePollState::Pending);
  }
  if(error == "slow_down") {
    return pollState(DevicePollState::SlowDown);
  }
  DevicePollResult result;
  result.state = DevicePollState::Failed;
  result.error = formatHttpError(response.statusCode, json);
  return result;
}
std::optional<MsaTokenPair> refreshMsaAccessToken(const std::string& clientId, const std::string& refreshToken,
                                                  std::string* errorOut = nullptr,
                                                  const std::atomic_bool* canceled = nullptr) {
  if(isCanceled(canceled)) {
    return std::nullopt;
  }
  const std::string body = "grant_type=" + urlEncodeForm("refresh_token") + "&client_id=" + urlEncodeForm(clientId) +
                           "&refresh_token=" + urlEncodeForm(refreshToken) + "&scope=" + urlEncodeForm(kMsaScope);
  const HttpResponse response =
      httpPostForm("https://login.microsoftonline.com/consumers/oauth2/v2.0/token", body, canceled);
  if(isCanceled(canceled)) {
    return std::nullopt;
  }
  if(!response.ok()) {
    if(errorOut != nullptr) {
      *errorOut = response.statusCode == 0 ? "Could not reach login.microsoftonline.com"
                                           : formatHttpError(response.statusCode, response.bodyAsString());
    }
    return std::nullopt;
  }
  const std::string json = response.bodyAsString();
  MsaTokenPair tokens;
  if(const std::optional<std::string> accessToken = json::stringField(json, "access_token")) {
    tokens.accessToken = *accessToken;
  }
  if(const std::optional<std::string> newRefreshToken = json::stringField(json, "refresh_token")) {
    tokens.refreshToken = *newRefreshToken;
  } else {
    tokens.refreshToken = refreshToken;
  }
  if(tokens.accessToken.empty()) {
    if(errorOut != nullptr) {
      *errorOut = "Token refresh response missing access_token";
    }
    return std::nullopt;
  }
  return tokens;
}
bool interruptibleWait(std::chrono::seconds duration, const std::atomic_bool* canceled) {
  const auto deadline = std::chrono::steady_clock::now() + duration;
  while(std::chrono::steady_clock::now() < deadline) {
    if(isCanceled(canceled)) {
      return false;
    }
    const auto remaining = deadline - std::chrono::steady_clock::now();
    const auto remainingMs = std::chrono::duration_cast<std::chrono::milliseconds>(remaining);
    std::this_thread::sleep_for(remainingMs < std::chrono::milliseconds(100) ? remainingMs
                                                                             : std::chrono::milliseconds(100));
  }
  return !isCanceled(canceled);
}
} // namespace
DeviceCodeRequestResult requestDeviceCode(const std::string& clientId, const std::atomic_bool* canceled,
                                          const AuthProgressCallback& progress) {
  DeviceCodeRequestResult result;
  reportProgress(progress, AuthStage::RequestingDeviceCode);
  if(isCanceled(canceled)) {
    result.error = "Microsoft sign-in canceled";
    return result;
  }
  const std::string body = "client_id=" + urlEncodeForm(clientId) + "&scope=" + urlEncodeForm(kMsaScope);
  const HttpResponse response =
      httpPostForm("https://login.microsoftonline.com/consumers/oauth2/v2.0/devicecode", body, canceled);
  if(isCanceled(canceled)) {
    result.error = "Microsoft sign-in canceled";
    return result;
  }
  const std::string json = response.bodyAsString();
  if(!response.ok()) {
    result.error = formatHttpError(response.statusCode, json);
    if(response.statusCode == 0) {
      result.error = "Could not reach login.microsoftonline.com";
    }
    return result;
  }
  DeviceCodeChallenge challenge;
  if(const std::optional<std::string> deviceCode = json::stringField(json, "device_code")) {
    challenge.deviceCode = *deviceCode;
  }
  if(const std::optional<std::string> userCode = json::stringField(json, "user_code")) {
    challenge.userCode = *userCode;
  }
  if(const std::optional<std::string> uri = json::stringField(json, "verification_uri")) {
    challenge.verificationUri = *uri;
  }
  if(const std::optional<std::string> uri = json::stringField(json, "verification_uri_complete")) {
    challenge.verificationUriComplete = *uri;
  }
  if(const std::optional<int> expiresIn = json::intField(json, "expires_in")) {
    challenge.expiresIn = *expiresIn;
  }
  if(const std::optional<int> interval = json::intField(json, "interval")) {
    challenge.interval = *interval > 0 ? *interval : 5;
  }
  if(challenge.deviceCode.empty() || challenge.userCode.empty() || challenge.verificationUri.empty()) {
    result.error = formatHttpError(response.statusCode, json);
    if(result.error == "Request failed") {
      result.error = "Device code response missing required fields";
    }
    return result;
  }
  result.ok = true;
  result.challenge = std::move(challenge);
  return result;
}
AuthResult loginWithDeviceCode(const std::string& clientId, const DeviceCodeChallenge& challenge,
                               const std::atomic_bool* canceled, const AuthProgressCallback& progress) {
  int pollSeconds = challenge.interval > 0 ? challenge.interval : 5;
  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::seconds(challenge.expiresIn > 0 ? challenge.expiresIn : 900);
  reportProgress(progress, AuthStage::WaitingForUser);
  while(std::chrono::steady_clock::now() < deadline) {
    if(!interruptibleWait(std::chrono::seconds(pollSeconds), canceled)) {
      return canceledResult();
    }
    DevicePollResult poll = pollDeviceCodeToken(clientId, challenge.deviceCode, canceled);
    if(poll.state == DevicePollState::Pending) {
      continue;
    }
    if(poll.state == DevicePollState::SlowDown) {
      pollSeconds += 5;
      continue;
    }
    if(poll.state == DevicePollState::Canceled) {
      return canceledResult();
    }
    if(poll.state == DevicePollState::Failed) {
      return fail(poll.error.empty() ? "Microsoft sign-in failed or was denied" : std::move(poll.error));
    }
    AuthResult result = completeFromMsa(clientId, poll.tokens.accessToken, canceled, progress);
    if(result.ok || result.needsProfileCreation) {
      result.account.refreshToken = poll.tokens.refreshToken;
    }
    secret::wipeString(poll.tokens.accessToken);
    secret::wipeString(poll.tokens.refreshToken);
    return result;
  }
  return fail("Microsoft sign-in timed out");
}
AuthResult restoreFromRefreshToken(const MicrosoftAccount& savedAccount, const std::atomic_bool* canceled,
                                   const AuthProgressCallback& progress) {
  if(isCanceled(canceled)) {
    return canceledResult();
  }
  reportProgress(progress, AuthStage::RefreshingMicrosoftToken);
  if(savedAccount.clientId.empty() || savedAccount.refreshToken.empty()) {
    return fail("Saved account is missing refresh credentials");
  }
  std::string refreshError;
  std::optional<MsaTokenPair> msaTokens =
      refreshMsaAccessToken(savedAccount.clientId, savedAccount.refreshToken, &refreshError, canceled);
  if(isCanceled(canceled)) {
    return canceledResult();
  }
  if(!msaTokens.has_value() || msaTokens->accessToken.empty()) {
    return fail(refreshError.empty() ? "Could not refresh Microsoft session; sign in again" : refreshError);
  }
  AuthResult result = completeFromMsa(savedAccount.clientId, msaTokens->accessToken, canceled, progress);
  if(result.ok || result.needsProfileCreation) {
    result.account.refreshToken =
        msaTokens->refreshToken.empty() ? savedAccount.refreshToken : msaTokens->refreshToken;
  }
  secret::wipeString(msaTokens->accessToken);
  secret::wipeString(msaTokens->refreshToken);
  return result;
}
AuthResult createMinecraftProfile(const MicrosoftAccount& account, const std::string& profileName,
                                  const std::atomic_bool* canceled, const AuthProgressCallback& progress) {
  if(isCanceled(canceled)) {
    return canceledResult();
  }
  if(account.accessToken.empty()) {
    return fail("Missing access token");
  }
  const std::string trimmedName = [&profileName]() {
    const std::size_t start = profileName.find_first_not_of(" \t\r\n");
    if(start == std::string::npos) {
      return std::string{};
    }
    const std::size_t end = profileName.find_last_not_of(" \t\r\n");
    return profileName.substr(start, end - start + 1);
  }();
  if(trimmedName.empty()) {
    return fail("Profile name is required");
  }
  if(trimmedName.size() < 3 || trimmedName.size() > 16 ||
     !std::all_of(trimmedName.begin(), trimmedName.end(), [](unsigned char ch) {
       return std::isalnum(ch) != 0 || ch == '_';
     })) {
    return fail("Profile names must be 3-16 letters, numbers, or underscores.");
  }
  reportProgress(progress, AuthStage::CreatingMinecraftProfile);
  const std::string createBody = std::string(R"({"profileName":")") + json::escape(trimmedName) + "\"}";
  const HttpResponse createResponse = httpPostJson("https://api.minecraftservices.com/minecraft/profile", createBody,
                                                   {{"Authorization", "Bearer " + account.accessToken}}, canceled);
  if(isCanceled(canceled)) {
    return canceledResult();
  }
  if(!createResponse.ok()) {
    const std::string createJson = createResponse.bodyAsString();
    if(const std::optional<std::string> detailsStatus = mojangDetailsStatus(createJson)) {
      if(*detailsStatus == "ALREADY_REGISTERED") {
        AuthResult result = fetchMinecraftProfile(account.clientId, account.accessToken, canceled);
        if(result.ok) {
          inheritCredentials(result, account);
          reportProgress(progress, AuthStage::Complete);
        }
        return result;
      }
    }
    return fail(formatMojangApiError(createResponse.statusCode, createJson));
  }
  reportProgress(progress, AuthStage::FetchingMinecraftProfile);
  AuthResult result = fetchMinecraftProfile(account.clientId, account.accessToken, canceled);
  if(result.ok) {
    inheritCredentials(result, account);
    reportProgress(progress, AuthStage::Complete);
  }
  return result;
}
} // namespace msauth
