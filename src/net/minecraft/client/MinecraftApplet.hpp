#pragma once
// Faithful port of net.minecraft.client.MinecraftApplet (beta 1.7.3 MCP).
#include <string>
namespace net::minecraft {
class MinecraftApplet {
public:
  MinecraftApplet() = default;
  virtual ~MinecraftApplet() = default;
  [[nodiscard]] virtual bool isActive() const {
    return active_;
  }
  [[nodiscard]] virtual std::string getParameter(const std::string& name) const {
    if(name == "stand-alone") {
      return "true";
    }
    return {};
  }
  virtual void clearMemory() {}
  void setActive(bool active) {
    active_ = active;
  }

protected:
  bool active_ = true;
};
} // namespace net::minecraft
