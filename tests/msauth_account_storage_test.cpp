#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "msauth/AccountStorage.hpp"
#include "net/minecraft/client/resource/ResourceDownloadThread.hpp"

#include <filesystem>
#include <fstream>
#include <iterator>

namespace net::minecraft::client::resource {

HttpResponse httpRequest(const HttpRequest& request)
{
    (void)request;
    return {};
}

} // namespace net::minecraft::client::resource

TEST_CASE("flat msauth account json imports")
{
    const auto imported = msauth::importAccountFromJsonText(R"({
        "clientId": "client-123",
        "profileName": "Steve",
        "profileId": "profile-123",
        "accessToken": "access-123",
        "refreshToken": "refresh-123"
    })");

    REQUIRE(imported.has_value());
    CHECK(imported->clientId == "client-123");
    CHECK(imported->profileName == "Steve");
    CHECK(imported->profileId == "profile-123");
    CHECK(imported->accessToken == "access-123");
    CHECK(imported->refreshToken == "refresh-123");
}

TEST_CASE("launcher accounts json picks the active msa account")
{
    const auto imported = msauth::importAccountFromJsonText(R"({
        "meta": "brace } and quote \"inside\" should stay inside the string",
        "accounts": [
            {
                "active": false,
                "type": "MSA",
                "profile": {
                    "name": "Old Steve",
                    "id": "old-profile"
                },
                "ygg": {
                    "token": "old-access",
                    "refresh_token": "old-refresh"
                },
                "msa-client-id": "old-client"
            },
            {
                "active": true,
                "type": "MSA",
                "ygg": {
                    "token": "fresh-access",
                    "refresh_token": "fresh-refresh",
                    "profile": {
                        "name": "Alex",
                        "id": "fresh-profile"
                    }
                },
                "msa-client-id": "fresh-client"
            }
        ]
    })");

    REQUIRE(imported.has_value());
    CHECK(imported->clientId == "fresh-client");
    CHECK(imported->profileName == "Alex");
    CHECK(imported->profileId == "fresh-profile");
    CHECK(imported->accessToken == "fresh-access");
    CHECK(imported->refreshToken == "fresh-refresh");
}

TEST_CASE("saved msauth account protects the refresh token and omits the access token")
{
    const std::filesystem::path runDirectory = std::filesystem::temp_directory_path() / "msauth-account-storage-test";
    std::error_code error;
    std::filesystem::remove_all(runDirectory, error);
    REQUIRE(std::filesystem::create_directories(runDirectory));

    msauth::MicrosoftAccount account;
    account.clientId = "client-123";
    account.profileName = "Steve";
    account.profileId = "profile-123";
    account.accessToken = "access-123";
    account.refreshToken = "refresh-123";

    REQUIRE(msauth::saveAccount(runDirectory, account));

    std::ifstream input(runDirectory / "msauth-account.json");
    REQUIRE(input.is_open());
    const std::string savedJson((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    CHECK(savedJson.find("access-123") == std::string::npos);
    CHECK(savedJson.find("refresh-123") == std::string::npos);
    CHECK(savedJson.find("refreshTokenProtected") != std::string::npos);

    const auto loaded = msauth::loadAccount(runDirectory);
    REQUIRE(loaded.has_value());
    CHECK(loaded->clientId == "client-123");
    CHECK(loaded->profileName == "Steve");
    CHECK(loaded->profileId == "profile-123");
    CHECK(loaded->accessToken.empty());
    CHECK(loaded->refreshToken == "refresh-123");

    std::filesystem::remove_all(runDirectory, error);
}
