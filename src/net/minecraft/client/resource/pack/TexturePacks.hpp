#pragma once
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <memory>
#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>
#include "net/minecraft/client/option/GameOptions.hpp"
#include "net/minecraft/client/resource/ResourcePack.hpp"
#include "net/minecraft/client/resource/pack/BuiltInTexturePack.hpp"
#include "net/minecraft/client/resource/pack/TexturePack.hpp"
#include "net/minecraft/client/resource/pack/ZippedTexturePack.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
namespace net::minecraft::client::resource::pack {
class TexturePacks {
public:
  TexturePacks(std::filesystem::path resourceRoot,
               std::filesystem::path gameDir,
               option::GameOptions* options = nullptr,
               texture::TextureManager* textureManager = nullptr)
      : resources_(std::move(resourceRoot)),
        defaultPack_(std::make_unique<BuiltInTexturePack>(resources_)),
        dir_(std::move(gameDir) / "texturepacks"),
        options_(options),
        textureManager_(textureManager) {
    if(options_ != nullptr) {
      selectedName_ = options_->skin;
    }
    std::filesystem::create_directories(dir_);
    reload();
    if(selected == nullptr) {
      selected = defaultPack_.get();
    }
    selected->open();
  }
  bool select(TexturePack* pack) {
    if(pack == nullptr || pack == selected) {
      return false;
    }
    selected->close();
    selectedName_ = pack->name;
    selected = pack;
    if(options_ != nullptr) {
      options_->skin = selectedName_;
      options_->save();
    }
    selected->open();
    return true;
  }
  void reload() {
    selected = nullptr;
    std::vector<TexturePack*> nextPacks;
    nextPacks.push_back(defaultPack_.get());
    if(std::filesystem::exists(dir_) && std::filesystem::is_directory(dir_)) {
      for(const auto& entry : std::filesystem::directory_iterator(dir_)) {
        if(!entry.is_regular_file()) {
          continue;
        }
        const std::string filename = entry.path().filename().string();
        std::string lower = filename;
        for(char& ch : lower) {
          ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        }
        if(!lower.ends_with(".zip")) {
          continue;
        }
        std::error_code ec;
        const auto fileSize = entry.file_size(ec);
        const auto modified = entry.last_write_time(ec);
        const std::string key = filename + ":" + std::to_string(fileSize) + ":" +
                                std::to_string(modified.time_since_epoch().count());
        TexturePack* pack = nullptr;
        const auto cached = cachedPacks_.find(key);
        if(cached == cachedPacks_.end()) {
          auto owned = std::make_unique<ZippedTexturePack>(entry.path(), defaultPack_.get());
          owned->key = key;
          owned->load();
          pack = owned.get();
          cachedPacks_.emplace(key, std::move(owned));
        } else {
          pack = cached->second.get();
        }
        if(pack->name == selectedName_) {
          selected = pack;
        }
        nextPacks.push_back(pack);
      }
    }
    for(TexturePack* pack : packs_) {
      if(std::find(nextPacks.begin(), nextPacks.end(), pack) != nextPacks.end()) {
        continue;
      }
      if(textureManager_ != nullptr) {
        pack->unload(*textureManager_);
      }
      if(pack != defaultPack_.get()) {
        cachedPacks_.erase(pack->key);
      }
    }
    packs_ = std::move(nextPacks);
    if(selected == nullptr) {
      selected = defaultPack_.get();
    }
  }
  [[nodiscard]] std::vector<TexturePack*> getAvailable() const {
    return packs_;
  }
  TexturePack* selected = nullptr;

private:
  ResourcePack resources_;
  std::unique_ptr<BuiltInTexturePack> defaultPack_;
  std::unordered_map<std::string, std::unique_ptr<ZippedTexturePack>> cachedPacks_;
  std::vector<TexturePack*> packs_;
  std::filesystem::path dir_;
  std::string selectedName_;
  option::GameOptions* options_ = nullptr;
  texture::TextureManager* textureManager_ = nullptr;
};
} // namespace net::minecraft::client::resource::pack
