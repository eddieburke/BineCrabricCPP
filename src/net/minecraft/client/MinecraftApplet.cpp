#include "net/minecraft/client/MinecraftApplet.hpp"
namespace net::minecraft {
std::string MinecraftApplet::getParameter(const std::string& name) const {
  if(name == "stand-alone") {
    return "true";
  }
  return {};
}
} // namespace net::minecraft
