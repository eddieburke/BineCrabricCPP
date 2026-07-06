#include "net/minecraft/mod/runtime/ModBootstrap.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/util/MinecraftDirectories.hpp"
#include "net/minecraft/mod/runtime/ModHost.hpp"
namespace net::minecraft::mod::runtime {
void bootstrapClient() {
  host().initialize(net::minecraft::client::util::MinecraftDirectories::getRunDirectory());
  host().loadEnabledPackageMods();
  net::minecraft::block::initializeBlocks();
}
void shutdownClient() {
  host().shutdown();
}
} // namespace net::minecraft::mod::runtime
