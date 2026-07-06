#pragma once
namespace net::minecraft::client {
class Minecraft;
}
namespace net::minecraft::client::render {
/// Mojang logo splash during client bootstrap.
/// Anchor: Minecraft.cpp L281–329.
class LoadingScreenRenderer {
public:
  static void renderLoadingScreen(Minecraft& client);
};
} // namespace net::minecraft::client::render
