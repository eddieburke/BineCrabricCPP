#include "net/minecraft/client/achievement/AchievementStatFormatter.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/option/GameOptions.hpp"
#include "net/minecraft/client/resource/language/I18n.hpp"
#include "net/minecraft/client/input/KeyCodes.hpp"
namespace net::minecraft::client::achievement {
AchievementStatFormatter::AchievementStatFormatter(Minecraft* client) : client_(client) {}
std::string AchievementStatFormatter::format(const std::string& text) const {
  if(client_ == nullptr) {
    return text;
  }
  const char* keyName = client::input::keyDisplayName(static_cast<int>(client_->options.inventoryKey.code));
  return resource::language::I18n::formatJava(text, {keyName != nullptr ? keyName : "?"});
}
} // namespace net::minecraft::client::achievement
