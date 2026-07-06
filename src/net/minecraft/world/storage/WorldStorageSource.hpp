#pragma once
#include "net/minecraft/world/WorldProperties.hpp"
#include "net/minecraft/world/storage/WorldSaveInfo.hpp"
#include "net/minecraft/world/storage/WorldStorage.hpp"
#include <memory>
#include <string>
#include <vector>
namespace net::minecraft::client::gui::screen {
class LoadingDisplay;
}
namespace net::minecraft {
class WorldStorageSource {
public:
  virtual ~WorldStorageSource() = default;
  [[nodiscard]] virtual std::string getName() const = 0;
  [[nodiscard]] virtual std::unique_ptr<WorldStorage> getSaveLoader(const std::string& name,
                                                                    bool createPlayerDataDir) = 0;
  [[nodiscard]] virtual std::vector<WorldSaveInfo> getAll() = 0;
  virtual void flush() = 0;
  [[nodiscard]] virtual std::optional<WorldProperties> getWorldProperties(const std::string& name) = 0;
  virtual void deleteSave(const std::string& name) = 0;
  virtual void rename(const std::string& saveName, const std::string& newName) = 0;
  [[nodiscard]] virtual bool needsConversion(const std::string& name) const = 0;
  virtual bool convert(const std::string& name, client::gui::screen::LoadingDisplay* display) = 0;
};
} // namespace net::minecraft
