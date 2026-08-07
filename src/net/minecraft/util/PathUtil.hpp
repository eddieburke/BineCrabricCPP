#pragma once
#include <string>
namespace net::minecraft::util {
// Strips leading '/' or '\' — the canonical resource-path form TextureManager's
// path maps and TextureRegistry's registry keys both key on. Lives in util (not
// client) because TextureRegistry is part of minecraft_core, which the dedicated
// server links without the client library.
inline std::string normalizePath(std::string path) {
 while(!path.empty() && (path.front() == '/' || path.front() == '\\')) {
  path.erase(path.begin());
 }
 return path;
}
} // namespace net::minecraft::util
