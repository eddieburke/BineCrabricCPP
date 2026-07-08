#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

#include "net/minecraft/server/ServerProperties.hpp"

namespace {
std::filesystem::path makeTempPropertiesPath(const std::string& name) {
    const std::filesystem::path directory = std::filesystem::temp_directory_path() / "minecraft_native_server_tests";
    std::filesystem::create_directories(directory);
    return directory / name;
}
}  // namespace

namespace net::minecraft::test {
TEST(ServerProperties, LoadsExistingProperties) {
    const std::filesystem::path path = makeTempPropertiesPath("existing.properties");
    {
        std::ofstream output(path);
        output << "server-port=25566\n";
        output << "max-players=8\n";
    }
    net::minecraft::server::ServerProperties properties(path);
    EXPECT_EQ(properties.getProperty("server-port", 25565), 25566);
    EXPECT_EQ(properties.getProperty("max-players", 20), 8);
}

TEST(ServerProperties, CreatesMissingProperties) {
    const std::filesystem::path path = makeTempPropertiesPath("missing.properties");
    std::filesystem::remove(path);
    net::minecraft::server::ServerProperties properties(path);
    EXPECT_TRUE(std::filesystem::exists(path));
    EXPECT_EQ(properties.getProperty("server-port", 25565), 25565);
}

TEST(ServerProperties, BooleanPropertyRoundTrip) {
    const std::filesystem::path path = makeTempPropertiesPath("boolean.properties");
    std::filesystem::remove(path);
    net::minecraft::server::ServerProperties properties(path);
    properties.setProperty("white-list", true);
    net::minecraft::server::ServerProperties reloaded(path);
    EXPECT_TRUE(reloaded.getProperty("white-list", false));
    properties.setProperty("white-list", false);
    net::minecraft::server::ServerProperties reloadedFalse(path);
    EXPECT_FALSE(reloadedFalse.getProperty("white-list", true));
}

TEST(ServerProperties, InvalidIntegerFallsBack) {
    const std::filesystem::path path = makeTempPropertiesPath("invalid_int.properties");
    {
        std::ofstream output(path);
        output << "view-distance=not-a-number\n";
    }
    net::minecraft::server::ServerProperties properties(path);
    EXPECT_EQ(properties.getProperty("view-distance", 10), 10);
}
}  // namespace net::minecraft::test
