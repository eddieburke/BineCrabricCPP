#pragma once
namespace net::minecraft::client {
class Minecraft;
}
namespace net::minecraft::client::lifecycle {
/// Client teardown: stats save, world clear, GL/sound/input shutdown.
/// Anchor: Minecraft.cpp L382–424, L549–565.
class ClientShutdown {
public:
  static void stop(Minecraft& client);
  static void cleanHeap(Minecraft& client);
};
} // namespace net::minecraft::client::lifecycle
