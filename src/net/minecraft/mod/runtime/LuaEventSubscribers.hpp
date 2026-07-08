#pragma once
#include <memory>
#include <string_view>

#include "net/minecraft/mod/runtime/ModHost.hpp"

namespace net::minecraft::mod::runtime {
[[nodiscard]] bool isSupportedLuaEvent(std::string_view event);
void subscribeLuaCallback(const std::shared_ptr<ModHost::LoadedLuaMod>& mod,
                          const ModHost::LoadedLuaMod::Callback& callback);
}  // namespace net::minecraft::mod::runtime
