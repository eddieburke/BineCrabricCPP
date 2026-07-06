#include "net/minecraft/mod/HookBus.hpp"
#include "net/minecraft/mod/MinecraftApi.hpp"
namespace net::minecraft::mod {
MINECRAFT_API HookBus& hooks() {
  static HookBus bus;
  return bus;
}
} // namespace net::minecraft::mod
