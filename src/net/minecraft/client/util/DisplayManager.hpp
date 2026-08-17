#pragma once
#include <string>
namespace net::minecraft::client {
class Minecraft;
}
namespace net::minecraft::client::util {
class DisplayManager {
 public:
 static void setupAndCreateDisplay(Minecraft& client);
 static void toggleFullscreen(Minecraft& client);
 static void resize(Minecraft& client, int width, int height);
 static void logGlError(const std::string& phase);
};
} // namespace net::minecraft::client::util
