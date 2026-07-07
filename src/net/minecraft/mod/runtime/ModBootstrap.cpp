#include "net/minecraft/mod/runtime/ModBootstrap.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/mod/runtime/ModHost.hpp"
namespace net::minecraft::mod::runtime {
void bootstrapClient() {
  net::minecraft::block::initializeBlocks();
}
void shutdownClient() {
  host().shutdown();
}
} // namespace net::minecraft::mod::runtime
