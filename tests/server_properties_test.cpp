#include "support/server_test_macros.hpp"
#include "net/minecraft/server/ServerProperties.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
namespace {
std::filesystem::path makeTempPropertiesPath(const std::string& name) {
  const std::filesystem::path directory = std::filesystem::temp_directory_path() / "minecraft_native_server_tests";
  std::filesystem::create_directories(directory);
  return directory / name;
}
void testLoadsExistingProperties() {
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
void testCreatesMissingProperties() {
  const std::filesystem::path path = makeTempPropertiesPath("missing.properties");
  std::filesystem::remove(path);
  net::minecraft::server::ServerProperties properties(path);
  EXPECT_TRUE(std::filesystem::exists(path));
  EXPECT_EQ(properties.getProperty("server-port", 25565), 25565);
}
void testBooleanPropertyRoundTrip() {
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
void testInvalidIntegerFallsBack() {
  const std::filesystem::path path = makeTempPropertiesPath("invalid_int.properties");
  {
    std::ofstream output(path);
    output << "view-distance=not-a-number\n";
  }
  net::minecraft::server::ServerProperties properties(path);
  EXPECT_EQ(properties.getProperty("view-distance", 10), 10);
}
} // namespace
int main() {
  RUN_SERVER_TEST(testLoadsExistingProperties);
  RUN_SERVER_TEST(testCreatesMissingProperties);
  RUN_SERVER_TEST(testBooleanPropertyRoundTrip);
  RUN_SERVER_TEST(testInvalidIntegerFallsBack);
  if(server_test::failureCount() != 0) {
    std::cout << server_test::failureCount() << " test(s) failed\n";
    return 1;
  }
  std::cout << "All server properties tests passed\n";
  return 0;
}
